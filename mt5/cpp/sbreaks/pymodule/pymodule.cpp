// This is:
// A __cdecl x64 Python module can be imported as `import explotest`
// means you need rename from .dll to .pyd using pybind11
//

// The DLL will only build if DEBUG preprocessor is not set.
// pybind11 only works in release mode
// and all code in the project should also be in release



////////////////////////////////////////////////
///////////////// Python API //////////////////
///////////////////////////////////////////////


#pragma once

#define BUILDING_DLL
#include <limits> // std::numeric_limits
#include "pybind11/embed.h"
#include "pybind11/pybind11.h"
#include "pybind11/numpy.h"
#include "pybind11/cast.h"
#include "indicators.h"


namespace py = pybind11;

extern std::shared_ptr<MoneyBarBuffer> m_bars; // buffer of money bars base for everything

unixtime_ms SendTicks(py::array_t<MqlTick> ticks)
{
    // make a copy to be able to use fixticks (non const)
    std::vector<MqlTick> nonconst_ticks(ticks.data(), ticks.data() + ticks.size());

    double lost_ticks;
    auto return_time = OnTicks(nonconst_ticks.data(), nonconst_ticks.size(), &lost_ticks);
    std::cout << "lost ticks %: " << lost_ticks << std::endl;
    return return_time;
}


py::array_t<MoneyBar> GetMoneyBars() {
    // this bellow also works, but too much code
    // probably faster 
    //auto array_one = m_bars->array_one();
    //auto arr1 = std::get<0>(array_one);
    //auto size1 = std::get<1>(array_one);
    //auto array_two = m_bars->array_two();
    //auto arr2 = std::get<0>(array_two);
    //auto size2 = std::get<1>(array_two);
    //std::copy(arr1, arr1 + size1, pbuf);
    //std::copy(arr2, arr2 + size2, pbuf+size1);    
    auto bars = std::vector<MoneyBar>(m_bars->begin(), m_bars->end());
    return py::array_t<MoneyBar>(bars.size(), bars.data());
}

// only for doubles needs to convert to real NANS
// std::replace(vec.begin(), vec.end(), DBL_NAN_MT5, FLT_NAN);

py::array GetSADF(){
    auto vec = std::vector<float>(m_SADFi->Begin(0), m_SADFi->End(0));
    return py::array(vec.size(), vec.data());
}

// dispersion
py::array GetSADFdisp() {
    auto vec = std::vector<float>(m_SADFi->Begin(1), m_SADFi->End(1));
    return py::array(vec.size(), vec.data());
}


py::array GetCumSum() {
    auto vec = std::vector<float>(m_CumSumi->Begin(0), m_CumSumi->End(0));
    return py::array(vec.size(), vec.data());
}

py::array GetMbReturns() {
    auto vec = std::vector<double>(m_MbReturn->Begin(0), m_MbReturn->End(0));
    return py::array(vec.size(), vec.data());
}

py::array GetStdevMbReturns() {
    auto vec = std::vector<double>(m_StdevMbReturn->Begin(0), m_StdevMbReturn->End(0));
    return py::array(vec.size(), vec.data());
}

py::array_t<Event> GetEvents() {
     return py::array_t<Event>(m_Events->size(), m_Events->data());
}

py::array_t<Event> GetLabelledEvents(double mtgt, int barrier_str, int nbarriers, int barrier_inc) {
    auto vec = LabelEvents(m_Events->begin(), m_Events->end(), mtgt, barrier_str, nbarriers, barrier_inc);
    return py::array_t<Event>(vec.size(), vec.data());
}


py::array GetFracDiff() {
    auto vec = std::vector<float>(m_FdMb->Begin(0), m_FdMb->End(0));
    return py::array(vec.size(), vec.data());
}

// name for EACH column feature on X vector
// TODO:
//std::vector<std::string> pyFeatures() {
//    return m_features;
//}


