
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

#include "psiReceiver.h"
#include "common.h"


namespace PSI {

int param_size = 9;
psiparams onlineparam = {1024, 1024*1024, 1024*1024, 20, 60, 32, 32, 256, 256};



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



// int GetRandAOT1out2(epoint p, ui32* randa) { // 外部申请空间;
//   GetRandomUint32(8, randa);
//   basepointmul(p, randa);

//   return 0;
// }


int ComputeKey(std::string src,
      std::vector<affpoint> randASet,
      int width,
      std::vector<block32> scalarSet,
      std::vector<affpoint>& k0,
      std::vector<affpoint>& k1) {
  // compute k0, k1; src is B, string
  // 1. convert src to affpoint
  // 2. k0 = aB, k1 = a(B-A)
  std::vector<std::string> pointSet;
  StringSplit(src, '\n', pointSet);  // 解析出width个Point
  for (int i=0; i < width; i++) {
    Point B;
    affpoint k;
    affpoint temp0;
    affpoint temp1;
    B.set_pointset(pointSet[i]);
    Point2AffinePoint(B, &k);  // 有重复代码，优化！
    pointmul(&temp0, &k, scalarSet[i].rand);
    k0.push_back(temp0);
    auto fuA = PointNeg(randASet[i]);
    pointadd(&temp1, &k, &fuA);
    pointmul(&temp1, &temp1, scalarSet[i].rand);
    k1.push_back(temp1);
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
  std::cout << "server send A to client." << std::endl;
  // Receiver B from Bob
  Point batchB;
  stream->Read(&batchB);
  std::cout << "server got B from client" << std::endl;
  // compute k0, k1
  // k0 = aB
  // k1 = a(B-A)
  ComputeKey(batchB.pointset(), randASet, width, scalarSet, k0, k1);

  sleep(1);
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
      std::vector<std::string>& receiverSet,
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
    std::cout << "server BatchOT k0 size = " << k0.size() << std::endl;
  }


int PsiReceive(ServerReaderWriter<Point, Point>* stream) {
  Parserparam();
  std::vector<string> serverData;
  // string srcFilePath = "src/data/serverData.csv";
  // int res = InitData(srcFilePath, serverData);
  for (int i=0; i < onlineparam.width; i++) {
    serverData.push_back(std::to_string(i+2));
  }

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
