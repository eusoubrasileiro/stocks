#pragma once
#include "time.h"

#pragma pack(push, 2)
struct bsignal { // bollinger band signal
    uint64_t tuidx; // when that happend - 'time' idx
    int   band; // which band
    int   sign = 0; // what sign
};
#pragma pack(pop)

// single element needed for training
struct XyPair
{
    bsignal bsg;
    bool isready = false;
    std::vector<double> X;
    int y = 0; // y label value
};


