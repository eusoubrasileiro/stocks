#pragma warning (disable : 4146)
#include <iostream>
#include <torch\torch.h>

using std;



// Simple Cross-Correlation only valid samples using Pytorch-Cpp
// that's what conv1d does
int main() {
  	// Create the device we pass around based on whether CUDA is available.
    // my case you need to pass a double/float array
    double array[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
    // clone is necessary since from_blob create a tensor that doesnt own the data
    // so to(device) would not work
    auto options = torch::TensorOptions().dtype(torch::kFloat64);
    torch::Tensor thdata = torch::from_blob(array, { 1, 1, 10 }, options).clone();
    thdata = thdata.toType(torch::kFloat); // convert to float to use conv
    thdata.to(device);
    torch::Tensor thfilter = torch::ones({ 1, 1, 3 }, device); // float by default

    //torch::Tensor indata = new torch::Tensor();

    std::cout << thdata << std::endl;
    std::cout << torch::conv1d(thdata, thfilter) << std::endl;

	return 0;
}




// calculate the fractdif coeficients to be used next
// $$ \omega_{k} = -\omega_{k-1}\frac{d-k+1}{k} $$
// output is allocated inside to a pytorch tensor
// on current device
void setFracDifCoefs(double d, int size){
    float *w = new float [size];
    w[0] = 1.;
    for(int k=1; k<size; k++)
        w[k]=-w[k-1]/k*(d-k+1);
    std::reverse(w, w+size);
    thfdcoefs = torch::from_blob(w, {1, 1, size}, options).clone();
    thfdcoefs.to(device);
}

// apply fracdif filter on signal array
// FracDifCoefs must be supplied
// output is allocated inside
int FracDifApply(double signal[], int size, double output[]){
  torch::Tensor thdata = torch::from_blob(signal, { 1, 1, size}, options).clone();
  thdata = thdata.toType(torch::kFloat); // convert to float to use conv
  thdata.to(device);
  torch::Tensor result = torch::conv1d(thdata, thfdcoefs);
  
  result.data_ptr<double>()

  //return outsize;
}

auto options = torch::TensorOptions().dtype(torch::kFloat64);
torch::Tensor thfdcoefs;
torch::Device device;

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
        case DLL_PROCESS_ATTACH:
            device = torch::Device(torch::kCPU);
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
