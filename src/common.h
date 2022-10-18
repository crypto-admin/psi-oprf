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

#include <string>
#include <vector>
#include "crypto/sm2.h"
#include "crypto/sm4.h"
#include "crypto/sm3_hash.h"
#include "src/proto/ot.grpc.pb.h"

using ot::Point;

namespace PSI {


#define ui32 unsigned int
#define u8  unsigned char

struct block {
    unsigned char msg[16] = {0};
};

struct block32 {
    ui32 rand[8];
};

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

int PrintAffPoint(affpoint src);
int GetRandom(int length, unsigned char * dst);
int GetRandomUint32(int length, ui32* dst);
int StringSplit(const std::string src,
                char split,
                std::vector<std::string>* des);

int Point2AffinePoint(Point src, affpoint* dst);
int AffinePoint2String(const affpoint& point, std::string* dst);
affpoint PointNeg(affpoint src);
int Sm4EncBlock(block* src, int length, block* dst, unsigned char key[16]);
int Prf(unsigned char *seed, int length, unsigned char *dst);
int Small8toChar(small src[DIG_LEN], unsigned char *dst);
int PrintBlock(block src);
int MockData(std::vector<block>* src, int dataSize);

}  // namespace PSI
