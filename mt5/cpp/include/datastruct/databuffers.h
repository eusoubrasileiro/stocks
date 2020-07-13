#pragma once
#include <fstream>
#include <string>
#include <iostream>
#include <time.h>
#include "cwindicators.h"
#include "moneybars.h"

#ifdef BUILDING_DLL
#define EXPORT
#define DLL_EXPORT extern "C" __declspec(dllexport)

#else // Google Tests only
#define IMPORT
#define DLL_EXPORT extern "C" __declspec(dllimport)
#endif

#ifdef DEBUG
extern std::ofstream debugfile;
#else
#define debugfile std::cout
#endif


// from databuffers.h/cpp
// buffer of money bars base for everything
extern std::shared_ptr<MoneyBarBuffer> m_bars;

DLL_EXPORT int64_t CppOnTicks(MqlTick* mt5_pticks, int mt5_nticks, double *lost_ticks);

DLL_EXPORT void CppDataBuffersInit(
    double ticksize,
    double tickvalue,
    double moneybar_size,  // R$ to form 1 money bar
    char*  cs_symbol);     // cs_symbol is a char[] null terminated string (0) value at end

DLL_EXPORT BufferMqlTicks* GetBufferMqlTicks();

DLL_EXPORT void TicksToFile(char* cs_filename);

DLL_EXPORT bool isInsideFile(char* cs_filename);