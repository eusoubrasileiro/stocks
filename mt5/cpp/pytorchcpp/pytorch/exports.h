#ifndef EXPORTS_H
#define EXPORTS_H

#define NOMINMAX

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files
#include <windows.h>
#include <iostream>


/*  To use this exported function of dll, include this header
 *  in your project.
 */

#ifdef BUILDING_DLL
#define DLL_EXPORT __declspec(dllexport)
#else
#define DLL_EXPORT __declspec(dllimport)
#endif

extern "C"
{
	int DLL_EXPORT FracDifApply(double signal[], int size, double output[]);
	void DLL_EXPORT setFracDifCoefs(double d, int size);
}

#endif //EXPORTS_H
