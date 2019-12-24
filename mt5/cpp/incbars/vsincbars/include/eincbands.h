#pragma once


#define             Expert_Fracdif           0.6         // fraction for fractional difference
#define             Expert_Fracdif_Window    512         // window size fraction fracdif
#define             Expert_Delay             8000        // delay to perform an order sucessfully after evaluating an entry (ms)

// the grace of x64 arch
// for MQL5 or even C++ call - thankfully __stdcall is useless in x64 arch
// so will not use __stdcall specifier since Python is also x64
// extern "C" guarantes no decoration is inserted on the function name

#define NOMINMAX

#ifdef BUILDING_DLL
#define EXPORT
#define DLL_EXPORT extern "C" __declspec(dllexport)

#include "time.h"
#include "bars.h"
#include "xypair.h"

#else // Google Tests only 
#define IMPORT 
#define DLL_EXPORT extern "C" __declspec(dllimport)

#endif

#include <tuple>
#include "pybind11/embed.h"
#include "pybind11/pybind11.h"
#include "pybind11/numpy.h"
#include "pybind11/stl.h"
#include "pybind11/functional.h"
#include "cwindicators.h"
#include "ticks.h"
#include "callpython.h"

namespace py = pybind11;



DLL_EXPORT void CppExpertInit(int nbands, int bbwindow, double devs, int batch_size, int ntraining,
    double start_hour, double end_hour, double expire_hour,
    double ordersize, double stoploss, double targetprofit, int incmax,
    double lotsmin, double ticksize, double tickvalue,
    double moneybar_size,  // R$ to form 1 money bar
    // ticks control
    bool isbacktest, char *cs_symbol, int64_t mt5_timenow,
    short mt5debug=0); // metatrader debugging level



// called by mt5 after 
// m_ncopied = CopyTicksRange(m_symbol, m_copied_ticks,
// COPY_TICKS_ALL, m_cmpbegin_time, 0)
// passing m_copied_ticks as *mt5_pticks
// returns the next cmpbegin_time
DLL_EXPORT int64_t CppOnTicks(MqlTick *mt5_pticks, int mt5_nticks);
// int64_t same as long for Mql5

// update everything with new bars that just arrived
// AddTicks > 0
// if sucessfull update creating new data on all
// indicators and signal features return true
DLL_EXPORT bool CppRefresh(void);

// Global Buffer Size (all buffers MUST and HAVE same size)
// that's why they all use same SetSize for setting
// Expert_BufferSize
// All are updated after bars so that's the main
// begin of new bar is also begin of new data on all buffers
DLL_EXPORT size_t BufferSize(); // size of all buffers
DLL_EXPORT size_t BufferTotal(); // count of bars or all buffers data
DLL_EXPORT size_t NewDataIdx(); // start index on all buffers of new bars after CppOnTicks & NewData == true
DLL_EXPORT size_t ValidDataIdx(); // start index of valid data on all buffers (indicators) any time

DLL_EXPORT bool CppNewData(); // are there any new bars after call of OnTicks

DLL_EXPORT BufferMqlTicks *GetTicks(); // get m_ticks variable

DLL_EXPORT void TicksToFile(char *cs_filename);

DLL_EXPORT bool isInsideFile(char *cs_filename);

// classes and features
DLL_EXPORT void CreateXyVectors();

// API Python - only for gtests
DLL_EXPORT int64_t pyAddTicks(py::array_t<MqlTick> ticks);

// returns -1/0/1 Sell/Nothing/Buy 
DLL_EXPORT int isSellBuy();

#ifdef EXPORT

py::array_t<MoneyBar> pyGetMoneyBars();

// index offset correction between uid and current buffer indexes
inline void updateBufferUidOffset();


// verify a signal in any band
// on the last m_added bars
// get the latest signal
// return index on buffer of existent entry signal
// only if more than one signal
// all in the same direction
// otherwise returns -1
// only call when there's at least one signal
// stored in the buffer of raw signal bands
int lastRawSignals();

void CreateXyVectors();
int LabelSignal(std::list<BSignal>::iterator current, std::list<BSignal>::iterator end, XyPair& xy);
bool FillInXFeatures(XyPair &xypair);

std::tuple<py::array, py::array, py::array_t<LbSignal>> pyGetXyvectors();
int pyGetXdim();

// sklearn model
bool PythonTrainModel();
int  PythonPredict(XyPair xypair);

#endif
