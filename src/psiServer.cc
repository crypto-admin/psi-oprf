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
#include <gflags/gflags.h>

#ifdef BAZEL_BUILD
#include "src/proto/ot.grpc.pb.h"
#else
#include "src/proto/ot.grpc.pb.h"
#endif

#include "psiReceiver.h"


using namespace PSI;
using namespace std;

DEFINE_bool(debug, false, "Open the print, and grpc test");
DEFINE_string(port, "50051", "psi server default port");
DEFINE_int32(senderSize, 1024, "sender's data size");
DEFINE_int32(receiverSize, 1024, "receiver's data size");
DEFINE_int32(width, 600, "matrix width");
DEFINE_int32(hashSize, 32, "default hash size");

// Logic and data behind the server's behavior.
class PsiServiceImpl final : public Psi::Service {
 public:
  PsiServiceImpl(bool debug,
                 uint32_t senderSize,
                 uint32_t receiverSize,
                 uint32_t height,
                 int width,
                 int hashSize
                 ) {
    debug = debug,
    senderSize = senderSize;
    receiverSize = receiverSize;
    height = height;
    width = width;
    hashSize = hashSize;
  }
  Status SendPoint(ServerContext* context,
                  ServerReaderWriter<Point, Point>* stream) override {
    if (debug) {
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

    int resPsiReceive = PsiReceive(stream, debug, senderSize,receiverSize, height, width, hashSize);
    std::cout << resPsiReceive << std::endl;
    // 20221012确认是内部函数的导致的double free

    return Status::OK;
  }

 private:
    bool debug = false;
    uint32_t senderSize = 1024;
    uint32_t receiverSize = 1024;
    uint32_t height = 1024;
    int width = 600;
    int hashSize = 32;
};

void RunServer(bool debug,
              std::string serverPort,
              uint32_t senderSize,
              uint32_t receiverSize,
              uint32_t height,
              int width,
              int hashSize
              ) {
  std::string address = "0.0.0.0";
  std::string port = "50051";
  if (serverPort != "50051") {
    port = serverPort;
  }

  std::string server_address = address + ":" + port;
  PsiServiceImpl servicePsi(debug, senderSize, receiverSize, height, width, hashSize);

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
  google::ParseCommandLineFlags(&argc, &argv, true);
  std::cout << "port:" << FLAGS_port << std::endl;
  std::cout << "debug:" << FLAGS_debug << std::endl;
  std::cout << "senderSize :" << FLAGS_senderSize << std::endl;
  std::cout << "receiverSize:" << FLAGS_receiverSize << std::endl;
  std::cout << "width:" << FLAGS_width << std::endl;

  RunServer(FLAGS_debug, FLAGS_port, FLAGS_senderSize, FLAGS_receiverSize, FLAGS_receiverSize, FLAGS_width, 32);

  return 0;
}
