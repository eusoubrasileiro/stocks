#include "exports.h"
#include <iostream>
#include <armadillo>

using namespace std;
using namespace arma;

// a sample exported function
int DLL_EXPORT Unique(double arr[], int n)
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


// https://stackoverflow.com/questions/15213082/c-histogram-bin-sorting
//
// unsigned int bin;
// for (unsigned int sampleNum = 0; sampleNum < SAMPLE_COUNT; ++sampleNum)
// {
//       const int sample = data0[sampleNum];
//       bin = BIN_COUNT;
//       for (unsigned int binNum = 0; binNum < BIN_COUNT; ++binNum)  {
//             const int rightEdge = binranges[binNum];
//             if (sample <= rightEdge) {
//                bin = binNum;
//                break;
//            }
//       }
//       bins[bin]++;
//  }


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
