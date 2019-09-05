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
    for(unsigned int i=0; i<u.n_elem; i++)
        arr[i] = uarr[i];
    return u.n_elem;
}


// histogram of counts smoothed by sma
// // M = floor((end-start)/delta), so that (start + M*delta) ≤ end >> (vmax-vmin+2binsize)/binsize
// bins array gets replaced by counts on each bin smoothed by SMA of size window
void DLL_EXPORT Histsma(double data[], int n, double bins[], int nb, int window){
    vec vec_data = vec(data, n);  // copy the memory
    uvec uvec_hist = hist(vec_data, vec(bins, nb));
    vec vec_hist(uvec_hist.n_elem);
    copy(uvec_hist.begin(), uvec_hist.end(), vec_hist.begin()); // convert to double
    // centered simple moving average
    vec _histsma = conv(vec_hist, (1./window) * ones<vec>(window), "same");
    // back to array
    copy(_histsma.begin(), _histsma.end(), bins);
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


extern "C" DLL_EXPORT int APIENTRY DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
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
