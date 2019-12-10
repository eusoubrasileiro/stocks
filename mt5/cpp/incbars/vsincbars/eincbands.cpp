#define BUILDING_DLL
#include "eincbands.h"
#include "embpython.h"

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
MoneyBarBuffer m_bars; // buffer of money bars base for everything
std::vector<double> m_mlast; // moneybar.last values of last added money bars
double m_moneybar_size; // R$ to form 1 money bar

// bollinger bands configuration
// upper and down and middle
int m_nbands; // number of bands
int m_bbwindow; // reference size of bollinger band indicator others
double m_bbwindow_inc;   // are multiples of m_bbwindow_inc

// feature indicators
CFracDiffIndicator m_fd_mbarp; // frac diff on money bar prices

// store buy|sell|hold signals for each bband - raw
std::vector<CBandSignal> m_rbandsgs;

std::vector<int> m_last_raw_signal; // last raw signal in all m_nbands
int m_last_raw_signal_index; // related to m_bars buffer on refresh()

// storage of signals
// save only signal ocurrences
std::list<bsignal> m_bsignals; // signals
// last uid of bar inserted on storage of signals
int64_t m_bsignals_lastuid;

// features and training vectors
// array of buffers for each signal feature
int m_nsignal_features;
int m_batch_size;
int m_ntraining;
int m_xtrain_dim; // dimension of x train vector
buffer<XyPair> m_xypairs;

// For labelling
// execution of positions and orders
// max time to hold a position in miliseconds
int64_t m_expire_time;
// operational window for expert since 00:00:00
double m_start_hour;
double m_end_hour;
// round price minimum volume of symbol 100's stocks 1 for future contracts
double m_lotsmin;
// LotsMin same as SYMBOL_VOLUME_MIN
#define roundVolume(vol) int(vol/m_lotsmin)*m_lotsmin
// profit or stop loss calculation for training
double m_ordersize;
double m_stoploss; // stop loss value in $$$ per order
double m_targetprofit; //targetprofit in $$$ per order
int    m_incmax;// max number of increases on a position

// max number of orders
// position being a execution of one order or increment
// on the same direction
// number of maximum 'positions'
//int m_max_positions; // maximum number of 'positions'
//int m_last_positions; // last number of 'positions'
//double m_last_volume; // last volume + (buy) or - (sell)
//double m_volume; // current volume of open or not positions



// python sklearn model trainning
//bool m_recursive;
//sklearnModel m_model;
unsigned int m_model_refresh; // how frequent to update the model
// (in number of new training samples)
// helpers to count training samples
unsigned long m_xypair_count; // counter to help train/re-train model
unsigned long m_model_last_training; // referenced on m_xypair_count's




