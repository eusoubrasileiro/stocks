#pragma once
#include "time.h"

#pragma pack(push, 2)
struct bsignal { // bollinger band signal
    int   bfidx; // when happened buffer index - temporary
    int   band; // which band
    int   sign; // what sign
};
#pragma pack(pop)

// single element needed for training
struct XyPair
{
    uint64_t tidx; // when that happend - 'time' idx
    // used to assembly X feature vector
    int y; // y label value
    double *X;
};


