#define BUILDING_DLL
#include "eincbands.h"
#include "embpython.h"

const int                Expert_BufferSize = 100000; // indicators buffer needed
//const int                Expert_MaxFeatures = 100; // max features allowed - not used
//const double             Expert_MoneyBar_Size = 100e3; // R$ to form 1 money bar
const double             Expert_Fracdif = 0.6; // fraction for fractional difference
const double             Expert_Fracdif_Window = 512; // window size fraction fracdif

// I dont want to use a *.def file to export functions from a namespace
// so I am not using namespaces {namespace eincbands}


// starts/increases positions against the trend to get profit on mean reversions
// Using Dolar/Real Bars and consequently tick data
// only for tick time-frame
// Net Mode Only!
// isInsideDay() should be checked by Metatrader prior doing 
// any openning of positions   

// Base Data
//std::shared_ptr<CCBufferMqlTicks> m_ticks; // buffer of ticks w. control to download unique ones.

// All Buffer-Derived classes bellow have indexes alligned
// except for CObjectBuffer<XyPair>
std::shared_ptr<MoneyBarBuffer> m_bars; // buffer of money bars base for everything

std::vector<double> m_mlast; // moneybar.last values of last added money bars
// created from ticks
std::shared_ptr<CCTimeDayBuffer> m_times; // time buffer from moneybar time-ms
//MqlDateTime m_mqldt_now;
//MqlDateTime m_last_time; // last time after refresh

double m_moneybar_size; // R$ to form 1 money bar

// bollinger bands configuration
std::vector<std::shared_ptr<CTaBBANDS>> m_bands; // array of indicators for each bollinger band
// upper and down and middle
int m_nbands; // number of bands
int m_bbwindow; // reference size of bollinger band indicator others
double m_bbwindow_inc;   // are multiples of m_bbwindow_inc

// feature indicators
std::shared_ptr<CFracDiffIndicator> m_fd_mbarp; // frac diff on money bar prices

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


// Python API
// pass to python bars var
py::array_t<MoneyBar> pymbars(Expert_BufferSize);

void Initialize(int nbands, int bbwindow,
    int batch_size, int ntraining,
    double ordersize, double stoploss, double targetprofit,
    double run_stoploss, double run_targetprofit, bool recursive,
    double ticksize, double tickvalue, double moneybar_size, // R$ to form 1 money bar
    int max_positions)
{
    m_moneybar_size = moneybar_size;
    m_bbwindow_inc = 0.5;
    //m_model = new sklearnModel;
    m_xypair_count = 0;
    m_model_last_training = 0;
    m_last_raw_signal_index = -1;
    m_last_positions = 0; // number of 'positions' openned
    m_last_volume = 0;
    m_volume = 0;
    // set safe pointers to new objects
    m_bars.reset(new MoneyBarBuffer(tickvalue,
        ticksize, m_moneybar_size, Expert_BufferSize));
    m_times.reset(new CCTimeDayBuffer(Expert_BufferSize));

    m_recursive = recursive;
    m_nbands = nbands;
    m_bbwindow = bbwindow;
    // ordersize is in $$
    m_ordersize = ordersize;
    // for training
    m_stoploss = stoploss; // value in $$$
    m_targetprofit = targetprofit;
    // for execution
    m_run_stoploss = run_stoploss; // value in $$$
    m_run_targetprofit = run_stoploss;
    // training
    m_batch_size = batch_size;
    m_ntraining = ntraining; // minimum number of X, y training pairs
    // total of signal features
    // m_nbands band signals
    // indicar features is 1
    // 1 - fracdiff on prices
    m_nsignal_features = m_nbands + 1;
    m_xtrain_dim = m_nsignal_features * m_batch_size;

    // allocate array of bbands indicators
    m_bands.resize(m_nbands);

    // group of samples and signal vectors to train the model
    // resize buffer of training pairs
    m_xypairs.Resize(m_ntraining);
    // resize x feature vector inside it
    // but they do not exist cant resize something that doesnt exist
    // only the space available for them
    //m_model_refresh = model_refresh;

    m_raw_signal.resize(m_nbands); // one for each band
    m_last_raw_signal.resize(m_nbands);

    CreateBBands();
    CreateOtherFeatureIndicators();
}