void Initialize(int nbands, int bbwindow, int batch_size, int ntraining,
    double start_hour, double end_hour, double expire_hour,
    double ordersize, double stoploss, double targetprofit, int incmax,
    double lotsmin, double ticksize, double tickvalue,
    double moneybar_size) // R$ to form 1 money bar
{
    m_incmax = incmax;
    m_start_hour = start_hour;
    m_end_hour = end_hour;
    m_expire_time = (int64_t) expire_hour*3600*1000; // to ms
    m_lotsmin = lotsmin;
    m_moneybar_size = moneybar_size;
    m_bbwindow_inc = 0.5;
    //m_model = new sklearnModel;
    m_xypair_count = 0;
    m_model_last_training = 0;
    m_last_raw_signal_index = -1;
    // set safe pointers to new objects
    m_bars.Init(tickvalue,
        ticksize, m_moneybar_size);

    m_nbands = nbands;
    m_bbwindow = bbwindow;
    // ordersize is in $$
    m_ordersize = ordersize;
    // for training
    m_stoploss = stoploss; // value in $$$
    m_targetprofit = targetprofit;
    // training
    m_batch_size = batch_size;
    m_ntraining = ntraining; // minimum number of X, y training pairs
    // total of signal features
    // m_nbands band signals
    // indicar features is 3
    // 1 - fracdiff on prices - stationary
    // 2 - time to form a bar in seconds - stationary adfuller test
    // 3 - number of ticks to form a bar  - stationary adfuller test
    m_nsignal_features = m_nbands + 3;
    // one additional feature - last value
    // +1 - band number - disambiguation - 
    //     equal X's might need to be classified differently 
    //     due comming from a different band
    m_xtrain_dim = m_nsignal_features * m_batch_size + 1;

    // group of samples and signal vectors to train the model
    // resize buffer of training pairs
    m_xypairs.set_capacity(m_ntraining);
    // resize x feature vector inside it
    // but they do not exist cant resize something that doesnt exist
    // only the space available for them
    //m_model_refresh = model_refresh;

    m_rbandsgs.resize(m_nbands); // one for each band
    m_last_raw_signal.resize(m_nbands); // resize and fill in (but we dont care will be overwritten)

    m_bsignals_lastuid = 0;

    // void CreateBBands()
    // raw signal storage + ctalib bbands
    double inc = m_bbwindow_inc;
    for (int i = 0; i < m_nbands; i++) { // create multiple increasing bollinger bands
        m_rbandsgs[i].Init(m_bbwindow * inc, 2.5, 2); // 2-Weighted type
        // weighted seams more meaning since i know each bar
        // represents same ammount of money so have a equal weight
        inc += m_bbwindow_inc;
    }

    // CreateOtherFeatureIndicators();
    // only fracdiff on prices
    m_fd_mbarp.Init(Expert_Fracdif_Window, Expert_Fracdif);
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
size_t AddTicks(const MqlTick* cticks, int size) {

    return m_bars.AddTicks(cticks, size);

}

// Since all buffers must be alligned and also have same size
// Expert_BufferSize
// Same new bars will come for all indicators and else
// All are updated after bars so that's the main
// begin of new bar is also begin of new data on all buffers
size_t BufferSize() { return m_bars.capacity(); }
size_t BufferTotal() { return m_bars.size(); }
// start index on all buffers of new bars after AddTicks > 0
size_t NewDataIdx() { return m_bars.BeginNewBarsIdx();  }

// start index and count of new bars that just arrived
bool Refresh()
{
    //int valid_start = 0;

    //// arrays of new data update them
    //m_bars.RefreshArrays();
    //// all bellow must be called to maintain alligment of buffers
    //// by creating empty samples

    //// features indicators
    //// fracdif on bar prices order of && matter
    //// first is what you want to do and second what you want to try
    //m_fd_mbarp.Refresh(m_bars.new_avgprices, m_bars.m_nnew);

    //valid_start = m_fd_mbarp.validIdx();

    //// update signals based on all bollinger bands
    //for (int j = 0; j < m_nbands; j++) {
    //    m_rbandsgs[j].Refresh(m_bars.new_avgprices, m_bars.m_nnew);
    //    int tmp_start = m_rbandsgs[j].validIdx();
    //    // get only the intersection - region where samples were calculated
    //    // on all indicators
    //    valid_start = (result < ncalculated) ? result : ncalculated;
    //}

    //if (ncalculated > 0) {
    //// valid samples calculated
    //// store only signal ocurrences
    //int i = BufferTotal() - ncalculated;
    //for (; i < BufferTotal(); i++)
    //    for (int j = 0; j < m_nbands; j++)
    //        if (m_rbandsgs[j][i] != 0)  // need to know which uid/time for each sample added
    //            m_bsignals.push_back({ m_bars.uidtimes[i], j, (int)m_rbandsgs[j][i] });
    //}

    //return (ncalculated > 0);
    return true;
}


void verifyEntry() {

    // has an entry signal in any band? on the lastest
    // raw signals added bars
    if (lastRawSignals() != -1)
    {
        int m_xypair_count_old = m_xypairs.size();
        // only when a new entry signal recalc classes
        // only when needed
        LabelClasses();
        //CreateXFeatureVectors(m_xypairs);
        // add number of xypairs added (cannot decrease)
        m_xypair_count += (m_xypairs.size() - m_xypair_count_old);
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
        //            m_rbandsgs[i].m_data[m_rbandsgs[i].Count() - 1] = y_pred;
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
        for (int i = NewDataIdx(); i < BufferTotal(); i++) {
            m_last_raw_signal[j] = m_rbandsgs[j][i];
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


// next n signals with same sign <buffer index, band number>
// and band number higher than previous
// we can only increase positions if band higher than previous
inline std::list<std::pair<uint64_t, int>> bufidxNextnSame(std::list<bsignal>::iterator start,
                             std::list<bsignal>::iterator end, int sign, int band, uint64_t bfidx_ct){
    std::list<std::pair<uint64_t, int>> bufidxnextn; // indexes of next n signals same sign
    int ncount = 0;
    int last_band = band;
    // go to next - first signal after passed-signal
    start++;
    while (start != end && ncount < m_incmax){
        if (start->sign == sign && start->band > last_band) {
            bufidxnextn.push_back(std::make_pair(start->tuidx - bfidx_ct, start->band)); // current buffer index
            ncount++;
            last_band = start->band;
        }
        start++;
    }
    return bufidxnextn;
}

// Create target class by analysing raw band signals
// label or re-label or re-classify every signal on every band
// that are different than 0
// those that went true receive 1 buy or -1 sell
// those that went bad receive 0 hold
void LabelClasses(){
    // w. bags of signals saved
    // label every signal
    // Creating Xy pairs
    XyPair xy;
    int res;
    auto iter = m_bsignals.begin();
    uint64_t bfidx_ct = 0;

    // correction of position of signals to current buffer index position
    if (iter != m_bsignals.end()) // correction between uid's and current buffer index
        bfidx_ct = iter->tuidx - m_bars.Search(iter->tuidx);

    while(iter != m_bsignals.end()){
        auto nextn = bufidxNextnSame(iter, m_bsignals.end(), iter->sign, iter->band, bfidx_ct);
        res = LabelSignal(*iter, iter->tuidx - bfidx_ct, nextn, xy); 
        if (res == -2) // not enough data for labelling - stop everything
            break;
        else
        if (res == 1) {
            m_bsignals.erase(iter); // done with this
            m_xypairs.add(xy);
        }
        else
        if (res == -3) { // outside the operational window
            m_bsignals.erase(iter); // remove this
        }
        iter++;
    }
}

int LabelSignal(bsignal signal, size_t bfidx, std::list<std::pair<uint64_t, int>> nextn, XyPair& xy){
    // targetprofit - profit to close open position
    // amount - tick value * quantity bought in R$
    int day = 0;
    double entryprice = 0;
    double profit = 0;
    double slprofit = 0; // high-low profit (based on ask-bid prices)
    double quantity = 0; // number of contracts or stocks shares


    // get 'real' price of execution of this signal
    // first bar considering the operational time delay
    // first bar with time >= expert delay included
    int start_bfidx = m_bars.SearchStime(m_bars[bfidx].emsc + Expert_Delay);

    if (start_bfidx == -1) // cannot start labelling
        return -2; //  did not find bar, not enough data in future

    entryprice = m_bars[start_bfidx].avgprice;

    // save this buy or sell
    xy.bsg = signal;
    xy.y = signal.sign; // start by beliving 
    // for this position
    // start time - where buy/sell really takes place
    int64_t start_time = m_bars[start_bfidx].emsc;
    // current day
    tm time = m_bars[start_bfidx].time;
    day = time.tm_mday; // day-of-month identifier for crossing-day

    // ignore a signal if it is after
    // the operational window
    // to avoid contaminating model
    // current hour decimal
    double chour = (time.tm_sec / 3600. + time.tm_min / 60.) + time.tm_hour;
    if (chour > m_end_hour || chour < m_start_hour)
        return -3;

    int secs_toend_day = (m_end_hour - chour) * 3600;
    // time in ms for operations end of day
    int64_t end_day = m_bars[start_bfidx].smsc + secs_toend_day * 1000;

    // buffer index of next signal with same sign
    auto nextsignidx = nextn.begin();

    // will only increase position 
    // if signal comes from a superior band -- higher number
    int last_band = signal.band; 

    quantity = roundVolume(m_ordersize / entryprice);
    // starting jump forward to where the buy/sell really takes place
    for (int i=start_bfidx; i<BufferTotal(); i++) { // from past to present

        // see if should close:
        // - calculate profit / stoploss
        // - closed by time

        // use avgprice for TP
        // and high-low bid from ticks to stop_loss
        if (signal.sign > 0) { // long
            profit = (m_bars[i].avgprice - entryprice) * quantity;// # avg. current profit
            //  stop-loss profit - people buy at bid -  (low worst scenario)
            slprofit = (m_bars[i].bidl - entryprice) * quantity;
        }
        else { // short
            profit = (entryprice - m_bars[i].avgprice) * quantity;// # avg. current profif
            //  stop-loss profit - people buy at bid - (high worst scenario)
            slprofit = (entryprice - m_bars[i].bidh) * quantity;
        }
        // first barrier
        if (profit >= m_targetprofit) // done classifying this signal
            return 1;
        // second barrier
        else
        if (slprofit <= (-m_stoploss)) { // re-classify as hold
            xy.y = 0; // not a good deal, was better not to entry
            return 1;
        }
        // third barrier - time 
        // 1.expire-time or 2.day-end 
        // or 3.crossed-to a new day (should never happen)
        if(m_bars[i].emsc - start_time > m_expire_time ||
            m_bars[i].emsc > end_day || // operations end of day
            m_bars[i].time.tm_mday != day) { // crossed to a new day
            xy.y = 0; // expired position
            return 1;
        }
        // increase position if 
        // 1. can increase position - maxinc restriction
        // 2. is time for that
        // 3. it is from a higher band - starting w. original signal
        // pair is <buffer index, band number>
        if (nextsignidx != nextn.end() && 
            nextsignidx->first == i && 
            nextsignidx->second > last_band){
            // find entry time
            auto new_posbfidx = m_bars.SearchStime(m_bars[nextsignidx->first].emsc + Expert_Delay);
            if (new_posbfidx == -1) // cannot continue labelling
                return -2; //  did not find bar, not enough data in future
            // to avoid contaminating model
            // ignore the new signal if it is after
            // 1. the operational window
            // 2. expire time
            // 3. crossed to a new day
            if (m_bars[new_posbfidx].emsc - start_time > m_expire_time ||
                m_bars[new_posbfidx].emsc > end_day || // operations end of day
                m_bars[new_posbfidx].time.tm_mday != day) { // crossed to a new day                
                nextsignidx = nextn.end(); // cannot increase any other position
                continue;
            }
            auto new_entryprice = m_bars[new_posbfidx].avgprice;
            auto new_quantity = roundVolume(m_ordersize / new_entryprice);
            // updtate to weighted average price
            entryprice = (new_entryprice * new_quantity) + (entryprice * quantity);
            entryprice /= (new_quantity + quantity);
            // sum quantity
            quantity += new_quantity;            
            last_band = nextsignidx->second;
            nextsignidx++;
        }
    }
    // if reaches here did not close the position
    // did not expire
    // did not stop-loss
    // not enough data to classify this sample yet
    return -1;
}


void CreateXFeatureVectors()
{ // using the pre-filled buffer of y target classes
  // assembly the X array of features for it
  // from more recent to oldest
  // if it is already filled (ready) stop
    for (int i=0; i < m_xypairs.size(); i++) {
        // no need to assembly any other if this is already ready
        // olders will also be ready
        if(CreateXFeatureVector(m_xypairs[i]) == -1) // old signal or cannot create
           m_xypairs.erase(m_xypairs.begin() + i);
    }
}

int CreateXFeatureVector(XyPair &xypair)
{
    if (xypair.isready) // no need to assembly
        return 0;

    // find xypair.time current position on all buffers (have same size)
    // for the signal that triggered this xypair
    // uidxtime is allways sorted
    int bfidxsg = m_bars.Search(xypair.bsg.tuidx);

    // cannot assembly X feature train with such an old signal
    //  not in buffer anymore - such should be removed ...
    if (bfidxsg < 0)
        return -1;
    // I want to include the signal the originates this xypair
    bfidxsg += 1;
    int batch_start_idx = bfidxsg - m_batch_size;
    // not enough : bands raw signal, features in time to form a batch
    // cannot create a X feature vector and will never will
    // remove it
    if (batch_start_idx < 0)
        return -1;

    xypair.X.resize(m_xtrain_dim);
    int xifeature = 0;
    // something like
    // double[Expert_BufferSize][nsignal_features] would be awesome
    // easier to copy to X also to create cross-features
    // using a constant on the second dimension is possible    
    for (int timeidx = batch_start_idx; timeidx < bfidxsg; timeidx++) {
        // from past to present [bufi-batch_size:bufi+1)
        // features from band signals
        for (int i = 0; i < m_nbands; i++, xifeature++) {
            xypair.X[xifeature] = m_rbandsgs[i][timeidx];
        }
        // fracdif feature
        xypair.X[xifeature++] = m_fd_mbarp[timeidx];
        // time to form a bar in seconds
        xypair.X[xifeature++] = (m_bars[timeidx].emsc- m_bars[timeidx].smsc)/1000.;
        // number of ticks to form a bar
        xypair.X[xifeature++] = m_bars[timeidx].nticks;

        // could use -- askh-askl or bidh-bidl? but there are many outliers (analysis on python)
        // data is reliable since I also need to interpolate it
        // also ask prices are 'fake' appearances
        // so better not use

        // some indicators might contain EMPTY_VALUE == DBL_MAX values
        // sklearn throws an exception because of that
        // anyhow that's not a problem for the code since the python exception
        // is catched behind the scenes and a valid model will only be available
        // once correct samples are available
    }
    // last feature
    // disambiguation - band number
    xypair.X[xifeature++] = xypair.bsg.band;

    xypair.isready = true;
    return 1; // suceeded
}




////////////////////////////////////////////////
///////////////// Python API //////////////////
///////////////////////////////////////////////


// or by Python
size_t pyAddTicks(py::array_t<MqlTick> ticks)
{
    return AddTicks(ticks.data(), ticks.size());
}

std::shared_ptr<py::array_t<MoneyBar>> ppymbars;

py::array_t<MoneyBar> pyGetMoneyBars() {
    MoneyBar* pbuf = (MoneyBar*)ppymbars->request().ptr;
    for (size_t idx = 0; idx<BUFFERSIZE; idx++)
        pbuf[idx] = m_bars[idx];
    return *ppymbars;
}

std::tuple<py::array, py::array, py::array> pyGetXyvectors(){
    // pybind11 automatically casts/converts std:: types
    // vectors got to a list
    // but you can use cast to convert to a numpy array
    // needs "pybind11/stl.h"
    std::vector<double> X;
    std::vector<uint64_t> tidx(m_xypairs.size());
    std::vector<double> Y(m_xypairs.size());  // if use push_back dont need set size
    X.resize((m_xypairs.size() * m_xtrain_dim)); // neeeded to std::copy

    int idx = 0;
    for (; idx < m_xypairs.size(); idx++) {
        if (m_xypairs[idx].isready) {
            std::copy(m_xypairs[idx].X.begin(), m_xypairs[idx].X.end(),
                X.begin() + idx * m_xtrain_dim);
            Y[idx]= m_xypairs[idx].y;
            tidx[idx] = m_xypairs[idx].bsg.tuidx;
        }
    }

    X.resize(idx * m_xtrain_dim); // to data size
    Y.resize(idx);
    tidx.resize(idx);
    py::array pyX = py::cast(X);
    py::array pyY = py::cast(Y);
    py::array pyIdx = py::cast(tidx);
    pyX.resize({ idx, m_xtrain_dim });

    return std::make_tuple(pyX, pyY, pyIdx);
}


int pyGetXdim() {
    return m_xtrain_dim;
}
