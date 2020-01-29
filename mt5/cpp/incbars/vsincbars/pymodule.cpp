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
    // is loaded the DllMain will be called

    // pybind11.readthedocs.io/en/stable/advanced/pycpp/numpy.html#structured-types
    PYBIND11_NUMPY_DTYPE(tm, tm_sec, tm_min, tm_hour, tm_mday, tm_mon, tm_year, tm_wday, tm_yday, tm_isdst);
    PYBIND11_NUMPY_DTYPE(MqlTick, time, bid, ask, last, volume, time_msc, flags, volume_real);
    PYBIND11_NUMPY_DTYPE(MoneyBar, avgprice, nticks, time, smsc, emsc, uid, dtp, askh, askl, bidh, bidl, netvol);
    PYBIND11_NUMPY_DTYPE(LbSignal, twhen, band, sign, y, ninc, tdone, ret, inf_start, inf_end);

    // cannot instanciate this as global, will start before the interpreter -> then boom $*(@&(
    ppymbars.reset(new py::array_t<MoneyBar>(BUFFERSIZE));

    // optional module docstring
    m.doc() = "incbars metatrader 5 expert - python api pybind11";

    m.def("initialize", &CppExpertInit, "Initialize Increase Bands Expert",
        py::arg("nbands"), py::arg("bbwindow"), py::arg("stddevs"), py::arg("batch_size"), py::arg("ntraining"),
        py::arg("start_hour"), py::arg("end_hour"), py::arg("expire_hour"),
        py::arg("ordersize"), py::arg("stoploss"), py::arg("targetprofit"), py::arg("incmax"),
        py::arg("min_lots"), py::arg("ticksize"), py::arg("tickvalue"),
        py::arg("moneybarsize"), // R$ to form 1 money bar
        // ticks control
        py::arg("isbacktest"), py::arg("cs_symbol"), py::arg("mt5_timenow"),
        py::arg("mt5_debug"));

    // signature with py::array_t for AddTicks
    m.def("addticks", &pyAddTicks, "send ticks to the expert",
        py::arg("nprecords_mqltick"));

    m.def("refresh", &CppRefresh, "update everything with newbars, after addticks > 0");

    m.def("buffersize", &BufferSize, "size of all buffers");

    m.def("buffertotal", &BufferTotal, "count of bars or all buffers data");

    m.def("newdataidx", &NewDataIdx, "start index on buffers of new bars after AddTicks > 0");

    m.def("moneybars", &pyGetMoneyBars, "get MoneyBar buffer as is");

    m.def("createxyvectors", &CreateXyVectors, "label b.signals and fill out X & y pair vectors");

    m.def("getxyvectors", &pyGetXyvectors, "get X feature vectors, y class labels and uid index for money bars");

    m.def("xdim", &pyGetXdim, "get x feature vector dimension");

    m.def("minbarsxvector", &MinPrevBars, "number of previous bars needed to form one X vector");

    m.def("adfuller", &pyadfuller, "augmented dickey fuller test - return statistic");

    m.def("thsadf", &thsadf, "supremum augmented dickey fuller test torch GPU");

    //m.def("unload", &unloadModule, "unload incbars"); - breaks python interpreter
}
