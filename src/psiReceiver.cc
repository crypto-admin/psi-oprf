#include <iostream>
#include <memory>
#include "psiReceiver.h"
#include "crypto/sm2.h"


namespace PSI {
    
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
}