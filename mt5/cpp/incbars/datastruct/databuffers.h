#pragma once

#include "bars.h"

#ifdef BUILDING_DLL
#define EXPORT
#define DLL_EXPORT extern "C" __declspec(dllexport)

#else // Google Tests only
#define IMPORT
#define DLL_EXPORT extern "C" __declspec(dllimport)

#endif

#include <string>
#include <iostream>
#include <time.h>

DLL_EXPORT void CppDataBuffersInit(double ticksize, double tickvalue,
    double moneybar_size,  // R$ to form 1 money bar
    bool isbacktest, // ticks control, for backtesting and debug
    char* cs_symbol,  // cs_symbol is a char[] null terminated string (0) value at end
    int64_t mt5_timenow);

DLL_EXPORT MoneyBarBuffer* CppGetMoneyBars();

DLL_EXPORT BufferMqlTicks* CppGetTicks(); // get m_ticks variable

DLL_EXPORT int64_t CppOnTicks(MqlTick* mt5_pticks, int mt5_nticks);

DLL_EXPORT void CppTicksToFile(char* cs_filename);

DLL_EXPORT bool CppisInsideFile(char* cs_filename);

DLL_EXPORT int CppMoneyBarMt5Indicator(double* mt5_ptO, double* mt5_ptH, double* mt5_ptL, double* mt5_ptC, double* mt5_ptM, unixtime* mt5_petimes, int mt5ptsize, int maxbars);

DLL_EXPORT bool CppNewBars(); // are there any new bars after call of CppOnTicks