#pragma once
#include <fstream>
#include <string>
#include <iostream>
#include <time.h>
#include "cwindicators.h"
#include "moneybars.h"
#include "stdbars.h"

#ifdef DEBUG
extern std::ofstream debugfile;
#else
#define debugfile std::cout
#endif


// from databuffers.h/cpp
// buffer of money bars base for everything
extern std::shared_ptr<MoneyBarBuffer> m_bars;

extern std::shared_ptr<StdBarBuffer> m_tbars; // 1M bars H/L for tripple barrier labelling

int64_t OnTicks(MqlTick* mt5_pticks, int64_t mt5_nticks, double *lost_ticks);

void DataBuffersInit(
    double ticksize,
    double tickvalue,
    double moneybar_size,  // R$ to form 1 money bar
    char* cs_symbol,     // cs_symbol is a char[] null terminated string (0) value at end
    float start_hour=10.5, // valid operational window
    float end_hour=16.5);

BufferMqlTicks* GetBufferMqlTicks();

void TicksToFile(char* cs_filename);

bool isInsideFile(char* cs_filename);

size_t BufferSize();

size_t BufferTotal();