#pragma warning (disable : 4146)
#include <iostream>
#include <torch\torch.h>
#define PYTORCHCPP_DLL
#include "pytorchcpp.h"

auto double_option = torch::TensorOptions().dtype(torch::kFloat64);
torch::Tensor thfdcoefs;
torch::Device deviceCPU = torch::Device(torch::kCPU);
torch::Device device = deviceCPU;

// calculate the fractdif coeficients to be used next
// $$ \omega_{k} = -\omega_{k-1}\frac{d-k+1}{k} $$
// output is allocated inside to a pytorch tensor
// on current device
void setFracDifCoefs(double d, int size){
    double *w = new double[size];
    w[0] = 1.;
    for(int k=1; k<size; k++)
        w[k]=-w[k-1]/k*(d-k+1);
    std::reverse(w, w+size);
    thfdcoefs = torch::from_blob(w, {1, 1, size}, double_option);
	// to GPU or not
    thfdcoefs.to(device);
}

// apply fracdif filter on signal array
// FracDifCoefs must be supplied
// output is allocated inside
int FracDifApply(double signal[], int size, double output[]){
  torch::Tensor thdata = torch::from_blob(signal, { 1, 1, size}, double_option).clone();
  // to GPU or not
  thdata.to(device);
  torch::Tensor thresult = torch::conv1d(thdata, thfdcoefs).reshape({ -1 });
  // back to CPU
  thresult = thresult.to(deviceCPU);
  // double array output
  double* ptr_data = thresult.data_ptr<double>();
  int outsize = thresult.size(0);
  memcpy(output, ptr_data, sizeof(double) * outsize);

  return outsize;
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
        case DLL_PROCESS_ATTACH:
            if (torch::cuda::is_available()) {
              //std::cout << "CUDA is available! Running on GPU." << std::endl;
              device = torch::Device(torch::kCUDA);
            }
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
