#ifndef CTALIB_EXPORTS_H
#define CTALIB_EXPORTS_H

#define NOMINMAX

#ifdef CTALIB_DLL
#define DLL_EXPORT __declspec(dllexport)
#else
#define DLL_EXPORT __declspec(dllimport)
#endif

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
 // Windows Header Files
#include <windows.h>
#include <iostream>

// warning do not mix c code static library with c++ of armadillo (c++17) in a dll
// lost hours trying to make a mixed c++/c dll

extern "C" // removes decoration of functions __stdcall would mess up but in x64 it is ignored
{

    DLL_EXPORT int __stdcall taMA(int  startIdx, // start the calculation at index
        int    endIdx, // end the calculation at index
        const double inReal[],
        int   optInTimePeriod, // From 1 to 100000  - EMA window
        int   optInMAType,
        double        outReal[]);


    DLL_EXPORT int __stdcall taSTDDEV(int  startIdx, // start the calculation at index
        int    endIdx, // end the calculation at index
        const double inReal[],
        int           optInTimePeriod, // From 1 to 100000  - EMA window
        double        outReal[]);


    DLL_EXPORT int __stdcall taBBANDS(int  startIdx, // start the calculation at index
        int    endIdx, // end the calculation at index
        const double inReal[],
        int      optInTimePeriod, // From 1 to 100000  - MA window
        double   optInNbDev,
        int      optInMAType, // MA type
        double outRealUpperBand[],
        double outRealMiddleBand[],
        double outRealLowerBand[]);

};

#endif //CTALIB_EXPORTS_H
