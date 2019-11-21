//
//
// This is:
// 1. __stdcall windows dll for generic porpouses
// 2. a __cdecl x64 Python module can be imported as `import incbars`
// means you need rename from .dll to .pyd
//
//
// Here just the python module code using pybind11
//
//

// The DLL will only build if DEBUG preprocessor is not
// set pybind11 only works in release mode
// and another code in the project should also be in release 

#pragma once

#define BUILDING_DLL
#include "pybind11/embed.h"
#include "pybind11/pybind11.h"
#include "pybind11/numpy.h"
#include "dll.h"
#include "eincbands.h"

namespace py = pybind11;

// for Python use  __cdecl
PYBIND11_MODULE(incbars, m) {

    // dont need to init anything since when the python module (this dll)
    // is loaded the DllMain will be called? are you sure?
    DllMain(0, DLL_PROCESS_ATTACH, 0); // just to make sure

    // optional module docstring
    m.doc() = "pybind11 incbars api - metatrader 5 expert";

    m.def("initialize", &Initialize, "Initialize Increase Bands Expert",
        py::arg("nbands"), py::arg("bbwindow"), py::arg("batch_size"),
        py::arg("ntraining"), py::arg("ordersize"), py::arg("stoploss"),
        py::arg("targetprofit"), py::arg("run_stoploss"), py::arg("run_targetprofit"),
        py::arg("recursive"), py::arg("ticksize"), py::arg("tickvalue"),
        py::arg("max_positions"));
    
    // pybind11.readthedocs.io/en/stable/advanced/pycpp/numpy.html#structured-types
    PYBIND11_NUMPY_DTYPE(MqlTick, time, bid, ask, last, volume, time_msc, flags, volume_real);

    // create signature with py::array_t for AddTicks
    m.def("addticks", &AddTicks, "send ticks to the expert");
    //    MqlTick * cticks, int size);
}
