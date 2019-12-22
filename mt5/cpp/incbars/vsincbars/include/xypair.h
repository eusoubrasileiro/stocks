#pragma once
#include "time.h"

#pragma pack(push, 2)
struct bsignal { // bollinger band signal
    uint64_t twhen; // when that happend - 'time' idx
    int   band; // which band
    int   sign = 0; // what sign
};
struct bsignal_labelled : bsignal
{
    int y = 0; // y label value
    int ninc = 0; // number of increases in position
    uint64_t tdone; // when that happend - 'time' idx of labelling
};
#pragma pack(pop)

// single element needed for training
struct XyPair : bsignal_labelled
{
    std::vector<double> X;
};


