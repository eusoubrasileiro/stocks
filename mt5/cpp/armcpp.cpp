#include "exports.h"
#include <iostream>
#include <armadillo>

using namespace std;
using namespace arma;

// a sample exported function
int DLL_EXPORT __stdcall Unique(double arr[], int n)
{
    vec v = vec(arr, n); // copy the same memory
    vec u = unique(v);
    double *uarr = u.memptr();
    //v.colptr(0)
    //uvec h1 = hist(v, 11);
    //uvec h2 = hist(v, linspace<vec>(-2,2,11));
    for(unsigned int i=0; i<u.n_elem; i++)
        arr[i] = uarr[i];

    return u.n_elem;
}


extern "C" DLL_EXPORT BOOL APIENTRY DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
            // attach to process
            // return FALSE to fail DLL load
            break;

        case DLL_PROCESS_DETACH:
            // detach from process
            break;

        case DLL_THREAD_ATTACH:
            // attach to thread
            break;

        case DLL_THREAD_DETACH:
            // detach from thread
            break;
    }
    return TRUE; // succesful
}
