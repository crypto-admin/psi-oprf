/*
 *
 * Copyright 2015 gRPC authors.
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

#include <iostream>
#include <memory>
#include <string>
#include <fstream>

#ifdef BAZEL_BUILD
#include "src/proto/ot.grpc.pb.h"
#else
#include "src/proto/ot.grpc.pb.h"
#endif

#include "psiReceiver.h"


using namespace PSI;
using namespace std;

int param_size = 9;

struct psiparams {
    ui32 senderSize;
    ui32 receiverSize;
    ui32 height; 
    ui32 logHeight; 
    ui32 width; 
    ui32 hashLengthInBytes;  // h2 hash byte len, default 32
    ui32 h1LengthInBytes; 
    ui32 bucket1;
    ui32 bucket2;
};

psiparams onlineparam = {1024, 1024*1024, 1024*1024, 20, 600, 32, 32, 256, 256};

// read server params from config file, as csv, json..
int Parserparam() {
  string config;
  ifstream config_file("src/config/serverConfig.csv", ios::in);
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
    onlineparam.width = param_temp[4]; // int to bool;
    onlineparam.hashLengthInBytes = param_temp[5];
    onlineparam.h1LengthInBytes = param_temp[6];
    onlineparam.bucket1 = param_temp[7];
    onlineparam.bucket2 = param_temp[8];
  } else {
    return 2; // param size error;
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
  uint32_t param_temp[param_size]; // 7 is para num of pirparams
  int index = 0;
  while (getline(srcFile, ele)) {
    src.push_back(ele.c_str());
    index++;
  }
}

// Logic and data behind the server's behavior.
class PsiServiceImpl final : public Psi::Service {

  Status SendPoint(ServerContext* context, 
                  ServerReaderWriter<Point, Point>* stream
                 ) override {

    Point request;
    stream->Read(&request);
    auto reply = request.pointset();   
    std::cout << "get request from client = " << reply << std::endl;
    reply = reply + "got";
    Point xa;
    xa.set_pointset(reply); 
    stream->Write(xa);
    PsiReceiver r;

    Parserparam();
    std::vector<string> serverData;
    string srcFilePath = "src/data/server.csv";
    int res = InitData(srcFilePath, serverData);

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
      onlineparam.bucket2
    );

    return Status::OK;
  }
};

void RunServer() {
  std::string address = "0.0.0.0";
  std::string port = "50051";
  std::string server_address = address + ":" + port;
  PsiServiceImpl servicePsi;

  ServerBuilder builder;
  // Listen on the given address without any authentication mechanism.
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  // Register "service" as the instance through which we'll communicate with
  // clients. In this case it corresponds to an *synchronous* service.
  builder.RegisterService(&servicePsi);
  // Finally assemble the server.
  std::unique_ptr<Server> server(builder.BuildAndStart());
  std::cout << "Server listening on " << server_address << std::endl;

  // Wait for the server to shutdown. Note that some other thread must be
  // responsible for shutting down the server for this call to ever return.
  server->Wait();
}

int main(int argc, char** argv) {
  RunServer();

  return 0;
}