std::tuple<py::array, py::array> thsadf(py::array_t<float> data, int maxw, int minw, int p, bool usedrift, float gpumem_gb, bool verbose) {
    // make a copy to be able to use (non const)
    std::vector<float> indata(data.data(), data.data() + data.size());
    std::vector<float> outsadf, outadfmaxidx;
    auto sadf_size = indata.size() - maxw + 1;
    outsadf.resize(sadf_size);
    outadfmaxidx.resize(sadf_size);
    sadf(indata.data(), outsadf.data(), outadfmaxidx.data(), indata.size(), maxw, minw, p, usedrift, gpumem_gb, verbose);

    return std::make_tuple(py::array(sadf_size, outsadf.data()), py::array(sadf_size, outadfmaxidx.data()));
}



// for Python use  __cdecl
PYBIND11_MODULE(explotest, m){

    // dont need to init anything since when the python module (this dll)
    // is loaded the DllMain will be called

    // pybind11.readthedocs.io/en/stable/advanced/pycpp/numpy.html#structured-types
    PYBIND11_NUMPY_DTYPE(tm, tm_sec, tm_min, tm_hour, tm_mday, tm_mon, tm_year, tm_wday, tm_yday, tm_isdst);
    PYBIND11_NUMPY_DTYPE(MqlTick, time, bid, ask, last, volume, time_msc, flags, volume_real);
    PYBIND11_NUMPY_DTYPE(MoneyBar, wprice, wprice90, wprice10, nticks, time, smsc, emsc, open, high, low, highfirst, uid, dtp, netvol, inday);
    PYBIND11_NUMPY_DTYPE(Event, time, twhen, tdone, cumsum, side, vertbar, y, ret, sltp, inf_start, inf_end);

    // optional module docstring
    m.doc() = "explosiveness tests - python api pybind11";

    m.def("initmoneybars", &DataBuffersInit, "Initialize Money Bar and MqlTicks buffers",
        py::arg("ticksize"), py::arg("tickvalue"), py::arg("moneybar_size"), py::arg("symbol"),
        py::arg("start_hour"), py::arg("end_hour"));

    m.def("initindicators", &IndicatorsInit, "Initialize Indicators", 
        py::arg("maxwindow"), py::arg("minwindow"), py::arg("order"), py::arg("usedrift"),
        py::arg("cum_reset"));

    // signature with py::array_t for AddTicks
    m.def("sendticks", &SendTicks, "Send ticks to expert",
        py::arg("nprecords_mqltick"));

    m.def("buffersize", &BufferSize, "size of all buffers");

    m.def("buffertotal", &BufferTotal, "count of bars or all buffers data");

    //m.def("newdataidx", &NewDataIdx, "start index on buffers of new bars after AddTicks > 0");

    m.def("mbars", &GetMoneyBars, "get MoneyBar buffer as is");

    m.def("mbars_sadf", &GetSADF, "get SADF MoneyBar buffer as is");

    m.def("mbars_sadfd", &GetSADFdisp, "get SADF ADF max values dispersion MoneyBar buffer as is");

    m.def("mbars_sadf_csum", &GetCumSum, "get CumSum on SADF MoneyBar buffer as is");

    m.def("mbars_fdiff", &GetFracDiff, "get Fracitional Differation of Money Bars");

    m.def("mbars_returns", &GetMbReturns, "get somewhat Average Money Bar Returns");

    m.def("mbars_stdev_returns", &GetStdevMbReturns, "get volatility of Returns as standard deviation");

    //m.def("createxyvectors", &CreateXyVectors, "label b.signals and fill out X & y pair vectors");

    //m.def("getxyvectors", &pyGetXyvectors, "get X feature vectors, y class labels and uid index for money bars");

    //m.def("xdim", &pyGetXdim, "get x feature vector dimension");

    //m.def("minbarsxvector", &MinPrevBars, "number of previous bars needed to form one X vector");

    //thsadf(py::array_t<float> data, int maxw, int p, float gpumem_gb, bool verbose)
    m.def("thsadf", &thsadf, "supremum augmented dickey fuller test torch GPU",
        py::arg("data"), py::arg("maxw"), py::arg("minw"),
        py::arg("order"), py::arg("drift"), py::arg("gpumem_gb"), py::arg("verbose"));

    m.def("get_events", &GetEvents, "get cum sum on sadf money bars events");

    m.def("get_events_lbl", &GetLabelledEvents, "label events", py::arg("mult_tgt"),
        py::arg("barrier_str"), py::arg("nbarriers"), py::arg("barrier_inc"));

}
