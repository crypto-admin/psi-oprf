
/*
 *
 * Copyright 2022 crypto-admin.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */
#include <unistd.h>
#include <iostream>
#include <sstream>
#include <memory>
#include <random>
#include <unordered_map>
#include <utility>

#include "psiReceiver.h"
#include "common.h"


namespace PSI {

int param_size = 9;
psiparams onlineparam = {1024, 1024, 1024, 10, 60, 32, 32, 256, 256};

// read server params from config file, as csv, json..
int Parserparam() {
  string config;
  ifstream config_file("src/config/serverConfig.csv", ios::in);
  if (!config_file.is_open()) {
    std::cout << "open config file fail." << std::endl;
    return 1;
  }
  uint32_t param_temp[param_size];  // 9 is para num of pirparams
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

int InitData(std::string filePath, std::vector<string>& src) {
  string ele;
  ifstream srcFile(filePath, ios::in);
  if (!srcFile.is_open()) {
    std::cout << "open data file fail." << std::endl;
    return 1;
  }
  uint32_t param_temp[param_size];  // 9 is para num of pirparams
  int index = 0;
  while (getline(srcFile, ele)) {
    src.push_back(ele.c_str());
    index++;
  }
}

int ComputeKey(std::string src,
      std::vector<affpoint> randASet,
      int width,
      std::vector<block32> scalarASet,
      std::vector<affpoint>& k0,
      std::vector<affpoint>& k1) {
  // compute k0, k1; src is B, string
  // 1. convert src to affpoint
  // 2. k0 = aB, k1 = a(B-A)
  std::vector<std::string> pointSet;
  StringSplit(src, '\n', &pointSet);  // 解析出width个Point
  for (int i=0; i < width; i++) {
    Point B;
    affpoint k;
    affpoint temp0;
    affpoint temp1;
    affpoint temp2;
    B.set_pointset(pointSet[i]);
    Point2AffinePoint(B, &k);  // 有重复代码，优化！
    pointmul(&temp0, &k, scalarASet[i].rand);  // a*B
    k0.push_back(temp0);
    auto fuA = PointNeg(randASet[i]);  // fuA = -A
    pointadd(&temp1, &k, &fuA);
    pointmul(&temp2, &temp1, scalarASet[i].rand);
    k1.push_back(temp2);
  }

  return 0;
}

int BatchOT(ServerReaderWriter<Point, Point>* stream,
            const ui32& width,
            std::vector<affpoint>& k0,
            std::vector<affpoint>& k1) {
  std::vector<affpoint> randASet;
  std::vector<block32> scalarSet;

  std::string randASetString = "";
  for (int i=0; i < width; i++) {  // why not use batch?
    block32 randa;
    GetRandomUint32(8, randa.rand);
    affpoint p;
    basepointmul(&p, randa.rand);
    randASet.push_back(p);
    scalarSet.push_back(randa);
    std::string temp = "";
    AffinePoint2String(p, &temp);
    randASetString += temp;
  }
  Point send;
  send.set_pointset(randASetString);
  stream->Write(send);
  // std::cout << "server send A to client." << std::endl;
  // Receiver B from Bob
  Point batchB;
  stream->Read(&batchB);
  // std::cout << "server got B from client" << std::endl;
  // compute k0, k1
  // k0 = aB
  // k1 = a(B-A)
  ComputeKey(batchB.pointset(), randASet, width, scalarSet, k0, k1);

  // sleep(1);
  std::cout << "server ot end." << std::endl;

  return 0;
}

void PsiReceiver::run(
      ServerReaderWriter<Point, Point>* stream,
      const ui32& senderSize,
      const ui32& receiverSize,
      const ui32& height,
      const ui32& logHeight,
      const ui32& width,
      std::vector<block>& receiverSet,
      const ui32& hashLengthInBytes,
      const ui32& h1LengthInBytes,
      const ui32& bucket1,
      const ui32& bucket2) {
    clock_t start;
    auto heightInBytes = (height + 7) / 8;
    auto widthInBytes = (width + 7) / 8;
    auto locationInBytes = (logHeight + 7) / 8;
    auto receiverSizeInBytes = (receiverSize + 7) / 8;
    auto shift = (1 << logHeight) - 1;
    auto widthBucket1 = sizeof(block) / locationInBytes;  // 16/3 = 5

    ///////////////////// Base OTs ///////////////////////////
    std::vector<affpoint> k0;
    std::vector<affpoint> k1;
    BatchOT(stream, width, k0, k1);
    // std::cout << "server BatchOT k0 size = " << k0.size() << std::endl;
    // for (int i = 0; i < k0.size(); i++) {
    //   PrintAffPoint(k0[i]);
    //   PrintAffPoint(k1[i]);
    //   std::cout << "-------------------------------" << std::endl;
    // }  // Test Batch OT correctness

    /****** PSI batch compute Matrix A/B ******/
    u8* matrixA[widthBucket1];  // 每次处理5列
    u8* matrixDelta[widthBucket1];
    for (auto i = 0; i < widthBucket1; ++i) {
      matrixA[i] = new u8[heightInBytes];
      matrixDelta[i] = new u8[heightInBytes];
    }


    u8* transLocations[widthBucket1];
    for (auto i = 0; i < widthBucket1; ++i) {
      transLocations[i] = new u8[receiverSize * locationInBytes + sizeof(ui32)];
    }

    block randomLocations[bucket1];

    u8* transHashInputs[width];
    for (auto i = 0; i < width; ++i) {
      transHashInputs[i] = new u8[receiverSizeInBytes];
      memset(transHashInputs[i], 0, receiverSizeInBytes);
    }
    unsigned char aesKey[16];
    GetRandom(16, aesKey);
    for (int i = 0; i < 16; i++) {
      std::cout << int(aesKey[i]) << std::endl;
    }

    Point key;
    std::string keystring(reinterpret_cast<char*>(aesKey), 16);
    key.set_pointset(keystring);
    stream->Write(key);  // TODO:open it

    block* recvSet = new block[receiverSize];
    block* aesInput = new block[receiverSize];
    block* aesOutput = new block[receiverSize];

    u8 h1Output[h1LengthInBytes];

    for (auto i = 0; i < receiverSize; ++i) {
      SM3_Hash((u8*)(receiverSet.data() + i), sizeof(block), h1Output, 32); 
      // 32 is sm3 out len;
      aesInput[i] = *(block*)h1Output;
      recvSet[i] = *(block*)(h1Output + sizeof(block));
    }
    
    // for (int x = 0; x < 16; x++) {
    //   std::cout << int(aesInput[0].msg[x]) << std::endl;
    // }
    Sm4EncBlock(aesInput, receiverSize, aesOutput, aesKey);

    for (auto i = 0; i < receiverSize; ++i) {
      for (auto loop = 0; loop < 16; ++loop) {
        recvSet[i].msg[loop] ^= aesOutput[i].msg[loop];
      }
    }
    //
    std::cout << "Receiver set transstformed" << std::endl;

    for (auto wLeft = 0; wLeft < width; wLeft += widthBucket1) {
      auto wRight = wLeft + widthBucket1 < width ? wLeft + widthBucket1 : width;
      auto w = wRight - wLeft;

      //////////// Compute random locations (transposed) ////////////////
      for (auto low = 0; low < receiverSize; low += bucket1) {
        auto up = low + bucket1 < receiverSize ? low + bucket1 : receiverSize;

        Sm4EncBlock(recvSet + low, up - low, randomLocations, aesKey);

        for (auto i = 0; i < w; ++i) {
          for (auto j = low; j < up; ++j) {
            memcpy(transLocations[i] + j * locationInBytes, (u8*)(randomLocations + (j - low)) + i * locationInBytes, locationInBytes);
          }
        }
      }

      //////////// Compute matrix Delta /////////////////////////////////

      for (auto i = 0; i < widthBucket1; ++i) {
        memset(matrixDelta[i], 255, heightInBytes);
      }

      for (auto i = 0; i < w; ++i) {
        for (auto j = 0; j < receiverSize; ++j) {
          auto location = (*(ui32*)(transLocations[i] + j * locationInBytes)) & shift;
          matrixDelta[i][location >> 3] &= ~(1 << (location & 7));
        }
      }

      //////////////// Compute matrix A & sent matrix ///////////////////////

      u8* sentMatrix[w];

      for (auto i = 0; i < w; ++i) {
        // PRNG prng(otMessages[i + wLeft][0]);
        // prng.get(matrixA[i], heightInBytes);
        auto k0point = k0[i+wLeft];  // affpoint
        auto k1point = k1[i+wLeft];  // affpoint
        unsigned char seed[32];
        unsigned char* r0Extend = new unsigned char[heightInBytes];
        unsigned char* r1Extend = new unsigned char[heightInBytes];
        Small8toChar(k0point.x, seed);
        Prf(seed, heightInBytes, r0Extend);
        Small8toChar(k1point.x, seed);
        Prf(seed, heightInBytes, r1Extend);
        memcpy(matrixA[i], r0Extend, heightInBytes);
        sentMatrix[i] = new u8[heightInBytes];
        memcpy(sentMatrix[i], r1Extend, heightInBytes);

        for (auto j = 0; j < heightInBytes; ++j) {
          sentMatrix[i][j] ^= matrixA[i][j] ^ matrixDelta[i][j];
        }
        std::string send(reinterpret_cast<char*>(sentMatrix[i]), heightInBytes);
        // Must test data loss;
        Point SendPoint;
        SendPoint.set_pointset(send);
        stream->Write(SendPoint);
        // ch.asyncSend(sentMatrix[i], heightInBytes);
      }

      ///////////////// Compute hash inputs (transposed) /////////////////////
      for (auto i = 0; i < w; ++i) {
        for (auto j = 0; j < receiverSize; ++j) {
          auto location = (*(ui32*)(transLocations[i] + j * locationInBytes)) & shift;
          auto  temp = matrixA[i][location >> 3] & (1 << (location & 7));
          transHashInputs[i + wLeft][j >> 3] |= (u8)(bool)temp << (j & 7);
        }
      }
    }

    std::cout << "Receiver matrix sent and transposed hash input computed" << std::endl;

		/////////////////// Compute hash outputs ///////////////////////////
    // RandomOracle H(hashLengthInBytes);
    u8 hashOutput[sizeof(block)];
    // u8 hashOutput[hashLengthInBytes];
    std::unordered_map<uint64_t, std::vector<std::pair<block, ui32>>> allHashes;

    u8* hashInputs[bucket2];
    for (auto i = 0; i < bucket2; ++i) {
      hashInputs[i] = new u8[widthInBytes];
    }

    for (auto low = 0; low < receiverSize; low += bucket2) {
      auto up = low + bucket2 < receiverSize ? low + bucket2 : receiverSize;

      for (auto j = low; j < up; ++j) {
        memset(hashInputs[j - low], 0, widthInBytes);
      }

      for (auto i = 0; i < width; ++i) {
        for (auto j = low; j < up; ++j) {
          hashInputs[j - low][i >> 3] |= (u8)((bool)(transHashInputs[i][j >> 3] & (1 << (j & 7)))) << (i & 7);
        }
      }

      for (auto j = low; j < up; ++j) {
        // H.Reset();
        // H.Update(hashInputs[j - low], widthInBytes);
        // H.Final(hashOutput);
        SM3_Hash(hashInputs[j - low], widthInBytes, hashOutput, sizeof(block));
        allHashes[*(uint64_t*)(hashOutput)].push_back(std::make_pair(*(block*)hashOutput, j));
      }
    }

    std::cout << "Receiver hash outputs computed\n";

    ///////////////// Receive hash outputs from sender and compute PSI ///////////////////
    u8* recvBuff = new u8[bucket2 * hashLengthInBytes];
    auto psi = 0;
		
    for (auto low = 0; low < senderSize; low += bucket2) {
      auto up = low + bucket2 < senderSize ? low + bucket2 : senderSize;

      // ch.recv(recvBuff, (up - low) * hashLengthInBytes);
      Point recvBuffPoint;
      stream->Read(&recvBuffPoint);
      auto recvBufString = recvBuffPoint.pointset();
      char arr[recvBufString.length()+1];
      strcpy(arr, recvBufString.c_str());
      memcpy(recvBuff, (unsigned char*)arr, (up - low) * hashLengthInBytes);


      for (auto idx = 0; idx < up - low; ++idx) {
        uint64_t mapIdx = *(uint64_t*)(recvBuff + idx * hashLengthInBytes);

        auto found = allHashes.find(mapIdx);
        if (found == allHashes.end()) continue;
        
        for (auto i = 0; i < found->second.size(); ++i) {
          if (memcmp(&(found->second[i].first), recvBuff + idx * hashLengthInBytes, hashLengthInBytes) == 0) {
            ++psi;
            break;
          }
        }
      }
    }

    if (psi == 100) {
      std::cout << "Receiver intersection computed - correct!\n";
    } else {
      std::cout << "psi result err, psi joined num = " << psi <<  std::endl;
    }
    std::cout << "psi server end." << std::endl;

}


int PsiReceive(ServerReaderWriter<Point, Point>* stream) {
  Parserparam();
  std::vector<block> serverData;
  // string srcFilePath = "src/data/serverData.csv";
  // int res = InitData(srcFilePath, serverData);
  auto res = MockData(&serverData, onlineparam.receiverSize);

  PsiReceiver r;
  r.run(stream, \
      onlineparam.senderSize,
      onlineparam.receiverSize,
      onlineparam.height,
      onlineparam.logHeight,
      onlineparam.width,
      serverData,
      onlineparam.hashLengthInBytes,
      onlineparam.h1LengthInBytes,
      onlineparam.bucket1,
      onlineparam.bucket2);
}

}  // namespace PSI
