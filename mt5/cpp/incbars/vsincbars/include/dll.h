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

// for C++/MQL5 call
	int DLL_EXPORT __stdcall Unique(double arr[], int n);
	int DLL_EXPORT __stdcall pyTrainModel(double X[], int y[], int ntraining, int xtrain_dim,
		char* model, int pymodel_size_max);
	int DLL_EXPORT __stdcall pyPredictwModel(double X[], int xtrain_dim,
		char* model, int pymodel_size);
  // for Python use  __cdecl
	

#endif //EXPORTS_H
