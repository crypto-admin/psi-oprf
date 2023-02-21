
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
#include <ctime>
#include "nlohmann/json.hpp"

#include "psiReceiver.h"
#include "common.h"

using json = nlohmann::json;

namespace PSI {

int param_size = 9;
psiparams onlineparam = {1024, 1024, 1024, 10, 200, 16, 32, 256, 256};

// read server params from config file, as csv, json..
int Parserparam() {
  string config;
  ifstream config_file("config/serverConfig.csv", ios::in);
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

int ParseJsonparm() {
  std::ifstream f("config/serverConfig.json");
  json data = json::parse(f);
  // Access the values existing in JSON data

/*
{
    "senderSize": 1024,
    "receiverSize": 4096,
    "height": 4096,
    "logHeight": 12,
    "width" : 600,
    "hashLengthInBytes" :32,
    "h1LengthInBytes": 32,
    "bucket1": 256,
    "bucket2": 256
}
*/

  onlineparam.senderSize = data.at("senderSize").get<uint32_t>();
  onlineparam.receiverSize = data.at("receiverSize").get<uint32_t>();
  onlineparam.height = data.at("height").get<uint32_t>();
  onlineparam.logHeight = data.at("logHeight").get<uint32_t>();
  onlineparam.width = data.at("width").get<uint32_t>();  // int to bool;
  onlineparam.hashLengthInBytes = data.at("hashLengthInBytes").get<uint32_t>();
  onlineparam.h1LengthInBytes = data.at("h1LengthInBytes").get<uint32_t>();
  onlineparam.bucket1 = data.at("bucket1").get<uint32_t>();
  onlineparam.bucket2 = data.at("bucket2").get<uint32_t>();
  std::cout << "receiverSize = " <<  data.at("receiverSize").get<int>() << std::endl;

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
    Point2AffinePoint(B, &k);  // 有重复代码，优化!
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
  for (int i = 0; i < width; i++) {  // why not use batch?
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
    clock_t start, end;
    auto heightInBytes = (height + 7) / 8;
    auto widthInBytes = (width + 7) / 8;
    auto locationInBytes = (logHeight + 7) / 8;
    auto receiverSizeInBytes = (receiverSize + 7) / 8;
    auto shift = (1 << logHeight) - 1;
    auto widthBucket1 = sizeof(block) / locationInBytes;  // 16/3 = 5
    std::cout << "widthBucket1 = " << widthBucket1 << std::endl;

    ///////////////////// Base OTs ///////////////////////////
    start = clock();
    std::vector<affpoint> k0;
    std::vector<affpoint> k1;
    BatchOT(stream, width, k0, k1);
    end = clock();
    std::cout << "BatchOT spent time: " << (double)(end-start)/CLOCKS_PER_SEC << std::endl;


    /****** PSI batch compute Matrix A/B ******/
    start = clock();
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

    Point key;
    std::string keystring = Char2hexstring(reinterpret_cast<char*>(aesKey), 16);
    key.set_pointset(keystring);
    stream->Write(key);  // TODO:open it

    block* recvSet = new block[receiverSize];
    block* aesInput = new block[receiverSize];
    block* aesOutput = new block[receiverSize];

    u8 h1Output[h1LengthInBytes];

    #pragma omp parallell for
    for (auto i = 0; i < receiverSize; ++i) {
      // SM3_Hash((u8*)(receiverSet.data() + i), sizeof(block), h1Output, 32);
      // 20230209 update to Blake3
      Blake3_Hash((u8*)(receiverSet.data() + i), sizeof(block), h1Output);
      // 32 is sm3 out len;
      aesInput[i] = *(block*)h1Output;
      recvSet[i] = *(block*)(h1Output + sizeof(block));
    }

    Sm4EncBlock(aesInput, receiverSize, aesOutput, aesKey);

    for (auto i = 0; i < receiverSize; ++i) {
      for (auto loop = 0; loop < 16; ++loop) {
        recvSet[i].msg[loop] ^= aesOutput[i].msg[loop];
      }
    }
    end = clock();
    std::cout << "Receiver set transstformed, spend time = " << (double)(end-start)/CLOCKS_PER_SEC << std::endl;


    start = clock(); // big loop time
    // create a store for the keys
    uint8_t expandedKeys[176];
    AES_128_Key_Expansion(aesKey, expandedKeys);
    for (auto wLeft = 0; wLeft < width; wLeft += widthBucket1) {
      auto wRight = wLeft + widthBucket1 < width ? wLeft + widthBucket1 : width;
      auto w = wRight - wLeft;
      // start = clock();
      //////////// Compute random locations (transposed) ////////////////
      for (auto low = 0; low < receiverSize; low += bucket1) {
        auto up = low + bucket1 < receiverSize ? low + bucket1 : receiverSize;
        
        Sm4EncBlockWithExpandKeyUp(recvSet + low, up - low, randomLocations, expandedKeys);
        
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
      
      #pragma omp parallell for
      for (auto i = 0; i < w; ++i) {
        for (auto j = 0; j < receiverSize; ++j) {
          auto location = (*(ui32*)(transLocations[i] + j * locationInBytes)) & shift;
          matrixDelta[i][location >> 3] &= ~(1 << (location & 7));
        }
      }

      //////////////// Compute matrix A & sent matrix ///////////////////////
      u8* sentMatrix[w];
      unsigned char* seed = new unsigned char[32];
      unsigned char* r0Extend = new unsigned char[heightInBytes];
      unsigned char* r1Extend = new unsigned char[heightInBytes];
      for (auto i = 0; i < w; ++i) {
        // PRNG prng(otMessages[i + wLeft][0]);
        // prng.get(matrixA[i], heightInBytes);
        auto k0point = k0[i+wLeft];  // affpoint
        auto k1point = k1[i+wLeft];  // affpoint
        Small8toChar(k0point.x, seed);
        Prf(seed, heightInBytes, r0Extend);
        Small8toChar(k1point.x, seed);
        Prf(seed, heightInBytes, r1Extend);
        memcpy(matrixA[i], r0Extend, heightInBytes);
        sentMatrix[i] = new u8[heightInBytes];
        memcpy(sentMatrix[i], r1Extend, heightInBytes);

        for (auto j = 0; j < heightInBytes; ++j) {
          sentMatrix[i][j] ^= (matrixA[i][j] ^ matrixDelta[i][j]);
        }
        // std::string send((char*)(sentMatrix[i]), heightInBytes);
        std::string send = Char2hexstring((char*)(sentMatrix[i]), heightInBytes);
        // Must test data loss;
        Point SendPoint;
        SendPoint.set_pointset(send);
        stream->Write(SendPoint);
      }

      ///////////////// Compute hash inputs (transposed) /////////////////////
      #pragma omp parallell for
      for (auto i = 0; i < w; ++i) {
        for (auto j = 0; j < receiverSize; ++j) {
          auto location = (*(ui32*)(transLocations[i] + j * locationInBytes)) & shift;
          auto  temp = matrixA[i][location >> 3] & (1 << (location & 7));
          transHashInputs[i + wLeft][j >> 3] |= (u8)(bool)temp << (j & 7);
        }
      }
      // end = clock(); // line273
      // std::cout << "Compute hash input, spend time = " << (double)(end-start)/CLOCKS_PER_SEC << std::endl;
    }

    end = clock();
    std::cout << "big loop, spend time = " << (double)(end-start)/CLOCKS_PER_SEC << std::endl;

		/////////////////// Compute hash outputs ///////////////////////////
    // RandomOracle H(hashLengthInBytes);
    u8 hashOutput[sizeof(block)] = {0};
    // u8 hashOutput[hashLengthInBytes];
    std::unordered_map<uint64_t, std::vector<std::pair<block, ui32>>> allHashes;
    start = clock();

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
        // SM3_Hash(hashInputs[j - low], widthInBytes, hashOutput, sizeof(block));
        Blake3_Hash(hashInputs[j - low], widthInBytes, hashOutput);
        allHashes[*(uint64_t*)(hashOutput)].push_back(std::make_pair(*(block*)hashOutput, j));
      }
    }
    end = clock();
    std::cout << "Receiver hash outputs computed, spend time = " << (double)(end-start)/CLOCKS_PER_SEC << std::endl;

    ///////////////// Receive hash outputs from sender and compute PSI ///////////////////
    u8* recvBuff = new u8[bucket2 * hashLengthInBytes];
    auto psi = 0;
    start = clock();
		
    for (auto low = 0; low < senderSize; low += bucket2) {
      auto up = low + bucket2 < senderSize ? low + bucket2 : senderSize;

      Point recvBuffPoint;
      stream->Read(&recvBuffPoint);
      auto recvBufString = recvBuffPoint.pointset();
      char arr[recvBufString.length()+1];
      auto recvLen = Hexstring2char(recvBufString, arr);
      memcpy(recvBuff, (unsigned char*)arr, (up - low) * hashLengthInBytes);

      for (auto idx = 0; idx < up - low; ++idx) {
        uint64_t mapIdx = *(uint64_t*)(recvBuff + idx * hashLengthInBytes);

        auto found = allHashes.find(mapIdx);
        if (found == allHashes.end()) continue;
        
        for (auto i = 0; i < found->second.size(); ++i) {
          if (memcmp(found->second[i].first.msg, recvBuff + idx * hashLengthInBytes, hashLengthInBytes) == 0) {
            ++psi;
            break;
          }
        }
      }
    }
    
    end = clock();
    std::cout << "psi output computed, spend time = " << (double)(end-start)/CLOCKS_PER_SEC << std::endl;

    if (psi == 100) {
      std::cout << "Receiver intersection computed - correct!\n";
    } else {
      std::cout << "psi result err, psi joined num = " << psi <<  std::endl;
    }
    std::cout << "psi server end." << std::endl;

}


int PsiReceive(ServerReaderWriter<Point, Point>* stream,
                 bool debug = false,
                 uint32_t senderSize = 1024,
                 uint32_t receiverSize = 1024,
                 uint32_t height = 1024,
                 int width = 600,
                 int hashSize = 32
              ) {
  // Parserparam();
  ParseJsonparm();
  std::cout << "receiverSize " << onlineparam.receiverSize << std::endl;
  std::vector<block> serverData;
  // string srcFilePath = "src/data/serverData.csv";
  // int res = InitData(srcFilePath, serverData);
  auto res = MockData(&serverData, onlineparam.receiverSize);
  // std::cout << "mockdata = " << std::endl;
  // for (int i = 0; i < 102; i++) PrintBlock(serverData[i]);

  PsiReceiver r;
  clock_t start, end;

  start = clock();

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
  
  end = clock();
  double endtime=(double)(end-start)/CLOCKS_PER_SEC;

  std::cout << "psiReceiver run end, spend time = " << endtime << std::endl;
  
  return 0;
}

}  // namespace PSI
