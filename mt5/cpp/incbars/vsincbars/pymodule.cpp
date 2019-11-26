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

// The DLL will only build if DEBUG preprocessor is not set.
// pybind11 only works in release mode
// and another code in the project should also be in release 

#pragma once

#define BUILDING_DLL
#include "dll.h"
#include "eincbands.h"

extern std::shared_ptr<py::array_t<MoneyBar>> ppymbars;

// for Python use  __cdecl
PYBIND11_MODULE(incbars, m) {

    // dont need to init anything since when the python module (this dll)
    // is loaded the DllMain will be called? are you sure?
    DllMain(0, DLL_PROCESS_ATTACH, 0); // just to make sure

    calledbyPython = true; // makes DllMain finalize_interpreter when DLL_PROCESS_DETACH

    // pybind11.readthedocs.io/en/stable/advanced/pycpp/numpy.html#structured-types
    PYBIND11_NUMPY_DTYPE(MqlTick, time, bid, ask, last, volume, time_msc, flags, volume_real);
    PYBIND11_NUMPY_DTYPE(MoneyBar, last, time_msc);

    // cannot instanciate this as global, will start before the interpreter -> then boom $*(@&(
    ppymbars.reset(new py::array_t<MoneyBar>(Expert_BufferSize));

    // optional module docstring
    m.doc() = "pybind11 incbars api - metatrader 5 expert";

    m.def("initialize", &Initialize, "Initialize Increase Bands Expert",
        py::arg("nbands"), py::arg("bbwindow"), py::arg("batch_size"),
        py::arg("ntraining"), py::arg("ordersize"), py::arg("stoploss"),
        py::arg("targetprofit"), py::arg("run_stoploss"), py::arg("run_targetprofit"),
        py::arg("recursive"), py::arg("ticksize"), py::arg("tickvalue"), py::arg("moneybarsize"),
        py::arg("max_positions"));  

    // signature with py::array_t for AddTicks
    m.def("addticks", &pyAddTicks, "send ticks to the expert", 
        py::arg("nprecords_mqltick"));

    m.def("refresh", &Refresh, "update everything with newbars, after addticks > 0");

    m.def("buffersize", &BufferSize, "size of all buffers");
    
    m.def("buffertotal", &BufferTotal, "count of bars or all buffers data");

    m.def("idxnewdata", &IdxNewData, "start index on buffers of new bars after AddTicks > 0");
  
    m.def("moneybars", &pyGetMoneyBars, "get MoneyBar buffer as is");
}

