
#pragma once

#include <iostream>
#include <sstream>
#include <memory>
#include <grpcpp/grpcpp.h>
#include "src/proto/ot.grpc.pb.h"
#include "common.h"


using grpc::ClientReaderWriter;
using ot::Point;
using namespace std;

namespace PSI {


// class PsiSender {
//  public:
//   PsiSender() {}
//   ~PsiSender() {}

//   void run(std::shared_ptr<ClientReaderWriter<Point, Point> > stream,
//             const ui32& senderSize, 
//             const ui32& receiverSize, 
//             const ui32& height, 
//             const ui32& logHeight, 
//             const ui32& width, 
//             std::vector<std::string>& senderSet,
//             const ui32& hashLengthInBytes, 
//             const ui32& h1LengthInBytes, 
//             const ui32& bucket1, 
//             const ui32& bucket2);

// };

void PsiSendRun(ClientReaderWriter<Point, Point>* stream,
            const ui32& senderSize,
            const ui32& receiverSize,
            const ui32& height,
            const ui32& logHeight,
            const ui32& width,
            std::vector<std::string>& senderSet,
            const ui32& hashLengthInBytes,
            const ui32& h1LengthInBytes,
            const ui32& bucket1,
            const ui32& bucket2);
int PsiSend(ClientReaderWriter<Point, Point>* stream);

}