void CreateBBands() {
    // raw signal storage
    for (int j = 0; j < m_nbands; j++) {
        m_raw_signal[j] = CCBuffer<int>(Expert_BufferSize);
    }

    // ctalib bbands
    double inc = m_bbwindow_inc;
    for (int i = 0; i < m_nbands; i++) { // create multiple increasing bollinger bands
        m_bands[i].reset(new CTaBBANDS(m_bbwindow * inc, 2.5, 1, Expert_BufferSize)); // 1-Exponential type
        inc += m_bbwindow_inc;
    }
}

void CreateOtherFeatureIndicators() {
    // only fracdiff on prices
    m_fd_mbarp.reset(new CFracDiffIndicator(Expert_Fracdif_Window, Expert_Fracdif, Expert_BufferSize));      
}

// breaking apart is better for testing and cleanner code
// and being reusable

//void AddTicks(const MqlTick* cticks, int size){
//    if (m_bars->AddTicks(cticks, size) > 0)
//    {
//        // some new ticks arrived and new bars created
//        // at least one new money bar created
//        // refresh whatever possible with new bars
//        if (Refresh()) {
//            // sucess of updating everything w. new data
//            verifyEntry();
//        }
//    }
//    // Metatrader Part
//    // update indicators for trailing stop
//    // check to see if we should close any open position
//    // 1. by time
//    // 2. and by trailing stop
//}

// will be called every < 1 second
//  Metatrader 5
int AddTicks(const MqlTick* cticks, int size) {

    return m_bars->AddTicks(cticks, size);

}

// Since all buffers must be alligned and also have same size
// Expert_BufferSize 
// Same new bars will come for all indicators and else
// All are updated after bars so that's the main
// begin of new bar is also begin of new data on all buffers
int BufferSize() { return m_bars->Size(); }
int BufferTotal() { return m_bars->Count(); }
// start index on all buffers of new bars after AddTicks > 0
int IdxNewData() { return m_bars->Count() - m_bars->m_nnew; }

// start index and count of new bars that just arrived
bool Refresh()
{
    m_bars->RefreshArrays(); // update internal latest arrays of times, prices
    m_times->AddRangeTimeMs(m_bars->m_times.data(), 0, m_bars->m_nnew);
    bool result = true;

    // all bellow must be called to maintain alligment of buffers
    // by creating empty samples

    // features indicators
    // fracdif on bar prices order of && matter
    // first is what you want to do and second what you want to try
    result = m_fd_mbarp->Refresh(m_bars->m_last.data(), 0, m_bars->m_nnew) && result;

    // update all bollinger bands indicators
    for (int j = 0; j < m_nbands; j++) {
        result = m_bands[j]->Refresh(m_bars->m_last.data(), 0, m_bars->m_nnew) && result;
    }

    // called after m_ticks.Refresh() and Refreshs() above
    // garantes not called twice and bband indicators available
    RefreshRawBandSignals(m_bars->m_last.data(), m_bars->m_nnew, result);

    return result;
}

void RefreshRawBandSignals(double last[], int count, int empty) {
    // should be called only once per refresh
    // use the last added samples
    empty = (empty) ? 1 : 0;
    int start_new = IdxNewData();
    for (int j = 0; j < m_nbands; j++) {
        //    Based on a bollinger band defined by upper-band and lower-band
        //    return signal:
        //        buy   1 : crossing down-outside it's buy
        //        sell -1 : crossing up-outside it's sell
        //        hold  0 : nothing usefull happend
        for (int i = 0; i < count; i++) {
            if (last[i] >= m_bands[j]->m_upper[start_new + i])
                m_raw_signal[j].Add(-1 * empty); // sell
            else
                if (last[i] <= m_bands[j]->m_down[start_new + i])
                    m_raw_signal[j].Add(+1 * empty); // buy
                else
                    m_raw_signal[j].Add(0); // nothing
        }
    }
}

