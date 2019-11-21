//
//
// This is:
// 1. __stdcall windows dll for generic porpouses
// 2. a __cdecl x64 Python module can be imported as `import incbars.dll`
// 
//
//
// Here just the python module code using pybind11
//
//
#pragma once

#define BUILDING_DLL
#include "pybind11/embed.h"
#include "pybind11/pybind11.h"
#include "pybind11/numpy.h"
#include "dll.h"
#include "eincbands.h"

int add(int i, int j) {
    return i + j;
}

namespace py = pybind11;

// for Python use  __cdecl
PYBIND11_MODULE(incbars, m) {

    // dont need to init anything since when the python module (this dll)
    // is loaded the DllMain will be called? are you sure?
    DllMain(0, DLL_PROCESS_ATTACH, 0); // just to make sure 

    // optional module docstring
    m.doc() = "pybind11 incbars api - metatrader 5 expert";

    m.def("initialize", &Initialize, "Initialize Increase Bands Expert");

    m.def("add", &add, "A function which adds two numbers");
}