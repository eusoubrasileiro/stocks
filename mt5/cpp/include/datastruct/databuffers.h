#pragma once
#include <fstream>
#include <string>
#include <iostream>
#include <time.h>
#include "cwindicators.h"
#include "moneybars.h"

#ifdef DEBUG
extern std::ofstream debugfile;
#else
#define debugfile std::cout
#endif


// from databuffers.h/cpp
// buffer of money bars base for everything
extern std::shared_ptr<MoneyBarBuffer> m_bars;

int64_t OnTicks(MqlTick* mt5_pticks, int mt5_nticks, double *lost_ticks);

void DataBuffersInit(
    double ticksize,
    double tickvalue,
    double moneybar_size,  // R$ to form 1 money bar
    char*  cs_symbol);     // cs_symbol is a char[] null terminated string (0) value at end

BufferMqlTicks* GetBufferMqlTicks();

void TicksToFile(char* cs_filename);

bool isInsideFile(char* cs_filename);

size_t BufferSize();

size_t BufferTotal();