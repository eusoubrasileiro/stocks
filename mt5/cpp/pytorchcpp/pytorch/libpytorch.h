#pragma once
#include "pytorchcpp.h"
#include <torch\torch.h>
#include <torch\cuda.h>

#include <fstream> // debugging dll load by metatrader 5 output to txt file -> located where it started
#include <mutex>
#define GIGABytes 1073741824.0

namespace th = torch;

extern std::ofstream debugfile;
// have only used once or twice for debugging
extern std::mutex m; // to ensure Metatrader only calls on GPU

extern c10::TensorOptions dtype32_option;
extern c10::TensorOptions dtype64_option;
extern c10::TensorOptions dtypeL_option;
extern c10::TensorOptions dtype_option;

extern th::Device deviceCPU;
extern th::Device deviceifGPU;