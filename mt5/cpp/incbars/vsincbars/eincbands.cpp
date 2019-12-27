#define BUILDING_DLL
#include "eincbands.h"

// I dont want to use a *.def file to export functions from a namespace
// so I am not using namespaces {namespace eincbands}


// starts/increases positions against the trend to get profit on mean reversions
// Using Dolar/Real Bars and consequently tick data
// only for tick time-frame
// Net Mode Only!
// isInsideDay() should be checked by Metatrader prior doing
// any openning of positions

// Base Data
std::shared_ptr<BufferMqlTicks> m_ticks; // buffer of ticks w. control to download unique ones.

// All Buffer-Derived classes bellow have indexes alligned
std::shared_ptr<MoneyBarBuffer> m_bars; // buffer of money bars base for everything
bool m_newbars;

double m_moneybar_size; // R$ to form 1 money bar
// offset correction between uid and current buffer indexes
uint64_t m_bfoffset = 0;

// bollinger bands configuration
// upper and down and middle
int m_nbands; // number of bands
int m_bbwindow; // reference size of bollinger band indicator others
double m_bbwindow_inc;   // are multiples of m_bbwindow_inc

// feature indicators
std::shared_ptr<CFracDiffIndicator> m_fd_mbarp; // frac diff on money bar prices

// store buy|sell|hold signals for each bband - raw
std::vector<CBandSignal> m_rbandsgs;

BSignal m_last_signal; // last raw signal in all m_nbands

// storage of signals
// save only signal ocurrences
std::list<BSignal> m_bsignals; // signals
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
sklearnModel m_model;
unsigned int m_model_refresh; // how frequent to update the model
// (in number of new training samples)
// helpers to count training samples
unsigned long m_xypair_count; // counter to help train/re-train model
unsigned long m_model_last_training; // referenced on m_xypair_count's
double m_devs;

