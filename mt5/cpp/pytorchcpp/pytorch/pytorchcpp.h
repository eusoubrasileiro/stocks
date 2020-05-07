#ifndef PYTORCHCPP_EXPORTS_H
#define PYTORCHCPP_EXPORTS_H

#ifdef PYTORCHCPP_DLL
#define DLL_EXPORT __declspec(dllexport)
#else
#define DLL_EXPORT __declspec(dllimport)
#endif

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers

// Windows Header Files
#include <windows.h>
#include <iostream>


/*  To use this exported function of dll, include this header
 *  in your project.
 */

extern "C" // removes decoration of functions __stdcall would mess up but in x64 it is ignored
{
    DLL_EXPORT int  __stdcall FracDifApply(double signal[], int size, double output[]);
    DLL_EXPORT void __stdcall setFracDifCoefs(double d, int size);
    DLL_EXPORT int sadf(float* signal, float* outsadf, float* lagout, int n, int maxw, int minw, int order, bool drift, float gpumem_gb, bool verbose);
    DLL_EXPORT int sadfd_mt5(double* signal, double* outsadf, double* lagout, int n, int maxw, int minw, int order, bool drift, double gpumem_gb, bool verbose);
}

#endif //PYTORCHCPP_EXPORTS_H
