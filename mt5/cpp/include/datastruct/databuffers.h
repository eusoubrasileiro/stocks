#pragma once
#include <fstream>
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

#include <string>
#include <iostream>
#include <time.h>
#include "cwindicators.h"

DLL_EXPORT int64_t CppOnTicks(MqlTick* mt5_pticks, int mt5_nticks, double *lost_ticks);

DLL_EXPORT void CppTicksToFile(char* cs_filename);

DLL_EXPORT bool CppisInsideFile(char* cs_filename);

DLL_EXPORT void CppDataBuffersInit(
    double ticksize,
    double tickvalue,
    double moneybar_size,  // R$ to form 1 money bar
    char*  cs_symbol);     // cs_symbol is a char[] null terminated string (0) value at end