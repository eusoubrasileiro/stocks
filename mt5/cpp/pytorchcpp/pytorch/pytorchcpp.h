#ifndef PYTORCHCPP_EXPORTS_H
#define PYTORCHCPP_EXPORTS_H

#ifdef PYTORCHCPP_DLL
#define DLL_EXPORT __declspec(dllexport)
#else
#define DLL_EXPORT __declspec(dllimport)
#endif

#define NOMINMAX
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
    DLL_EXPORT void sadf(float* signal, int n, int maxw, int minw, int p, float gpumem_gb, bool verbose);
}

#endif //PYTORCHCPP_EXPORTS_H
