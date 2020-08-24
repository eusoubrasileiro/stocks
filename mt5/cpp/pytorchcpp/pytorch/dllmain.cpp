#pragma once
#include "libpytorch.h"


std::ofstream debugfile("pytorchcpp.txt");
// have only used once or twice for debugging
std::mutex m; // to ensure Metatrader only calls on GPU

c10::TensorOptions dtype32_option = th::TensorOptions().dtype(th::kFloat32).requires_grad(false);
c10::TensorOptions dtype64_option = th::TensorOptions().dtype(th::kFloat64).requires_grad(false);
c10::TensorOptions dtypeL_option = th::TensorOptions().dtype(th::kLong).requires_grad(false);
c10::TensorOptions dtype_option = dtype32_option;

th::Device deviceCPU = th::Device(th::kCPU);
th::Device deviceifGPU = deviceCPU;


BOOL APIENTRY DllMain(HMODULE hModule,
    DWORD  ul_reason_for_call,
    LPVOID lpReserved
)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        if (th::cuda::is_available()) {
            //  debugfile << "CUDA is available! Running on GPU." << std::endl;
            deviceifGPU = th::Device(th::kCUDA);
        }
        //debugfile << "process attach" << std::endl;
        break;
    case DLL_PROCESS_DETACH:
        // detach from process
        //debugfile << "process deattach" << std::endl;
        break;
    case DLL_THREAD_ATTACH:
        // attach from thread
        //debugfile << "thread  attach" << std::endl;
        break;
    case DLL_THREAD_DETACH:
        // detach from thread
        //debugfile << "thread  deattach" << std::endl;
        break;
    }
    return TRUE; // succesful
}
