#include <iostream>
#include <memory>
#include "psiReceiver.h"
#include "crypto/sm2.h"


namespace PSI {


    int param_size = 9;
    psiparams onlineparam = {1024, 1024*1024, 1024*1024, 20, 600, 32, 32, 256, 256};

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
		

    }


    int PsiReceive(ServerReaderWriter<Point, Point>* stream) {
        Parserparam();
        std::vector<string> serverData;
        string srcFilePath = "src/data/server.csv";
        int res = InitData(srcFilePath, serverData);

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