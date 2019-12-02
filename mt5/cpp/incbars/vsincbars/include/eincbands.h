#pragma once

//const int                Expert_MaxFeatures = 100;     // max features allowed - not used
//const double             Expert_MoneyBar_Size = 100e3; // R$ to form 1 money bar


#define             Expert_Fracdif           0.6         // fraction for fractional difference
#define             Expert_Fracdif_Window    512         // window size fraction fracdif
#define             Expert_Delay             8000        // delay to perform an order sucessfully after evaluating an entry (ms)


#ifdef BUILDING_DLL
#define EXPORT
#define DLL_EXPORT extern "C" __declspec(dllexport)

#include "ticks.h"
#include "time.h"
#include "bars.h"
#include "cwindicators.h"
#include "xypair.h"

#include "pybind11/embed.h"
#include "pybind11/pybind11.h"
#include "pybind11/numpy.h"

namespace py = pybind11;

#else
#define IMPORT
#define DLL_EXPORT extern "C" __declspec(dllimport)

#include "ticks.h"

#endif

// for MQL5 or even C++ call - thankfully __stdcall is useless in x64 arch
// so will not use __stdcall specifier since Python is also x64
// extern "C" guarantes no decoration is inserted on the function name

DLL_EXPORT void Initialize(int nbands, int bbwindow,
    int batch_size, int ntraining,
    double ordersize, double stoploss, double targetprofit,
    double run_stoploss, double run_targetprofit, bool recursive,
    double ticksize, double tickvalue, double moneybar_size, // R$ to form 1 money bar
    double lotsmin, int max_positions);

// will be called every < 1 second
// by Metatrader 5
DLL_EXPORT int AddTicks(const MqlTick *cticks, int size);

// update everything with new bars that just arrived
// AddTicks > 0
// if sucessfull update creating new data on all
// indicators and signal features return true
DLL_EXPORT bool Refresh(void);

// Global Buffer Size (all buffers MUST and HAVE same size)
// that's why they all use same SetSize for setting
// Expert_BufferSize
// All are updated after bars so that's the main
// begin of new bar is also begin of new data on all buffers
DLL_EXPORT int BufferSize(); // size of all buffers
DLL_EXPORT int BufferTotal(); // count of bars or all buffers data
DLL_EXPORT int NewDataIdx(); // start index on all buffers of new bars after AddTicks > 0


#ifdef EXPORT

// by Python
int pyAddTicks(py::array_t<MqlTick> ticks);

py::array_t<MoneyBar> pyGetMoneyBars();


void verifyEntry();

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

// sklearn model
// classes and features
void LabelClasses();



//void CreateXFeatureVectors(CBuffer<XyPair> xypairs);
//bool CreateXFeatureVector(XyPair& xypair);

//bool PythonTrainModel();
//int  PythonPredict(XyPair& xypair);

#endif