void verifyEntry() {

    // has an entry signal in any band? on the lastest
    // raw signals added bars
    if (lastRawSignals() != -1)
    {
        int m_xypair_count_old = m_xypairs.Count();
        // only when a new entry signal recalc classes
        // only when needed
        //LabelClasses();
        //CreateXFeatureVectors(m_xypairs);
        // add number of xypairs added (cannot decrease)
        m_xypair_count += (m_xypairs.Count() - m_xypair_count_old);
        // update/train model if needed/possible
        //if (m_xypairs.Count() >= m_ntraining) {
        //    // first time training
        //    if (!m_model.isready) {
        //        // time to train the model for the first time
        //        m_model.isready = PythonTrainModel();
        //    }
        //    //else // time to update the model?
        //    //if((m_xypair_count - m_model_last_training) >= m_model_refresh){
        //    // if diference in training samples to the last training is bigger than
        //    // refresh criteria - time to train again
        //    //  m_model.isready = PythonTrainModel();
        //    //}
        //    // record when last training happened
        //    m_model_last_training = m_xypair_count;
        //}
        //// Call Python if model is already trainned
        //if (m_model.isready) {
        //    // x forecast vector
        //    XyPair Xforecast = new XyPair;
        //    Xforecast.time = m_times[m_last_raw_signal_index];
        //    CreateXFeatureVector(Xforecast);
        //    int y_pred = PythonPredict(Xforecast);
        //    BuySell(y_pred);
        //    if (m_recursive && (y_pred == 0 || y_pred == 1 || y_pred == -1)) {
        //        // modify the band raw signals by the ones predicted
        //        for (int i = 0; i < m_nbands; i++)
        //            // change from -1 to 1 or to 0
        //            m_raw_signal[i].m_data[m_raw_signal[i].Count() - 1] = y_pred;
        //    }
        //}
    }
}


// verify an entry signal in any band
// on the last m_added bars
// get the latest signal
// return index on buffer of existent entry signal
// only if more than one signal
// all in the same direction
// otherwise returns -1
// only call when there's at least one signal
// stored in the buffer of raw signal bands
int lastRawSignals() {
    int direction = 0;
    m_last_raw_signal_index = -1;
    // m_last_raw_signal_index is index based
    // on circular buffers index alligned on all
    for (int j = 0; j < m_nbands; j++) {
        for (int i = IdxNewData(); i < BufferTotal(); i++) {
            m_last_raw_signal[j] = m_raw_signal[j][i];
            if (m_last_raw_signal[j] != 0) {
                if (direction == 0) {
                    direction = m_last_raw_signal[j];
                    m_last_raw_signal_index = i;
                }
                else
                    // two signals with oposite directions on last added bars
                    // lets keep it simple now - lets ignore them
                    if (m_last_raw_signal[j] != direction) {
                        m_last_raw_signal_index = -1;
                        return -1;
                    }
                    else
                        // same direction only update index to get last
                        m_last_raw_signal_index = i;
            }
        }
    }
    return m_last_raw_signal_index;
}


// Python API

// or by Python 
int pyAddTicks(py::array_t<MqlTick> ticks)
{
    return AddTicks(ticks.data(), ticks.size());
}

py::array_t<MoneyBar> pyGetMoneyBars() {
    MoneyBar* pbuf = (MoneyBar*) pymbars.request().ptr;
    for (size_t idx = 0; idx<Expert_BufferSize; idx++)
        pbuf[idx] = m_bars->m_data[idx];
    return pymbars;
}
