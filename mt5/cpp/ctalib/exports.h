#ifndef EXPORTS_H
#define EXPORTS_H

#define NOMINMAX

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
 // Windows Header Files
#include <windows.h>
#include <iostream>

#ifdef BUILDING_DLL
    #define DLL_EXPORT __declspec(dllexport)
#else
    #define DLL_EXPORT __declspec(dllimport)
#endif

// warning do not mix c code static library with c++ of armadillo (c++17) in a dll
// lost hours trying to make a mixed c++/c dll 

extern "C"
{
	
int DLL_EXPORT taMA(int  startIdx, // start the calculation at index
	                  int    endIdx, // end the calculation at index
	                  const double inReal[],
	                  int   optInTimePeriod, // From 1 to 100000  - EMA window
	                  int   optInMAType,
                    double        outReal[]);
}

#endif //EXPORTS_H
