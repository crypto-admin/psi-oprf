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
psiparams onlineparam = {1024, 1024, 1024, 10, 10, 32, 32, 256, 256};


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
  std::cout << "batchot sender get ASet = " << randASetString << std::endl;

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
      std::vector<std::string>& senderSet,
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
  for (int i=0; i < width; i++) {
    choiceB[i] = static_cast<int>(choiceB[i]) % 2;
    std::cout << "choice i = " << int(choiceB[i]) << std::endl;
  }
  std::vector<affpoint> kc;
  auto res = BatchOTSender(stream, width, choiceB, kc);
  std::cout << "sender batch ot kc size = " << kc.size() << std::endl;
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
  for (int i = 0; i < 16; i++) {
    std::cout << int(aesKey[i]) << std::endl;
  }



}

int PsiSend(ClientReaderWriter<Point, Point>* stream) {
  Parserparam("src/config/clientConfig.csv");
  std::vector<std::string> clientData;
  std::string srcFilePath = "src/data/client.csv";
  // int res = InitData(srcFilePath, clientData);
  for (int i=0; i < onlineparam.width; i++) {
    clientData.push_back(std::to_string(i+2));
  }

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