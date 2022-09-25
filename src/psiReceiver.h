#pragma once

#include <vector>
#include <string>
#include <fstream>
#include <grpcpp/grpcpp.h>
#include "src/proto/ot.grpc.pb.h"
#include "common.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerReaderWriter;
using grpc::Status;
using ot::Psi;
using ot::Point;
using namespace std;


namespace PSI {

	class PsiReceiver {
	public:
		PsiReceiver() {}

		void run(
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
                const ui32& bucket2);

	};

    int PsiReceive(ServerReaderWriter<Point, Point>* stream);

}
