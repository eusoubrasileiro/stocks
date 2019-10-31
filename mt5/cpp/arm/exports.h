#ifndef __MAIN_H__
#define __MAIN_H__

#define NOMINMAX

#include <windows.h>

/*  To use this exported function of dll, include this header
 *  in your project.
 */

#ifdef BUILDING_DLL
    #define DLL_EXPORT __declspec(dllexport)
#else
    #define DLL_EXPORT __declspec(dllimport)
#endif


#ifdef __cplusplus
extern "C"
{
#endif

int DLL_EXPORT Unique(double arr[], int n);

void DLL_EXPORT Histsma(double data[], int n, double bins[], int nb, int nwindow);

#ifdef __cplusplus
}
#endif

#endif // __MAIN_H__
