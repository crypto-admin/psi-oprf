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

#ifdef BAZEL_BUILD
#include "src/proto/ot.grpc.pb.h"
#else
#include "src/proto/ot.grpc.pb.h"
#endif

#include "psiReceiver.h"


using namespace PSI;
using namespace std;

int debug = 0;


// Logic and data behind the server's behavior.
class PsiServiceImpl final : public Psi::Service {
  Status SendPoint(ServerContext* context,
                  ServerReaderWriter<Point, Point>* stream) override {
    if (debug != 0) {
      for (int loop = 0; loop < 100; loop++) {
        Point request;
        stream->Read(&request);
        auto reply = request.pointset();
        std::cout << "get request from client = " << reply << std::endl;
        reply = reply + "got";
        Point xa;
        xa.set_pointset(reply);
        stream->Write(xa);
      }
    }


    int resPsiReceive = PsiReceive(stream);  // 20221012确认是内部函数的导致的double free

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

  builder.SetMaxReceiveMessageSize(10 * 1024 * 1024);
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
