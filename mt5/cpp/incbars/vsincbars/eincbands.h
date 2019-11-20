#pragma once
#include "ticks.h"
#include "time.h"
#include "bars.h"
#include "cwindicators.h"
#include "xypair.h"

const int                Expert_BufferSize = 100e3; // indicators buffer needed
const int                Expert_MaxFeatures = 100; // max features allowed - not used
const double             Expert_MoneyBar_Size = 100e3; // R$ to form 1 money bar
const double             Expert_Fracdif = 0.6; // fraction for fractional difference
const double             Expert_Fracdif_Window = 512; // window size fraction fracdif


// starts/increases positions against the trend to get profit on mean reversions
// Using Dolar/Real Bars and consequently tick data
// only for tick time-frame
// Net Mode Only!
// isInsideDay() should be checked by Metatrader prior doing 
// any openning of positions
class ExpertIncBands 
{

protected:
    // Base Data
    CCBufferMqlTicks m_ticks; // buffer of ticks w. control to download unique ones.

    // All Buffer-Derived classes bellow have indexes alligned
    // except for CObjectBuffer<XyPair>
    MoneyBarBuffer m_bars; // buffer of money bars base for everything
    std::vector<double> m_mlast; // moneybar.last values of last added money bars
    // created from ticks
    CCTimeDayBuffer m_times; // time buffer from moneybar time-ms
    //MqlDateTime m_mqldt_now;
    //MqlDateTime m_last_time; // last time after refresh

    // bollinger bands configuration
    std::vector<CTaBBANDS> m_bands; // array of indicators for each bollinger band
    // upper and down and middle
    int m_nbands; // number of bands
    int m_bbwindow; // reference size of bollinger band indicator others
    double m_bbwindow_inc;   // are multiples of m_bbwindow_inc

    // feature indicators
    CFracDiffIndicator m_fd_mbarp; // frac diff on money bar prices

    // store buy|sell|hold signals for each bband
    std::vector<CCBuffer<int>> m_raw_signal;

    std::vector<int> m_last_raw_signal; // last raw signal in all m_nbands
    int m_last_raw_signal_index; // related to m_bars buffer on refresh()

    // features and training vectors
    // array of buffers for each signal feature    
    int m_nsignal_features;
    int m_batch_size;
    int m_ntraining;
    int m_xtrain_dim; // dimension of x train vector
    CBuffer<XyPair> m_xypairs;

    // execution of positions and orders
    // profit or stop loss calculation for training
    double m_ordersize;
    double m_stoploss; // stop loss value in $$$ per order
    double m_targetprofit; //targetprofit in $$$ per order
    // profit or stop loss calculation for execution
    double m_run_stoploss; // stop loss value in $$$ per order
    double m_run_targetprofit; //targetprofit in $$$ per order
    // max number of orders
    // position being a execution of one order or increment
    // on the same direction
    // number of maximum 'positions'                             
    int m_max_positions; // maximum number of 'positions'
    int m_last_positions; // last number of 'positions'
    double m_last_volume; // last volume + (buy) or - (sell)
    double m_volume; // current volume of open or not positions

    // python sklearn model trainning
    bool m_recursive;
    //sklearnModel m_model;
    unsigned int m_model_refresh; // how frequent to update the model
    // (in number of new training samples)
    // helpers to count training samples
    unsigned long m_xypair_count; // counter to help train/re-train model
    unsigned long m_model_last_training; // referenced on m_xypair_count's

public:

    ExpertIncBands(void);

    void Initialize(int nbands, int bbwindow,
        int batch_size, int ntraining,
        double ordersize, double stoploss, double targetprofit,
        double run_stoploss, double run_targetprofit, bool recursive,
        double ticksize, double tickvalue,
        int max_positions=-1);

    void CreateBBands();

    void CreateOtherFeatureIndicators();

    // will be called every < 1 second
    // by Python or Metatrader 5
    void AddTicks(MqlTick* cticks, int size);

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


};