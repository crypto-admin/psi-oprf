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
#include <gflags/gflags.h>

#include <grpcpp/grpcpp.h>

#ifdef BAZEL_BUILD
#include "src/proto/ot.grpc.pb.h"
#else
#include "src/proto/ot.grpc.pb.h"
#endif
#include "psiSender.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using grpc::ClientReaderWriter;
using ot::Psi;
using ot::Point;
using namespace PSI;

DEFINE_bool(debug, false, "Open the print, and grpc test");
DEFINE_string(port, "50051", "psi server default port");
DEFINE_int32(senderSize, 1024, "sender's data size");
DEFINE_int32(receiverSize, 1024, "receiver's data size");
DEFINE_int32(width, 600, "matrix width");
DEFINE_int32(hashSize, 32, "default hash size");

bool debug = false;

class PsiClient {
 public:
   PsiClient(std::shared_ptr<Channel> channel)
      : stub_(Psi::NewStub(channel)) {}

  // Assembles the client's payload, sends it and presents the response back
  // from the server.
  std::string SendPoint(const std::string& user) {
    
    // Context for the client. It could be used to convey extra information to
    // the server and/or tweak certain RPC behaviors.
    ClientContext context;

    gpr_timespec timespec;
    timespec.tv_sec = 1000;
    timespec.tv_nsec = 0;
    timespec.clock_type=GPR_TIMESPAN;

    context.set_deadline(timespec);

    std::shared_ptr<ClientReaderWriter<Point, Point> > stream1(stub_->SendPoint(&context));

    ClientReaderWriter<Point, Point>* stream = stream1.get();

    if (debug) { // test conn
      // Data we are sending to the server.
      for (int loop = 0; loop < 100; loop++) {
        Point request;
        request.set_pointset(user);

        // Container for the data we expect from the server.
        Point reply;

        // The actual RPC.
        bool res = stream->Write(request);
        Point got;

        stream->Read(&got);
        std::cout << "client got " << got.pointset() <<std::endl;
      }
    }


    int psiRes = PsiSend(stream);


    Status status = stream->Finish();

    // Act upon its status.
    if (status.ok()) {
      // return reply.pointset();
      return "conn ok";
    } else {
      std::cout << status.error_code() << ": " << status.error_message()
                << std::endl;
      return "RPC failed";
    }
  }

 private:
  std::unique_ptr<Psi::Stub> stub_;
};

int main(int argc, char** argv) {
  std::string address = "localhost";
  std::string port = "50051";
  std::string server_address = address + ":" + port;
  std::cout << "Client querying server address: " << server_address << std::endl;


  // Instantiate the client. It requires a channel, out of which the actual RPCs
  // are created. This channel models a connection to an endpoint (in this case,
  // localhost at port 50051). We indicate that the channel isn't authenticated
  // (use of InsecureChannelCredentials()).

  grpc::ChannelArguments channel_args;
  channel_args.SetInt(GRPC_ARG_MAX_RECEIVE_MESSAGE_LENGTH, 10 * 1024 * 1024);
  PsiClient psiClient(grpc::CreateCustomChannel(
      server_address, grpc::InsecureChannelCredentials(), channel_args));
  std::string user("world");

  std::string reply = psiClient.SendPoint(user);
  std::cout << "Greeter received: " << reply << std::endl;

  return 0;
}
