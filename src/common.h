#pragma once

namespace PSI {


#define ui32 unsigned int
#define ui8  unsigned char
#define block unsigned char[16]

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

}