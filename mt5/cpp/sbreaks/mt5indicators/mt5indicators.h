#pragma once
// dllmain.cpp : Defines the entry point for the DLL application.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <algorithm>
#include <cmath>
#include "indicators.h"

// Exports only to Metatrader 5 __stdcall is on compiler options flag  /Gz
// but doesnt matter on x64 OS only extern C to remove  c++ name mangling

#ifdef BUILDING_DLL
#define DLL_EXPORT extern "C" __declspec(dllexport)
#else
#define DLL_EXPORT extern "C" __declspec(dllimport)
#endif

// replace float/double nans to DBL_EMPTY_VALUE Metatrader 5 not plotting
#define MT5_NAN_REPLACE(type, value) ( (std::isnan<type>(value))? DBL_NAN_MT5 : value ) 


DLL_EXPORT void CppIndicatorsInit(int maxwindow,
                                        int minwindow,
                                        int order,
                                        bool usedrift,
                                        int maxbars,
                                        int numcolors);

DLL_EXPORT void CppGetSADFWindows(int* minwin, int* maxwin);

DLL_EXPORT int CppMoneyBarMt5Indicator(double* mt5_ptO, double* mt5_ptH, double* mt5_ptL, double* mt5_ptC,
    double* mt5_ptM, unixtime* mt5_petimes, double* mt5_bearbull, int mt5ptsize);

DLL_EXPORT int CppSADFMoneyBars(double* mt5_SADFline, double* mt5_SADFdots, double* mt5_imaxadfcolor,
    double* mt5_imaxadflast, int mt5ptsize);

DLL_EXPORT unixtime_ms CppOnTicks(MqlTick* mt5_pticks, int mt5_nticks, double* lost_ticks);

DLL_EXPORT void CppDataBuffersInit(
                    double ticksize,
                    double tickvalue,
                    double moneybar_size,  // R$ to form 1 money bar
                    char* cs_symbol,
                    int maxbars);     // cs_symbol is a char[] null terminated string (0) value at end