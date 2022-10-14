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
    std::cout << dst << std::endl;
    return 0;
}

int GetRandom(int length, unsigned char * dst) {
  std::random_device rd;
  std::default_random_engine eng(rd());
  std::uniform_int_distribution<int> distr(0, 256);

  for (int n = 0; n < length; n++) {
    dst[n] = distr(eng);
  }

  return 0;
}

int GetRandomUint32(int length, ui32* dst) {
  unsigned char temp[4*length];
  GetRandom(4*length, temp);

  for (int i = 0; i < length; i++) {
    dst[i] = (temp[4*i] << 24) +
            (temp[4*i+1] << 16) +
            temp[4*i+2] << 8 +
            temp[4*i+3];
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


}  // namespace PSI
