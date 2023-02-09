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
#include <random>
#include <string>
#include <sstream>
#include "common.h"

using namespace std;
using ot::Point;

namespace PSI
{

    // p = 2^256-2^224-2^96+2^64-1
small P[DIG_LEN] = {
    0xFFFFFFFF, 0xFFFFFFFF, 0x00000000, 0xFFFFFFFF,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFE };


int PrintAffPoint(affpoint src) {
    std::string dst = "";
    AffinePoint2String(src, &dst);
    return 0;
}

int GetRandom(int length, unsigned char * dst) {
  std::random_device rd;
  std::default_random_engine eng(rd());
  std::uniform_int_distribution<int> distr(1, 255);

  for (int n = 0; n < length; n++) {
    dst[n] = distr(eng);
  }

  return 0;
}

int GetRandomUint32(int length, ui32* dst) {
  unsigned char temp[4*length];
  GetRandom(4*length, temp);
  for (int i = 0; i < length; i++) {
    dst[i] = (unsigned int)((temp[4*i]) << 24 |
            (temp[4*i+1]) << 16 |
            (temp[4*i+2]) << 8 |
            temp[4*i+3]);
  }
}

int AffinePoint2String(const affpoint& point, std::string* dst) {
  for (int i = 0; i < DIG_LEN; i++) {
    *dst += to_string(point.x[i]);
    *dst += "|";
  }
  *dst += ",";
    for (int i = 0; i < DIG_LEN; i++) {
    *dst += to_string(point.y[i]);
    *dst += "|";
  }
  *dst += "\n";
  return 0;
}

int StringSplit(const std::string src,
                char split,
                std::vector<std::string>* des) {
  std::istringstream iss(src);
  std::string token;
  while (getline(iss, token, split)) {
    des->push_back(token);
  }
  return 0;
}

int Point2AffinePoint(Point src, affpoint* dst) {
  std::vector<std::string> xy;
  std::vector<std::string> x;
  std::vector<std::string> y;

  StringSplit(src.pointset(), ',', &xy);
  // assert(x.size == 2);  // 2个坐标(x, y)
  StringSplit(xy[0], '|', &x);
  StringSplit(xy[1], '|', &y);
  for (int i = 0; i < DIG_LEN; i++) {
    dst->x[i] = atoi(x[i].c_str());
    dst->y[i] = atoi(y[i].c_str());
  }

  return 0;
}


affpoint PointNeg(affpoint src) {
  affpoint fu;
  small  temp[DIG_LEN];
  sub(temp, P, src.y);
  for (int i = 0; i < DIG_LEN; i++) {
    fu.x[i] = src.x[i];
    fu.y[i] = temp[i];
  }


  return fu;
}

int Sm4EncBlock(block* src, int length, block* dst, unsigned char key[16]) {
  // length is block num
  // sm4_context ctx;
  // sm4_setkey_enc(&ctx, key);
  // for (int blockNum = 0; blockNum < length; blockNum++) {
  //   sm4_crypt_ecb(&ctx, 16, src[blockNum].msg, dst[blockNum].msg);
  // }
  // 230307 update to AES
  // AES_KEY keyAes;
  // AES_set_encrypt_key(key, 128, &keyAes);
  // for (int blockNum = 0; blockNum < length; blockNum++) {
  //    AES_encrypt(src[blockNum].msg, dst[blockNum].msg, &keyAes);
  // }

  // create a store for the keys
  uint8_t expandedKeys[176];

  AES_128_Key_Expansion(key, expandedKeys);

  for (int blockNum = 0; blockNum < length; blockNum++) {
    AES_ECB_encrypt(src[blockNum].msg, dst[blockNum].msg, expandedKeys);
  }

  return 0;
}


int Blake3_Hash(uint8_t* in, uint32_t length, uint8_t* hashOut) {
    // Initialize the hasher.
  blake3_hasher hasher;
  blake3_hasher_init(&hasher);
  blake3_hasher_update(&hasher, in, length);


  // Finalize the hash. BLAKE3_OUT_LEN is the default output length, 32 bytes.
  // uint8_t output[BLAKE3_OUT_LEN];
  blake3_hasher_finalize(&hasher, hashOut, BLAKE3_OUT_LEN);
  return 0;
}

int Prf(unsigned char *seed, int length, unsigned char *dst) {
    // 使用SM3， 将seed扩展为length长度;
    int blockNum = length / 32;  // 32 is sm3 out len;
    int left = length % 32;
    
    unsigned char hashOut[32];
    unsigned char hashIn[32];
    memcpy(hashIn, seed, 32);

    for (int i = 0; i < blockNum; ++i) {
        // SM3_Hash(hashIn, 32, hashOut, 32);
        Blake3_Hash(hashIn, 32, hashOut);
        memcpy(dst+i*32, hashOut, 32);
        memcpy(hashIn, hashOut, 32);
    }
    // SM3_Hash(hashIn, 32, hashOut, 32);
    Blake3_Hash(hashIn, 32, hashOut);
    if (left) memcpy(dst + 32*blockNum, hashOut, left);

    return 0;
}

int Small8toChar(small src[DIG_LEN], unsigned char *dst) {
    for (int i = 0; i < DIG_LEN; i++) {
        dst[i*4] = (u8)((src[i] >> 24) & 0xff);
        dst[i*4+1] = (u8)(src[i] >> 16 & 0xff);
        dst[i*4+2] = (u8)(src[i] >> 8 & 0xff);
        dst[i*4+3] = (u8)(src[i] & 0xff);
    }

    return 0;
}

int PrintBlock(block src) {
  for (int i = 0; i < 16; i++) {
    printf("%02x,", src.msg[i]);
  }
  std::cout << std::endl;
}

int MockData(std::vector<block>* src, int dataSize) {
  unsigned char hashSrc[16];
  unsigned char hashDst[16];
  unsigned int seed = 100;
  for (int i = 0; i < 100; i++) {
    for (int k = 0; k < 16; k++) {
      unsigned char temp = rand_r(&seed) % 128;
      hashSrc[k] = temp;
    }
    SM3_Hash(hashSrc, 16, hashDst, 16);
    block temp;
    memcpy(temp.msg, hashDst, 16);
    src->push_back(temp);
    // PrintBlock(temp);
  }

  for (int i = 100; i < dataSize; i++) {
    GetRandom(16, hashSrc);
    SM3_Hash(hashSrc, 16, hashDst, 16);
    block temp;
    memcpy(temp.msg, hashDst, 16);
    src->push_back(temp);
  }

  return 0;
}


std::string Char2hexstring(char* str, int n)
{
  std::ostringstream oss;
  oss << std::hex;
  oss << std::setfill('0');
  oss << std::uppercase;   //大写
  for (int i = 0; i < n; i++)
  {
    unsigned char c = str[i];
    oss  << "0x" << std::setw(2) << (unsigned int)c;
    if (i < n - 1)
      oss << ',';
  }
  return oss.str();
}


//char* out输出结果，n返回数据的个数
int Hexstring2char(const std::string& str, char* out) {
  int n = 0;
  int temp;
  std::istringstream iss(str);
  iss >> std::hex;
  while (iss >> temp) {
    out[n++] = temp;
    iss.get();
  }
  return n;
}

}  // namespace PSI
