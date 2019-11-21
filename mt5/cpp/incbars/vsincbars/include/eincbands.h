#pragma once
#ifdef BUILDING_DLL
#define EXPORT 
#define DLL_EXPORT extern "C" __declspec(dllexport)

#include "ticks.h"
#include "time.h"
#include "bars.h"
#include "cwindicators.h"
#include "xypair.h"

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
    double ticksize, double tickvalue,
    int max_positions);

// will be called every < 1 second
// by Python or Metatrader 5
DLL_EXPORT void AddTicks(MqlTick* cticks, int size);


#ifdef EXPORT

void CreateBBands();

void CreateOtherFeatureIndicators();

// Global Buffer Size (all buffers MUST and HAVE same size)
// that's why they all use same SetSize for setting
// Expert_BufferSize 
// All are updated after bars so that's the main
// begin of new bar is also begin of new data on all buffers
int BufferSize();
int BufferTotal();
int BeginNewData();

// start index and count of new bars that just arrived
bool Refresh();

void RefreshRawBandSignals(double last[], int count, int empty);

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
//void LabelClasses();
//void BandCreateYTargetClasses(CCBuffer<int> bandsg_raw,
//    CBuffer<XyPair> xypairs, int band_number);
//void CreateXFeatureVectors(CBuffer<XyPair> xypairs);
//bool CreateXFeatureVector(XyPair& xypair);

//bool PythonTrainModel();
//int  PythonPredict(XyPair& xypair);

#endif