void CppExpertInit(int nbands, int bbwindow, double devs, int batch_size, int ntraining,
    double start_hour, double end_hour, double expire_hour,
    double ordersize, double stoploss, double targetprofit, int incmax,
    double lotsmin, double ticksize, double tickvalue,
    double moneybar_size,  // R$ to form 1 money bar
    // ticks control, 
    bool isbacktest, 
    char *cs_symbol,  // cs_symbol is a char[] null terminated string (0) value at end
    int64_t mt5_timenow,
    short mt5debug)
{
    mt5_debug_level = mt5debug; // level of messages while debugging on metatrader
    m_devs = devs;
    m_incmax = incmax;
    m_start_hour = start_hour;
    m_end_hour = end_hour;
    m_expire_time = (int64_t) ((double) expire_hour*3600*1000); // to ms
    m_lotsmin = lotsmin;
    m_moneybar_size = moneybar_size;
    m_bbwindow_inc = 0.5;
    //m_model = new sklearnModel;
    m_xypair_count = 0;
    m_model_last_training = 0;

    // reset everythin first
    m_ticks.reset(new BufferMqlTicks());
    m_bars.reset(new MoneyBarBuffer());
    m_fd_mbarp.reset(new CFracDiffIndicator());
    m_xypairs.clear();    
    m_rbandsgs.clear();
    m_bsignals.clear();

    m_ticks->Init(std::string(cs_symbol),
        isbacktest, mt5_timenow);
        
    m_bars->Init(tickvalue,
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
    // 1 - m_nbands band signals positive (1 byte OR coded)
    // 2 - m_nbands band signals negative (1 byte OR coded)
    // indicator features is 3
    // 3 - fracdiff on prices - stationary
    // 4 - time to form a bar in seconds - stationary adfuller test
    // 5 - number of ticks to form a bar  - stationary adfuller test
    m_nsignal_features = 2 + 3 + 2;
    // cross features
    // 6 - diff of times to form this bar with the previous    
    // 7 - diff of ticks to form this bar with the previous
    // one additional feature - last value 
    // 8 - band number - disambiguation -
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

    m_bsignals_lastuid = 0;

    // void CreateBBands()
    // raw signal storage + ctalib bbands
    double inc = m_bbwindow_inc;
    for (int i = 0; i < m_nbands; i++) { // create multiple increasing bollinger bands
        m_rbandsgs[i].Init(m_bbwindow * inc, m_devs, 2); // 2-Weighted type
        // weighted seams more meaning since i know each bar
        // represents same ammount of money so have a equal weight
        inc += m_bbwindow_inc;
    }

    // CreateOtherFeatureIndicators();
    // only fracdiff on prices
    m_fd_mbarp->Init(Expert_Fracdif_Window, Expert_Fracdif);
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


// returns the next cmpbegin_time
int64_t CppOnTicks(MqlTick *mt5_pticks, int mt5_nticks){

    int64_t cmpbegin_time = m_ticks->Refresh(mt5_pticks, mt5_nticks);

    m_newbars = false; 

    if (m_ticks->nNew() > 0){
        if (m_bars->AddTicks(m_ticks->end() - m_ticks->nNew(),
            m_ticks->end()) > 0)
            m_newbars = true;
    }

    return cmpbegin_time; 
}




// Since all buffers must be alligned and also have same size
// Expert_BufferSize
// Same new bars will come for all indicators and else
// All are updated after bars so that's the main
// begin of new bar is also begin of new data on all buffers
size_t BufferSize() { return m_bars->capacity(); }
size_t BufferTotal() { return m_bars->size(); }
// start index on all buffers of new bars after AddTicks > 0
size_t NewDataIdx() { return m_bars->BeginNewBarsIdx();  }
bool CppNewData() { return m_newbars; }

// start index of valid data on all buffers (indicators) any time
size_t ValidDataIdx() {
    size_t valid_start = 0;
    valid_start = m_fd_mbarp->valididx();
    for (int j = 0; j < m_nbands; j++){
        size_t tmp_start = m_rbandsgs[j].valididx();
        valid_start = (tmp_start > valid_start) ? tmp_start : valid_start;
    }
    return valid_start;
}

BufferMqlTicks* GetTicks() // get m_ticks variable
{ 
    return &(*m_ticks);
}

void TicksToFile(char* cs_filename) {
    BufferMqlTicks* ticks = GetTicks();
    SaveTicks(ticks, std::string(cs_filename));
}

bool isInsideFile(char* cs_filename) {
    BufferMqlTicks* ticks = GetTicks();
    return isInFile(ticks, std::string(cs_filename));
}


// start index and count of new bars that just arrived
bool CppRefresh()
{
    int ncalculated = 0;

    // arrays of new data update them
    m_bars->RefreshArrays();
    // all bellow must be called to maintain alligment of buffers
    // by creating empty samples

    // features indicators
    // fracdif on bar prices order of && matter
    // first is what you want to do and second what you want to try
    ncalculated = m_fd_mbarp->Refresh(m_bars->new_avgprices, m_bars->m_nnew);

    // update signals based on all bollinger bands
    for (int j = 0; j < m_nbands; j++) {
        int tmp_calc = m_rbandsgs[j].Refresh(m_bars->new_avgprices, m_bars->m_nnew);
        // get only the intersection - smallest region
        // where samples were calculated on all indicators
        ncalculated = (tmp_calc < ncalculated) ? tmp_calc : ncalculated;
    }

    if (ncalculated > 0) {
        // valid samples calculated
        // store only signal ocurrences
        int i = BufferTotal()-ncalculated;
        for (; i < BufferTotal(); i++)
            for (int j = 0; j < m_nbands; j++)
                if (m_rbandsgs[j][i] != 0)  // need to know which uid/time for each sample added
                    m_bsignals.push_back({ m_bars->at(i).emsc, m_bars->at(i).uid, j, (int)m_rbandsgs[j][i] });
    }

    return (ncalculated > 0);
}

// verify an entry signal in any band
// on the last m_added bars
// get the latest signal
// return index on buffer of existent entry signal
// only if more than one signal
// all in the same direction
// otherwise returns -1
// only call when there's at least one signal
// stored in the list of raw signal bands m_bsignals
inline bool lastSignals(unixtime now){
    unixtime_ms nowms = now * 1000;

    if(m_bsignals.size() == 0)
        return false;

    auto bsign = m_bsignals.rbegin(); // start by the last
    auto last_time = bsign->time;    
    // if last tick time is more recent than unixtime
    // use it
    nowms = (last_time > nowms)? last_time : nowms;
    // calculate delay limite to enter 
    auto time_limit = nowms - Expert_Acceptable_Entry_Delay; 
    if (last_time < time_limit) // last signal is already too old
        return false;
    int direction = bsign->sign; // all signs must be equal this 
    for (; bsign != m_bsignals.rend() && bsign->time >= time_limit; ++bsign)
        if (bsign->sign != direction)
            return false;
    --bsign; 
#ifdef META5DEBUG
    debugfile << "lastBSignals total signals analysed: " << std::distance(m_bsignals.rbegin(), bsign) + 1 << std::endl;
    debugfile << "lastBSignals delay (ms): " << nowms - m_bsignals.rbegin()->time << std::endl;
#endif
    // in the end is using only the last signal
    return true; // last signal
}

// returns -1/0/1 Sell/Nothing/Buy 
int isSellBuy(unixtime now) {

    auto result = 0;
    
    // has an entry signal in any band? on the lastest
    // raw signals added bars
    if (lastSignals(now))
    {
        auto last = *m_bsignals.rbegin(); // last band signal
        int m_xypair_count_old = m_xypairs.size();
        // only when a new entry signal recalc classes
        CreateXyVectors(); 

        // update/train model if needed/possible
        if(m_xypairs.size() >= m_ntraining && !m_model.isready)
            m_model.isready = PythonTrainModel();        

        // Call Python if model is already trainned/valid
        if(m_model.isready) {
           // x forecast vector
           XyPair X;
           X.band = last.band;
           X.sign = last.sign;           
           X.twhen = last.twhen;           
           FillInXFeatures(X);
           result = PythonPredict(X); // returns 0 or 1
           result = last.sign * result; // turn in -1 or 1
        }
    }

    return result;
}


void invalidateModel() {
    m_model.isready = false;
}



// start is the start
// next n signals with same sign <buffer index, band number>
// 1. and band number higher than previous
// 2. and time higher than last_time (cannot accept signs in different bands at the same time)
// we can only increase positions if band higher than previous
// and time greater than previous
inline std::list<std::pair<uint64_t, int>>
    bufidxNextnSame(std::list<BSignal>::iterator start, std::list<BSignal>::iterator end, uint64_t last_uidtime)
{
    std::list<std::pair<uint64_t, int>> bufidxnextn; // indexes of next n signals same sign
    BSignal current = *start;
    int ncount = 0;
    int last_band = current.band;
    // go to next - first signal after passed-signal
    start++;
    while (start != end && ncount < m_incmax){
        if (start->sign == current.sign && start->band > last_band && start->twhen > last_uidtime) {
            bufidxnextn.push_back(std::make_pair(start->twhen - m_bfoffset, start->band)); // current buffer index
            ncount++;
            last_band = start->band;
            last_uidtime = start->twhen;
        }
        start++;
    }
    return bufidxnextn;
}

// correction between uid's and current buffer index
// actual uid position on buffer is
// uid - m_bfoffset
inline void updateBufferUidOffset(){
    // uid starts at 0 alligned with bars (and all buffers) BUT
    // when buffers gets full allignment is lost
    // but a constant offset exists betwen uid and indexes of buffers
    m_bfoffset = m_bars->at(0).uid - m_bars->Search(m_bars->at(0).uid);
}


// Create target class by analysing raw band signals
// label every signal on every band
// that are different than 0
// those that went sucess receive 1
// those that went bad receive 0 hold
int CreateXyVectors(){
    // w. bags of signals saved
    // label every signal
    // Creating Xy pairs
    XyPair xy;
    int count, res;
    auto current = m_bsignals.begin();

    updateBufferUidOffset();
    count = 0;
    while(current != m_bsignals.end()){
        res = LabelSignal(current, m_bsignals.end(), xy);
        if (res == -2) // not enough data for labelling - stop everything
            break;
        else
        if (res == 1) {
            m_bsignals.erase(current); // done with this
            if (FillInXFeatures(xy)) { // remove old signal or cannot create
                m_xypairs.add(xy);
                count++;
            }
        }
        else
        if (res == -3) { // outside the operational window
            m_bsignals.erase(current); // remove this
        }
        current++;
    }
    return count; // number of xy pairs ready for training
}

int LabelSignal(std::list<BSignal>::iterator current, std::list<BSignal>::iterator end, 
                XyPair& xy){
    // targetprofit - profit to close open position
    // amount - tick value * quantity bought in R$
    int day = 0;
    double entryprice = 0;
    double profit = 0;
    double slprofit = 0; // high-low profit (based on ask-bid prices)
    double quantity = 0; // number of contracts or stocks shares

    size_t bfidx = current->twhen - m_bfoffset; // actual buffer index

    // get 'real' price of execution of this signal
    // first bar considering the operational time delay
    // first bar with time >= expert delay included
    int start_bfidx = m_bars->SearchStime(m_bars->at(bfidx).emsc + Expert_Delay);

    if (start_bfidx == -1) // cannot start labelling
        return -2; //  did not find bar, not enough data in future

    entryprice = m_bars->at(start_bfidx).avgprice;

    // save this buy or sell
    xy.time = current->time;
    xy.band = current->band;
    xy.sign = current->sign;
    xy.twhen = current->twhen;
    xy.ninc = 0; // no increases yet
    xy.y = 1; // start beliving
    // for this position
    // start time - where buy/sell really takes place
    auto start_time = m_bars->at(start_bfidx).emsc;
    // current day
    tm time = m_bars->at(start_bfidx).time;
    day = time.tm_yday; // day-of-year identifier for crossing-day

    // ignore a signal if it is after
    // the operational window
    // to avoid contaminating model
    // current hour decimal
    double chour = (time.tm_sec / 3600. + time.tm_min / 60.) + time.tm_hour;
    if (chour > m_end_hour || chour < m_start_hour)
        return -3;

    int secs_toend_day = (m_end_hour - chour) * 3600;
    // time in ms for operations end of day
    unixtime_ms end_day = m_bars->at(start_bfidx).smsc + secs_toend_day * 1000;

    // after getting the start_bfidx we search for the nextn
    // buffer index of next signal
    // 1. with same sign
    // 2. in a higher band
    // 3. uid time greater than start_bfidx
    auto nextn = bufidxNextnSame(current, end, m_bars->at(start_bfidx).uid);
    auto nextsignidx = nextn.begin();

    // will only increase position
    // if signal comes from a superior band -- higher number
    int last_band = current->band;

    quantity = roundVolume(m_ordersize / entryprice);
    // starting jump forward to where the buy/sell really takes place
    for (int i=start_bfidx; i<BufferTotal(); i++) { // from past to present

        // see if should close:
        // - calculate profit / stoploss
        // - closed by time

        // use avgprice for TP
        // and high-low bid from ticks to stop_loss
        if (current->sign > 0) { // long
            profit = (m_bars->at(i).avgprice - entryprice) * quantity;// # avg. current profit
            //  stop-loss profit - people buy at bid -  (low worst scenario)
            slprofit = (m_bars->at(i).bidl - entryprice) * quantity;
        }
        else { // short
            profit = (entryprice - m_bars->at(i).avgprice) * quantity;// # avg. current profif
            //  stop-loss profit - people buy at bid - (high worst scenario)
            slprofit = (entryprice - m_bars->at(i).bidh) * quantity;
        }
        // first barrier
        if (profit >= m_targetprofit) { // done classifying this signal
            xy.tdone = m_bars->uidtimes[i];
            return 1;
        }
        // second barrier
        else
        if (slprofit <= (-m_stoploss)) { // re-classify as hold
            xy.y = 0; // not a good deal, was better not to entry
            xy.tdone = m_bars->uidtimes[i];
            return 1;
        }
        // third barrier - time
        // 1.expire-time or 2.day-end        
        if (m_bars->at(i).emsc - start_time > m_expire_time ||
            m_bars->at(i).emsc > end_day) { // operations end of day
            if (profit >= 0.5*m_targetprofit ) // a minimal profit to be still valid
                xy.y = 1; // min. model class acc. must be calculated for worst scenario *this
            else
                xy.y = 0; // expired position
            xy.tdone = m_bars->uidtimes[i];
            // could include 
            return 1;
        }
        //else // should never happen - crossed to a new day 
        //    if (m_bars->at(i).time.tm_yday != day)  // i dont want this on my model
        //        return -3; // remove this 
        // same as     if (chour > m_end_hour || chour < m_start_hour)

        // increase position if
        // 1. can increase position - maxinc restriction
        // 2. is time for that
        // 3. it is from a higher band - starting w. original signal
        // pair is <buffer index, band number>
        if (nextsignidx != nextn.end() &&
            nextsignidx->first == i &&
            nextsignidx->second > last_band){
            // find entry time
            auto new_posbfidx = m_bars->SearchStime(m_bars->at(nextsignidx->first).emsc + Expert_Delay);
            if (new_posbfidx == -1) // cannot continue labelling
                return -2; //  did not find bar, not enough data in future
            // to avoid contaminating model
            // ignore the new signal if it is after
            // 1. the operational window
            // 2. expire time
            // 3. crossed to a new day
            if (m_bars->at(new_posbfidx).emsc - start_time > m_expire_time ||
                m_bars->at(new_posbfidx).emsc > end_day || // operations end of day
                m_bars->at(new_posbfidx).time.tm_mday != day) { // crossed to a new day
                nextsignidx = nextn.end(); // cannot increase any other position
                continue;
            }
            xy.ninc++;
            auto new_entryprice = m_bars->at(new_posbfidx).avgprice;
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



bool FillInXFeatures(XyPair &xypair)
{

    // find xypair.time current position on all buffers (have same size)
    // for the signal that triggered this xypair
    // uidxtime is allways sorted - dont need to find only correct by offset
    int bfidxsg = xypair.twhen - m_bfoffset;

    // cannot assembly X feature train with such an old signal
    //  not in buffer anymore - such should be removed ...
    if ( bfidxsg < 0 ) 
        return false;

    // I want to include the signal the originates this xypair
    bfidxsg += 1;
    int batch_start_idx = bfidxsg - m_batch_size;
    // not enough : bands raw signal, features in time to form a batch
    // cannot create a X feature vector and will never will
    // remove it
    // - 1 due diff cross features
    if (batch_start_idx-1 < 0 || 
        // batch_start using invalid data , cannot use it
        batch_start_idx-1 < ValidDataIdx()) 
        return false;

    xypair.X.resize(m_xtrain_dim);
    int xifeature = 0;
    // something like
    // double[Expert_BufferSize][nsignal_features] would be awesome
    // easier to copy to X also to create cross-features
    // using a constant on the second dimension is possible
    for (int timeidx = batch_start_idx; timeidx < bfidxsg; timeidx++) {
        // from past to present [bufi-batch_size:bufi+1)
        // features from band signals
        //for (int i = 0; i < m_nbands; i++, xifeature++) {
        //    xypair.X[xifeature] = m_rbandsgs[i][timeidx];
        // }
        // from m_nbands features (one per band) to 2 feature classes
        // removing one hot for every band making OR 2 classes
        // using - 1 bit to store each class
        //https://towardsdatascience.com/
        //one-hot-encoding-is-making-your-tree-based-
        //worse-heres-why-d64b282b5769 
        // positive signs on bands - class 1
        // negative signs on bands - class 2
        // since each class might be go from 0 to 1 
        // suppose [0, 1, -1] 3 bands
        // for positive class == [1, 2, 0] ==> int((0 + 1)//2) << 0 + int(1 + 1)//2 << 1 + (-1 + 1) << 2
        //                   binary              ==>       0 << 0 + 1 << 1 + 0 << 2 = 010
        // for negative class == [-1, 0, -2] ==> int(-(0 - 1)//2) << 0 + int(-(1 - 1)//2) << 1 + int(-(-1 - 1)//2) << 2)
        //                   binary              ==>       0 << 0 + 0 << 1 + 1 << 2 = 100
        xypair.X[(xifeature+0)] = 0;
        xypair.X[(xifeature+1)] = 0;
        for (int i=0; i<m_nbands; i++) {
            xypair.X[xifeature+0] += ((int) ( (int)(  (m_rbandsgs[i][timeidx]+1) ) / 2) ) << i; // positive signals
            xypair.X[xifeature+1] += ((int) ( (int)( -(m_rbandsgs[i][timeidx]-1) ) / 2) ) << i; // negative signals
        }
        xifeature+=2;
        // fracdif feature
        xypair.X[xifeature++] = m_fd_mbarp->at(timeidx);
        // time to form a bar in seconds
        double dt = (m_bars->at(timeidx).emsc - m_bars->at(timeidx).smsc) / 1000.;
        xypair.X[xifeature++] = dt;
        // number of ticks to form a bar
        double nticks = m_bars->at(timeidx).nticks;
        xypair.X[xifeature++] = nticks;
        // cross features
        // 6 - diff of times to form this bar with the previous    
        // 7 - diff of ticks to form this bar with the previous
        xypair.X[xifeature++] = dt - (m_bars->at(timeidx-1).emsc - m_bars->at(timeidx-1).smsc) / 1000.;
        xypair.X[xifeature++] = nticks - m_bars->at(timeidx-1).nticks;
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
    xypair.X[xifeature++] = xypair.band;

    return true; // suceeded
}


std::pair<std::vector<double>, std::vector<int>> GetXyvectors() {
    // pybind11 automatically casts/converts std:: types
    // vectors got to a list
    // but you can use cast to convert to a numpy array
    // needs "pybind11/stl.h"
    std::vector<double> X;
    std::vector<int> Y;  // if use push_back dont need set size
    X.resize((m_xypairs.size() * m_xtrain_dim)); // neeeded to std::copy

    size_t idx = 0;
    for (auto item = m_xypairs.begin(); item != m_xypairs.end(); item++) {
        std::copy((*item).X.begin(),
            (*item).X.end(),
            X.begin() +
            std::distance(m_xypairs.begin(), item) * m_xtrain_dim);
        Y.push_back((*item).y);
    }

    return std::make_pair(X, Y);
}


bool PythonTrainModel() {
    // create input params for Python function
    auto Xy = GetXyvectors();
    auto X = Xy.first.data();
    auto y = Xy.second.data();

    m_model.pymodel_size = pyTrainModel(X, y, m_ntraining, m_xtrain_dim,
        m_model.pymodel.data(), m_model.pymodel.capacity());
    m_model.isready = (m_model.pymodel_size > 0);

    return (m_model.pymodel_size > 0);
}

int PythonPredict(XyPair xypair){
    if (!m_model.isready)
        return -1;
    int y_pred = pyPredictwModel(xypair.X.data(), m_xtrain_dim,
        m_model.pymodel.data(), m_model.pymodel.capacity());

    return y_pred;
}


////////////////////////////////////////////////
///////////////// Python API //////////////////
///////////////////////////////////////////////


// or by Python
unixtime_ms pyAddTicks(py::array_t<MqlTick> ticks)
{
    // make a copy to be able to use fixticks (non const)
    std::vector<MqlTick> nonconst_ticks(ticks.data(), ticks.data()+ticks.size());

    return CppOnTicks(nonconst_ticks.data(), nonconst_ticks.size());
}

std::shared_ptr<py::array_t<MoneyBar>> ppymbars;

py::array_t<MoneyBar> pyGetMoneyBars() {
    MoneyBar* pbuf = (MoneyBar*)ppymbars->request().ptr;
    for (size_t idx = 0; idx<m_bars->size(); idx++)
        pbuf[idx] = m_bars->at(idx);
    return *ppymbars;
}

std::tuple<py::array, py::array, py::array_t<LbSignal>> pyGetXyvectors(){
    // pybind11 automatically casts/converts std:: types
    // vectors got to a list
    // but you can use cast to convert to a numpy array
    // needs "pybind11/stl.h"
    std::vector<double> X;
    std::vector<int> Y;  // if use push_back dont need set size
    py::array_t<LbSignal> lbsignals; // all info from every Xy pair
    X.resize((m_xypairs.size() * m_xtrain_dim)); // neeeded to std::copy
    
    lbsignals.resize({ m_xypairs.size() });
    LbSignal* pbuf = (LbSignal*)lbsignals.request().ptr;

    size_t idx = 0;
    for (auto item=m_xypairs.begin(); item != m_xypairs.end(); item++) {
        std::copy((*item).X.begin(), 
                  (*item).X.end(),
                   X.begin() + 
                   std::distance(m_xypairs.begin(), item) * m_xtrain_dim);
        Y.push_back((*item).y);
        pbuf[idx++] = (LbSignal)(*item);
    }

    py::array pyX = py::cast(X);
    pyX.resize({ (int) m_xypairs.size(), m_xtrain_dim });

    return std::make_tuple(pyX, py::cast(Y), lbsignals);
}


int pyGetXdim() {
    return m_xtrain_dim;
}


