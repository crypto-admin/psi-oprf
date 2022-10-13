/*
  Copyright [2022] <Crypto-admin>
*/


#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <random>
#include "psiSender.h"


namespace PSI {

    const int param_size = 9;
    psiparams onlineparam = {
      1024,
      1024*1024,
      1024*1024,
      20,
      60,
      32,
      32,
      256,
      256
    };

int Parserparam(std::string configPath ) {
  std::string config;
  std::ifstream config_file(configPath, std::ios::in);
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
    onlineparam.width = param_temp[4];  // int to bool;
    onlineparam.hashLengthInBytes = param_temp[5];
    onlineparam.h1LengthInBytes = param_temp[6];
    onlineparam.bucket1 = param_temp[7];
    onlineparam.bucket2 = param_temp[8];
  } else {
    return 2;  // param size error;
  }
  return 0;
}

int InitData(const std::string filePath, std::vector<std::string>& src) {
  std::string ele;
  std::ifstream srcFile(filePath, std::ios::in);
  if (!srcFile.is_open()) {
    std::cout << "open data file fail." << std::endl;
    return 1;
  }
  uint32_t param_temp[param_size];  //  7 is para num of pirparams
  int index = 0;
  while (getline(srcFile, ele)) {
    src.push_back(ele.c_str());
    index++;
  }
}

int GetRandom(int length, unsigned char* dst) {
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
                std::vector<std::string> &des) {
  std::istringstream iss(src);
  std::string token;
  while (getline(iss, token, split)) {
    des.push_back(token);
  }
  return 0;
}

int Point2AffinePoint(Point src, affpoint* dst) {
  std::vector<std::string> xy;
  std::vector<std::string> x;
  std::vector<std::string> y;

  StringSplit(src.pointset(), ',', xy);
  // assert(x.size == 2);  // 2个坐标(x, y)
  StringSplit(xy[0], '|', x);
  StringSplit(xy[1], '|', y);
  for (int i = 0; i < DIG_LEN; i++) {
    dst->x[i] = atoi(x[i].c_str());
    dst->y[i] = atoi(y[i].c_str());
  }

  return 0;
}
int BatchOTSender(ClientReaderWriter<Point, Point>* stream,
  const ui32& width,
  unsigned char* choiceB) {
  Point randA;
  Point randB;
  std::vector<Point> randASet;
  std::vector<Point> randBSet;
  std::string randBSetString = "";

  auto ret = stream->Read(&randA);
  std::string randASetString = randA.pointset();
  std::cout << "batchot sender get ASet = " << randASetString << std::endl;

  std::istringstream iss(randASetString);
  std::string token;
  while (getline(iss, token, '\n')) {
    randA.set_pointset(token);
    randASet.push_back(randA);
  }

  std::cout << "clent get randASet size = " << randASet.size() << std::endl;
  for (int i=0; i < width; i++) {  // why not use batch?
    block32 randb;
    GetRandomUint32(8, randb.rand);
    affpoint A;
    affpoint B;
    basepointmul(&B, randb.rand);  // pointB
    Point2AffinePoint(randASet[i], &A);

    if (static_cast<int>(choiceB[i]) == 1) {
      pointadd(&B, &B, &A);
    }
    std::string temp = "";
    AffinePoint2String(B, &temp);
    randB.set_pointset(temp);
    randBSet.push_back(randB);
    randBSetString += temp;
  }
  Point batchB;
  batchB.set_pointset(randASetString);
  stream->Write(batchB);

  return 0;
}
void PsiSendRun(
                ClientReaderWriter<Point, Point>* stream,
                const ui32& senderSize,
                const ui32& receiverSize,
                const ui32& height,
                const ui32& logHeight,
                const ui32& width,
                std::vector<std::string>& senderSet,
                const ui32& hashLengthInBytes,
                const ui32& h1LengthInBytes,
                const ui32& bucket1,
                const ui32& bucket2) {
  clock_t start;
  auto heightInBytes = (height + 7) / 8;
  auto widthInBytes = (width + 7) / 8;
  auto locationInBytes = (logHeight + 7) / 8;
  auto receiverSizeInBytes = (receiverSize + 7) / 8;
  auto shift = (1 << logHeight) - 1;
  auto widthBucket1 = sizeof(block) / locationInBytes;  // 16/3 = 5

  ///////////////////// Base OTs ///////////////////////////
  unsigned char * choiceB = new unsigned char[width];
  GetRandom(width, choiceB);
  for (int i=0; i < width; i++) {
    choiceB[i] = static_cast<int>(choiceB[i]) % 2;
    // std::cout << "choice i = " << int(choiceB[i]) << std::endl;
  }
  auto res = BatchOTSender(stream, width, choiceB);

}

int PsiSend(ClientReaderWriter<Point, Point>* stream) {
  Parserparam("src/config/clientConfig.csv");
  std::vector<std::string> clientData;
  std::string srcFilePath = "src/data/client.csv";
  // int res = InitData(srcFilePath, clientData);
  for (int i=0; i < onlineparam.width; i++) {
    clientData.push_back(std::to_string(i+2));
  }

  // PsiSender sender;
  PsiSendRun(stream, \
      onlineparam.senderSize,
      onlineparam.receiverSize,
      onlineparam.height,
      onlineparam.logHeight,
      onlineparam.width,
      clientData,
      onlineparam.hashLengthInBytes,
      onlineparam.h1LengthInBytes,
      onlineparam.bucket1,
      onlineparam.bucket2);

  return 0;
}

}