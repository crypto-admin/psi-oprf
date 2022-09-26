
#include "psiReceiver.h"
#include "crypto/sm2.h"

#include <iostream>
#include <unistd.h>
#include <memory>


namespace PSI {


    int param_size = 9;
    psiparams onlineparam = {1024, 1024*1024, 1024*1024, 20, 60, 32, 32, 256, 256};

    // read server params from config file, as csv, json..
int Parserparam() {
  string config;
  ifstream config_file("src/config/serverConfig.csv", ios::in);
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
    onlineparam.width = param_temp[4]; // int to bool;
    onlineparam.hashLengthInBytes = param_temp[5];
    onlineparam.h1LengthInBytes = param_temp[6];
    onlineparam.bucket1 = param_temp[7];
    onlineparam.bucket2 = param_temp[8];
  } else {
    return 2; // param size error;
  }
  
  return 0;
}

int InitData(std::string filePath, std::vector<string>& src) {
  string ele;
  ifstream srcFile(filePath, ios::in);
  if (!srcFile.is_open()) {
    std::cout << "open data file fail." << std::endl;
    return 1;
  }
  uint32_t param_temp[param_size]; // 7 is para num of pirparams
  int index = 0;
  while (getline(srcFile, ele)) {
    src.push_back(ele.c_str());
    index++;
  }
}

int GetRandom(int length, unsigned char * dst) { // 获取length字节的随机数; TODO:update speed.
  for (int i = 0; i < length ; i++) {
    dst[i] = rand() % 256;
  }
  return 0;
}

int GetRandomUint32(int length, ui32* dst) {
  
  unsigned char temp[4*length];
  GetRandom(4*length, temp);

  for (int i = 0; i < length; i++) {
    dst[i] = (temp[4*i] << 24) + (temp[4*i+1] << 16) + temp[4*i+2] << 8 + temp[4*i+3];
  }


}

// int GetRandAOT1out2(epoint p, ui32* randa) { // 外部申请空间;
//   GetRandomUint32(8, randa);
//   basepointmul(p, randa);

//   return 0;
// }

int AffinePoint2String(affpoint& point, std::string& dst) {
  for (int i = 0; i < DIG_LEN; i++) {
    dst += to_string(point.x[i]);
  }
  dst += "\n";
    for (int i = 0; i < DIG_LEN; i++) {
    dst += to_string(point.y[i]);
  }

  return 0;
}


int BatchOT(ServerReaderWriter<Point, Point>* stream, const ui32& width, ui8** k0Set, ui8** k1Set) {
  // ui32 randa[8];
  // GetRandomUint32(8, randa);
  // epoint A1;
  // affpoint A;
  // basepointmul(&A, randa);
  // std::cout << "test RandA =" << A.x << std::endl;
  // for (int i = 0; i < DIG_LEN; i++) {
  //   std::cout << A.x[i] << std::endl;
  // }
  
  std::vector<affpoint> randASet;
  std::vector<block32> scalarSet;
  ui32** AxSet;
  ui32** AySet;
  std::cout << "test2" << std::endl;

  for (int i=0; i < width; i++) {
    block32 randa;
    GetRandomUint32(8, randa.rand);
    affpoint p;
    basepointmul(&p, randa.rand);
    randASet.push_back(p);
    scalarSet.push_back(randa);
    std::string temp = "";
    AffinePoint2String(p, temp);
    Point send;
    if (temp != "") {
      send.set_pointset(temp);  
    } else {
      send.set_pointset("point");
    }
    
    stream->Write(send);
  }
  sleep(3);
  std::cout << "server ot end." << std::endl;

  return 0;
}

    
    void PsiReceiver::run(
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
                const ui32& bucket2) {
        clock_t start;
        auto heightInBytes = (height + 7) / 8;
		auto widthInBytes = (width + 7) / 8;
		auto locationInBytes = (logHeight + 7) / 8;
		auto receiverSizeInBytes = (receiverSize + 7) / 8;
		auto shift = (1 << logHeight) - 1;
		auto widthBucket1 = sizeof(block) / locationInBytes;  // 16/3 = 5   

    	///////////////////// Base OTs ///////////////////////////
    
    ui8** k0Set;
    ui8** k1Set;
		BatchOT(stream, width, k0Set, k1Set);

    }


    int PsiReceive(ServerReaderWriter<Point, Point>* stream) {
        Parserparam();
        std::vector<string> serverData;
        // string srcFilePath = "src/data/serverData.csv";
        // int res = InitData(srcFilePath, serverData);
        std::cout << "tst1" <<std::endl;
        for (int i=0; i<100; i++) {
          serverData.push_back(std::to_string(i+2));
        }

        PsiReceiver r;
        r.run(stream, \
            onlineparam.senderSize,
            onlineparam.receiverSize,
            onlineparam.height,
            onlineparam.logHeight,
            onlineparam.width,
            serverData, 
            onlineparam.hashLengthInBytes,
            onlineparam.h1LengthInBytes,
            onlineparam.bucket1, 
            onlineparam.bucket2
        );
    }
}