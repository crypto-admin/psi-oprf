/*
  Copyright [2022] <Crypto-admin>
*/


#include <fstream>
#include <vector>
#include <string>
#include <random>
#include "psiSender.h"


namespace PSI {

const int param_size = 9;
psiparams onlineparam = {1024, 1024, 1024, 10, 60, 16, 32, 256, 256};


int Parserparam(std::string configPath ) {
  std::string config;
  std::ifstream config_file(configPath, std::ios::in);
  if (!config_file.is_open()) {
    std::cout << "open config file fail." << std::endl;
    return 1;
  }
  uint32_t param_temp[param_size]; // 7 is para num of pirparams
  int index = 0;
  while (getline(config_file, config)) {
    param_temp[index] = atoi(config.c_str());
    index++;
    if (index > param_size) break;
  }
  if (index == param_size) {
    onlineparam.senderSize = param_temp[0];
    onlineparam.receiverSize = param_temp[1];
    onlineparam.height = param_temp[2];
    onlineparam.logHeight = param_temp[3];
    onlineparam.width = param_temp[4];  // int to bool;
    onlineparam.hashLengthInBytes = param_temp[5];
    onlineparam.h1LengthInBytes = param_temp[6];
    onlineparam.bucket1 = param_temp[7];
    onlineparam.bucket2 = param_temp[8];
  } else {
    return 2;  // param size error;
  }
  return 0;
}

int InitData(const std::string filePath, std::vector<std::string>& src) {
  std::string ele;
  std::ifstream srcFile(filePath, std::ios::in);
  if (!srcFile.is_open()) {
    std::cout << "open data file fail." << std::endl;
    return 1;
  }
  uint32_t param_temp[param_size];  //  7 is para num of pirparams
  int index = 0;
  while (getline(srcFile, ele)) {
    src.push_back(ele.c_str());
    index++;
  }
}

int BatchOTSender(ClientReaderWriter<Point, Point>* stream,
  const ui32& width,
  unsigned char* choiceB,
  std::vector<affpoint>& kc) {
  Point randA;
  std::vector<Point> randASet;
  std::vector<Point> randBSet;
  std::string randBSetString = "";

  auto ret = stream->Read(&randA);
  std::string randASetString = randA.pointset();
  // std::cout << "batchot sender get ASet = " << randASetString << std::endl;

  std::istringstream iss(randASetString);
  std::string token;
  while (getline(iss, token, '\n')) {
    randA.set_pointset(token);
    randASet.push_back(randA);
  }

  std::cout << "clent get randASet size = " << randASet.size() << std::endl;
  for (int i=0; i < width; i++) {  // why not use batch?
    block32 randb;
    GetRandomUint32(8, randb.rand);
    affpoint A;
    affpoint B;
    basepointmul(&B, randb.rand);  // pointB
    Point2AffinePoint(randASet[i], &A);

    if (static_cast<int>(choiceB[i]) == 1) {
      pointadd(&B, &B, &A);
    }
    affpoint k;
    pointmul(&k, &A, randb.rand);
    kc.push_back(k);
    std::string temp = "";
    AffinePoint2String(B, &temp);
    randBSetString += temp;
  }
  Point batchB;
  batchB.set_pointset(randBSetString);
  stream->Write(batchB);

  return 0;
}
void PsiSendRun(
      ClientReaderWriter<Point, Point>* stream,
      const ui32& senderSize,
      const ui32& receiverSize,
      const ui32& height,
      const ui32& logHeight,
      const ui32& width,
      std::vector<block>& senderSet,
      const ui32& hashLengthInBytes,
      const ui32& h1LengthInBytes,
      const ui32& bucket1,
      const ui32& bucket2) {
  clock_t start;
  auto heightInBytes = (height + 7) / 8;
  auto widthInBytes = (width + 7) / 8;
  auto locationInBytes = (logHeight + 7) / 8;
  auto senderSizeInBytes = (senderSize + 7) / 8;
  auto shift = (1 << logHeight) - 1;
  auto widthBucket1 = sizeof(block) / locationInBytes;  // 16/3 = 5

  ///////////////////// Base OTs ///////////////////////////
  unsigned char * choiceB = new unsigned char[width];
  GetRandom(width, choiceB);
  // for (int i=0; i < width; i++) {
  //   choiceB[i] = static_cast<int>(choiceB[i]) % 2;
  //   std::cout << "choice i = " << int(choiceB[i]) << std::endl;
  // }
  std::vector<affpoint> kc;
  auto res = BatchOTSender(stream, width, choiceB, kc);
  // std::cout << "sender batch ot kc size = " << kc.size() << std::endl;
  // for (int i = 0; i < kc.size(); i++) {
  //   PrintAffPoint(kc[i]);
  //   std::cout << std::endl;
  // }

  /** PSI process **/
  u8* transLocations[widthBucket1];
  for (auto i = 0; i < widthBucket1; ++i) {
    transLocations[i] = new u8[senderSize * locationInBytes + sizeof(ui32)];
  }

  block randomLocations[bucket1];
  u8* matrixC[widthBucket1];  // widthBucket1 是矩阵C的列数
  for (auto i = 0; i < widthBucket1; ++i) {
    matrixC[i] = new u8[heightInBytes];
    // C 中每个元素都是大小为heightInbytes的向量，且初始化为0;
  }

  u8* transHashInputs[width];
  // hash的个数, 也看做一个矩阵，列宽width, 行宽senderSizeInBytes;
  for (auto i = 0; i < width; ++i) {
    transHashInputs[i] = new u8[senderSizeInBytes];
    memset(transHashInputs[i], 0, senderSizeInBytes);
  }

  /////////// Transform input /////////////////////
  Point aesKeyPoint;
  stream->Read(&aesKeyPoint);
  auto key = aesKeyPoint.pointset();
  char arr[key.length()+1];
  unsigned char aesKey[16];
  strcpy(arr, key.c_str());
  memcpy(aesKey, (unsigned char*)arr, 16);
  // for (int i = 0; i < 16; i++) {
  //   std::cout << int(aesKey[i]) << std::endl;
  // }

  block* sendSet = new block[senderSize];
  block* aesInput = new block[senderSize];
  block* aesOutput = new block[senderSize];
  
  // RandomOracle H1(h1LengthInBytes);  // 32, 256bit
  u8 h1Output[h1LengthInBytes];
  std::cout << "-------------aes test.. " << std::endl;
  for (auto i = 0; i < senderSize; ++i) {
    // H1.Reset();
    // H1.Update((u8*)(senderSet.data() + i), sizeof(block));
    // H1.Final(h1Output);   // H1(x)
    SM3_Hash((u8*)(senderSet.data() + i), sizeof(block), h1Output, 32);

    aesInput[i] = *(block*)h1Output; // 做了截断, 前16字节；
    // PrintBlock(aesInput[i]);
    sendSet[i] = *(block*)(h1Output + sizeof(block));  // 后16字节;
  }

  // commonAes.ecbEncBlocks(aesInput, senderSize, aesOutput);
  Sm4EncBlock(aesInput, senderSize, aesOutput, aesKey);
  // for (int i = 0; i < 100; i++) {
  //   PrintBlock(aesOutput[i]);
  // }

  for (auto i = 0; i < senderSize; ++i) {
      for (auto loop = 0; loop < 16; ++loop) {
        sendSet[i].msg[loop] ^= aesOutput[i].msg[loop];
      }
  }

  std::cout << "Sender set transformed." << std::endl;
  for (int wLeft = 0; wLeft < width; wLeft += widthBucket1) {
    auto wRight = wLeft + widthBucket1 < width ? wLeft + widthBucket1 : width;
    int w = wRight - wLeft;  // mostly == withBucket1

    for (auto low = 0; low < senderSize; low += bucket1) {
      auto up = low + bucket1 < senderSize ? low + bucket1 : senderSize;
      // 每次处理256行; 如果一次处理sendersize行，太大了，赋值时间耗时长
      // commonAes.ecbEncBlocks(sendSet + low, up - low, randomLocations);
      Sm4EncBlock(sendSet + low, up - low, randomLocations, aesKey);
      // randomLocations is output;
      for (auto i = 0; i < w; ++i) {
        for (auto j = low; j < up; ++j) {
          memcpy(transLocations[i] + j * locationInBytes, (u8*)(randomLocations + (j - low)) + i * locationInBytes, locationInBytes);
        }
      }
    }

    //////////////// Extend OTs and compute matrix C ///////////////////
    u8* recvMatrix;
    recvMatrix = new u8[heightInBytes];

    for (auto i = 0; i < w; ++i) {
      // PRNG prng(otMessages[i + wLeft]);
      // prng.get(matrixC[i], heightInBytes);
      auto kcpoint = kc[i+wLeft];  // affpoint
      unsigned char *seed = new unsigned char[32];
      unsigned char* rcExtend = new unsigned char[heightInBytes];
      // for (int nn = 0; nn < 8; nn++) {
      //   std::cout << kcpoint.x[nn];
      // }
      // std::cout << "kcpoint test=========================" << std::endl;
      Small8toChar(kcpoint.x, seed);
      Prf(seed, heightInBytes, rcExtend);
      for (ui32 k = 0; k < heightInBytes; k++) {
        matrixC[i][k] = rcExtend[k];
      }
      // ch.recv(recvMatrix, heightInBytes);
      Point matrixCol;
      stream->Read(&matrixCol);  // heightInBytes
      auto matrixColString = matrixCol.pointset();
      char arrCol[heightInBytes+1];
      // strcpy(arrCol, matrixColString.c_str());
      Hexstring2char(matrixColString, arrCol);
      memcpy(recvMatrix, (unsigned char*)arrCol, heightInBytes);
      if (int(choiceB[i + wLeft])) {
        for (auto j = 0; j < heightInBytes; ++j) {
          matrixC[i][j] ^= recvMatrix[j];
        }
      }
    }
    ///////////////// Compute hash inputs (transposed) /////////////////////
    for (auto i = 0; i < w; ++i) {
      for (auto j = 0; j < senderSize; ++j) {  // H2 input == transHashInputs
        auto location = (*(ui32*)(transLocations[i] + j * locationInBytes)) & shift;
        transHashInputs[i + wLeft][j >> 3] |= (u8)((bool)(matrixC[i][location >> 3] & (1 << (location & 7)))) << (j & 7);
        // 按位或;跟papser的连接不同
      }
    }
  }
  std::cout << "Sender transposed hash input computed\n";

  /////////////////// Compute hash outputs ///////////////////////////
  // RandomOracle H(hashLengthInBytes); // SM3
  u8 hashOutput[sizeof(block)]; // think
  // u8 hashOutput[hashLengthInBytes];
  u8* hashInputs[bucket2];

  for (auto i = 0; i < bucket2; ++i) {
    hashInputs[i] = new u8[widthInBytes];
  }

  for (auto low = 0; low < senderSize; low += bucket2) {
    auto up = low + bucket2 < senderSize ? low + bucket2 : senderSize;

    for (auto j = low; j < up; ++j) {
      memset(hashInputs[j - low], 0, widthInBytes);
    }

    for (auto i = 0; i < width; ++i) {
      for (auto j = low; j < up; ++j) {
        hashInputs[j - low][i >> 3] |= (u8)((bool)(transHashInputs[i][j >> 3] & (1 << (j & 7)))) << (i & 7);
      }
    }

    u8* sentBuff = new u8[(up - low) * hashLengthInBytes];

    for (auto j = low; j < up; ++j) {
      // H.Reset();
      // H.Update(hashInputs[j - low], widthInBytes);
      // H.Final(hashOutput);
      SM3_Hash(hashInputs[j-low], widthInBytes, hashOutput, sizeof(block));
      // PrintBlock(*(block*)hashOutput);
      if (j == 0) {
        std::cout << "hash test sender" << std::endl;
        for (int nn = 0; nn < sizeof(block); nn++) 
          std::cout << int(hashOutput[nn]) << ",";
        std::cout << "&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&" << hashLengthInBytes << std::endl;
      }
    
      memcpy(sentBuff + (j - low) * hashLengthInBytes, hashOutput, hashLengthInBytes);
    }
    if (low == 0) {
      for (int xx = 0; xx < (up-low)* hashLengthInBytes; xx++) {
        if (xx % hashLengthInBytes == 0  && xx>0 ) std::cout << std::endl;
        printf("%02x,", (int)(unsigned char)(sentBuff[xx]));
        
      }
      std::cout << "sender oprf test--------------------------------" << std::endl;
    }

    // ch.asyncSend(sentBuff, (up - low) * hashLengthInBytes);
    Point hashSendPoint;
    std::string hashSendString = Char2hexstring((char*)sentBuff, (up - low) * hashLengthInBytes);
    // test ------------
    char testxx[hashSendString.length()+1];
    // strcpy(testxx, hashSendString.data());
    auto len = Hexstring2char(hashSendString, testxx);
    // if (low == 0) {
    //   std::cout << "convert test----------------------" << std::endl;
    //   for (int xx = 0; xx < (up-low)* hashLengthInBytes; xx++) {
    //     if (xx % hashLengthInBytes == 0  && xx>0 ) std::cout << std::endl;
    //     printf("%02x,", int((unsigned char)testxx[xx]));
        
    //   }
    // }
    // end test ----------------
    hashSendPoint.set_pointset(hashSendString);
    stream->Write(hashSendPoint);
  }

  std::cout << "Sender hash outputs computed and sent" << std::endl;
}

int PsiSend(ClientReaderWriter<Point, Point>* stream) {
  Parserparam("src/config/clientConfig.csv");
  std::vector<block> clientData;
  std::string srcFilePath = "src/data/client.csv";
  // int res = InitData(srcFilePath, clientData);
  auto res = MockData(&clientData, onlineparam.senderSize);
  // for (int i = 0; i < 102; i++) PrintBlock(clientData[i]);

  // PsiSender sender;
  PsiSendRun(stream, \
    onlineparam.senderSize,
    onlineparam.receiverSize,
    onlineparam.height,
    onlineparam.logHeight,
    onlineparam.width,
    clientData,
    onlineparam.hashLengthInBytes,
    onlineparam.h1LengthInBytes,
    onlineparam.bucket1,
    onlineparam.bucket2);

  return 0;
}

}