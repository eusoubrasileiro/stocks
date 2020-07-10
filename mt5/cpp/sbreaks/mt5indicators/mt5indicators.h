#pragma once
// dllmain.cpp : Defines the entry point for the DLL application.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <algorithm>
#include <cmath>
#include "databuffers.h"


DLL_EXPORT void CppMoneyIndicatorsInit(int maxwindow,
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