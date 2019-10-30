#pragma warning (disable : 4146)
#include <iostream>
#include <torch\torch.h>

// static Tensor at::conv1d(const Tensor& input, const Tensor& weight, const Tensor& bias = {}, IntArrayRef stride = 1,
// IntArrayRef padding = 0, IntArrayRef dilation = 1, int64_t groups = 1)


// Simple Cross-Correlation only valid samples using Pytorch-Cpp
// that's what conv1d does
int main() {
  	// Create the device we pass around based on whether CUDA is available.
    torch::Device device(torch::kCPU);
    if (torch::cuda::is_available()) {
      std::cout << "CUDA is available! Running on GPU." << std::endl;
      device = torch::Device(torch::kCUDA);
    }

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
