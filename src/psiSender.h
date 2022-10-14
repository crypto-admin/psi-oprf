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

}  // namespace PSI
