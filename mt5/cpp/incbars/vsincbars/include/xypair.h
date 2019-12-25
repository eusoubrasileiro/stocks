#pragma once
#include "time.h"

#pragma pack(push, 2)
struct BSignal { // bollinger band signal
    unixtime_ms time;
    uint64_t twhen; // when that happend - 'time' idx
    int   band; // which band
    int   sign = 0; // what sign
};
struct LbSignal // pybind11 requires POD so cannot derive from BSignal
{
    unixtime_ms time;
    uint64_t twhen; // when that happend - 'time' idx
    int   band; // which band
    int   sign = 0; // what sign
    int y = 0; // y label value
    int ninc = 0; // number of increases in position
    uint64_t tdone; // when that happend - 'time' idx of labelling
};
#pragma pack(pop)

// single element needed for training
struct XyPair : LbSignal
{
    std::vector<double> X;
};


