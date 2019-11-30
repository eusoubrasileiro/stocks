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
// except for CObjectBuffer<XyPair>
std::shared_ptr<MoneyBarBuffer> m_bars; // buffer of money bars base for everything

std::vector<double> m_mlast; // moneybar.last values of last added money bars
// created from ticks
std::shared_ptr<CCTimeDayBuffer> m_times; // time buffer from moneybar time-ms
//MqlDateTime m_mqldt_now;
//MqlDateTime m_last_time; // last time after refresh

double m_moneybar_size; // R$ to form 1 money bar

// bollinger bands configuration
// upper and down and middle
int m_nbands; // number of bands
int m_bbwindow; // reference size of bollinger band indicator others
double m_bbwindow_inc;   // are multiples of m_bbwindow_inc

// feature indicators
std::shared_ptr<CFracDiffIndicator> m_fd_mbarp; // frac diff on money bar prices

// store buy|sell|hold signals for each bband - raw
std::vector<std::shared_ptr<CBandSignal>> m_rbandsgs;

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

// max number of increases on a position
int m_incmax = 3;

// python sklearn model trainning
bool m_recursive;
//sklearnModel m_model;
unsigned int m_model_refresh; // how frequent to update the model
// (in number of new training samples)
// helpers to count training samples
unsigned long m_xypair_count; // counter to help train/re-train model
unsigned long m_model_last_training; // referenced on m_xypair_count's

// For labelling using 

// round price minimum volume of symbol 100's stocks 1 for future contracts
double m_lotsmin;
// LotsMin same as SYMBOL_VOLUME_MIN
#define roundVolume(vol) int(vol/m_lotsmin)*m_lotsmin


void Initialize(int nbands, int bbwindow,
    int batch_size, int ntraining,
    double ordersize, double stoploss, double targetprofit,
    double run_stoploss, double run_targetprofit, bool recursive,
    double ticksize, double tickvalue, double moneybar_size, // R$ to form 1 money bar
    double lotsmin, int max_positions)
{
    m_lotsmin = lotsmin;
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

    // group of samples and signal vectors to train the model
    // resize buffer of training pairs
    m_xypairs.Resize(m_ntraining);
    // resize x feature vector inside it
    // but they do not exist cant resize something that doesnt exist
    // only the space available for them
    //m_model_refresh = model_refresh;

    m_rbandsgs.resize(m_nbands); // one for each band
    m_last_raw_signal.resize(m_nbands);

    // void CreateBBands()
    // raw signal storage + ctalib bbands
    double inc = m_bbwindow_inc;
    for (int i = 0; i < m_nbands; i++) { // create multiple increasing bollinger bands
        m_rbandsgs[i].reset(new CBandSignal(m_bbwindow * inc, 2.5, 2, Expert_BufferSize)); // 2-Weighted type
        // weighted seams more meaning since i know each bar
        // represents same ammount of money so have a equal weight
        inc += m_bbwindow_inc;
    }

    // CreateOtherFeatureIndicators();
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

    // update signals based on all bollinger bands
    for (int j = 0; j < m_nbands; j++) {
        result = m_rbandsgs[j]->Refresh(m_bars->m_last.data(), 0, m_bars->m_nnew) && result;
    }

    return result;
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
        for (int i = IdxNewData(); i < BufferTotal(); i++) {
            m_last_raw_signal[j] = m_rbandsgs[j]->At(i);
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


// Create target class by analysing raw band signals
// label or re-label or re-classify every signal on every band
// that are different than 0 == those are already classified
// and are useless since dont represent an entry
// those that went true receive 1 buy or -1 sell
// those that wen bad receive 0 hold
void LabelClasses() {

    // targetprofit - profit to close open position
    // amount - tick value * quantity bought in $$
    double entryprice = 0;
    int history = 0; // nothing, buy = 1, sell = -1
    int day, previous_day = 0;
    long time;
    double profit = 0;
    double quantity = 0; // number of contracts or stocks shares
    // Starts from the past (begin of buffer of signals)
    int i = 0, j;

    // current signal on some band being analysed
    double cbsignal = 0; // nothing 0, buy = 1, sell = -1
    // position of last signal analysed - time and band
    int cbsignal_i, cbsignal_j; 

    // number of increases on a position - max is m_maxinc
    int posinc = 0; 

// go to the next band signal
#define next_bsginal()  { \
          cbsignal = 0;   \
          i = cbsignal_i; \
          j = cbsignal_j; \
          posinc = 0;    }

    for (;i<BufferTotal();i++) { // from past to present
    // time or bars

      for (j=0;j<m_nbands;j++) {

          // getting fresh next signal
          if (cbsignal == 0 && m_rbandsgs[j]->At(i) != 0) {
              cbsignal = m_rbandsgs[j]->At(i);
              cbsignal_i = i;
              cbsignal_j = j;
              posinc++;
              // entryprice = m_bars->At(i).avgprice; // last avg price ?? 
              // find first bar price considering the operational time delay
              // TODO: need to write a Find or Search for m_times
              int ibar = 0; // m_times->Search(m_bars->At(i).emsc + Expert_Delay, );
              // better price based on time
              entryprice = m_bars->At(ibar).avgprice;
              m_xypairs.Add( //  save this buy or sell 
                  XyPair(cbsignal, m_times->At(cbsignal_i), cbsignal_j)); 
              quantity = roundVolume(m_ordersize / entryprice);
              // then jump forward to where the buy/sell really takes place
              j = 0; i = ibar;
          }
          else // cbsignal is -1 or +1
          {
            // first take care of what you have
            // calculate profit / stoploss to see if should close

            // use avgprice or p10, 50, 90 or ohlc?
            // or even ticks ask, bid ohlc? -- better
            if(cbsignal > 0) 
                profit = (m_bars->At(i).avgprice - entryprice) * quantity;// # current profit
            else
                profit = (entryprice - m_bars->At(i).avgprice) * quantity;// # current profif

            // also need to check expire by time / day -- tripple barrier
            // first barrier
            if (profit >= m_targetprofit){ // done classifying this signal
                next_bsginal();
            }
            // second barrier
            else 
            if (profit <= (-m_stoploss)) { // re-classify as hold
                m_xypairs.Last()->y = 0; // not a good deal, was better not to entry
                next_bsginal();
            }
            // third barrier
            else
            // get more positions only if in same direction
            // and allowed
            if (m_rbandsgs[j]->At(i) == cbsignal && posinc < m_incmax){
                posinc++;
            }

          }


      }



    }
    // CBuffer<XyPair> m_xypairs

    //// OR
    //// For not adding again the same signal
    //// Starts where it stop the last time
    //if (xypairs.Count() > 0) {
    //    int last_buff_index = m_times->QuickSearch(xypairs.GetData(0).time);
    //    if (last_buff_index < 0) {
    //        //Print("This was not implemented and should never happen!");
    //        return;
    //    }
    //    // next signal after this!
    //    i = last_buff_index + 1;
    //}
    //for (; i < BufferTotal(); i++) { // from past to present
    //    time = m_times->At(i).ms;
    //    day = m_times->At(i).day; // unique day identifier
    //    if(day != previous_day){ // a new day reset everything
    //        if (history == 1 || history == -1) {
    //            // the previous batch o/f signals will not be saved. I don't want to train with that
    //            xypairs.RemoveLast();
    //        }
    //        entryprice = 0;
    //        history = 0; //# 0= nothing, buy = 1, sell = -1
    //        previous_day = day;
    //    }
    //    // first take care of what you have
    //    // calculate profit / stoploss to see if should close
    //    if(history != 0){
    //        if(history == 1){ // it was a buy
    //          profit = (m_bars->At(i).avgprice - entryprice) * quantity;// # current profit
    //          if(profit >= m_targetprofit){
    //              // xypairs.Last().y = 1 // a real (buy) index class nothing to do
    //              history = 0;
    //          }
    //          else
    //          if (profit <= (-m_stoploss)){ // reclassify the previous buy as hold
    //              xypairs.Last().y = 0; // not a good deal, was better not to entry
    //              history = 0;
    //          }
    //       }
    //       else{ // a sell -1
    //         profit = (entryprice - m_bars->At(i).avgprice) * quantity;// # current profit
    //         if (profit >= m_targetprofit) {
    //             // xypairs.Last().y = -1 // a real (sell) index class nothing to do
    //             history = 0;
    //         }
    //         else
    //         if (profit <= (-m_stoploss)) { // reclassify the previous sell as hold
    //             xypairs.Last().y = 0; // not a good deal, was better not to entry
    //             history = 0;
    //         }
    //      }
    //   }
    //   // get more positions or open a new one
    //   else {
    //     if (bandsg[i] == 1) {
    //         switch (history) {
    //           case 0:
    //               // new buy signal
    //               entryprice = m_bars[i].last; // last negotiated price
    //               xypairs.Add(new XyPair(1, m_times[i], band_number)); //  save this buy
    //               quantity = roundVolume(m_ordersize / entryprice);
    //               history = 1;
    //           break;
    //           case 1:
    //               // another buy in sequence
    //               // maybe it is dancing around this band boundary
    //               entryprice = m_bars[i].last;
    //               xypairs.Add(new XyPair(1, m_times[i], band_number)); //  save this buy
    //               quantity = roundVolume(m_ordersize / entryprice);
    //               history = 1;
    //           break;
    //           case -1;
    //               // a sell after a buy with no profit
    //               // the previous batch of signals will be saved with this class (hold)
    //               xypairs.Last().y = 0; // reclassify the previous buy/sell as hold
    //           break;
    //         }
    //   }
    //    // 4 main cases
    //    // signal  1 - history = 0 || != 0  (total 1/0, 1/-1,, 1/1) = 3
    //    // signal -1 - history = 0 || != 0  (total -1/0, -1/1,, -1/-1) = 3
    //    // signal  0 - history = 1          (total 0/1) = 1
    //    // signal  0 - history = -1         (total 0/-1) = 1
    //    // useless : signal 0 - history = 0 - total total 3x3 = 9

    //    else
    //        if (bandsg[i] == -1) {
    //            if (history == 0) {
    //                entryprice = m_bars[i].last;
    //                xypairs.Add(new XyPair(-1, m_times[i], band_number)); //  save this sell
    //            }
    //            else { // another sell in sequence
    //                 // or after a buy  in the same minute
    //                // the previous batch of signals will be saved with this class (hold)
    //                xypairs.Last().y = 0; // reclassify the previous buy/sell as hold
    //                // new buy sell
    //                // maybe we can have profit here
    //                // in a fast movement
    //                entryprice = m_bars[i].last;
    //                xypairs.Add(new XyPair(-1, m_times[i], band_number)); //  save this buy
    //            }
    //            quantity = roundVolume(m_ordersize / entryprice);
    //            history = -1;
    //        }
    //        else // signal 0 history 1
    //            if (history == 1) { // equivalent to + && bandsg[i] == 0
    //                profit = (m_bars[i].last - entryprice) * quantity;// # current profit
    //                if (profit >= m_targetprofit) {
    //                    // xypairs.Last().y = 0 // a real (buy) index class nothing to do
    //                    history = 0;
    //                }
    //                else
    //                    if (profit <= (-m_stoploss)) { // reclassify the previous buy as hold
    //                        xypairs.Last().y = 0; // not a good deal, was better not to entry
    //                        history = 0;
    //                    }
    //            }
    //            else // signal 0 history -1
    //                if (history == -1) { // equivalent to + && bandsg[i] == 0
    //                    profit = (entryprice - m_bars[i].last) * quantity;// # current profit
    //                    if (profit >= m_targetprofit) {
    //                        // xypairs.Last().y = 0 // a real (sell) index class nothing to do
    //                        history = 0;
    //                    }
    //                    else
    //                        if (profit <= (-m_stoploss)) { // reclassify the previous sell as hold
    //                            xypairs.Last().y = 0; // not a good deal, was better not to entry
    //                            history = 0;
    //                        }
    //                }
    //    // else signal 0 history 0
    //}
    ////# reached the end of data buffer did not close one buy previouly open
    //if (history == 1 || history == -1) // don't know about the future cannot train with this guy
    //    xypairs.RemoveLast();
}



//void CreateXFeatureVectors(CObjectBuffer<XyPair>& xypairs)
//{ // using the pre-filled buffer of y target classes
//  // assembly the X array of features for it
//  // from more recent to oldest
//  // if it is already filled (ready) stop
//    for (int i = 0; i < xypairs.Count(); i++) {
//        // no need to assembly any other if this is already ready
//        // olders will also be ready
//        if (!CreateXFeatureVector(xypairs.GetData(i)))
//            xypairs.RemoveData(i);
//    }
//
//}
//
//bool CreateXFeatureVector(XyPair& xypair)
//{
//    if (xypair.isready) // no need to assembly
//        return true;
//
//    // find xypair.time current position on all buffers (have same size)
//    // time is allways sorted
//    int bufi = m_times.QuickSearch(xypair.time);
//
//    // cannot assembly X feature train with such an old signal not in buffer anymore
//    // such should be removed ...
//    if (bufi < 0) { // not found -1 : this should never happen?
//        Print("ERROR: This should never happen!");
//        return false;
//    }
//    // not enough : bands raw signal, features in time to form a batch
//    // cannot create a X feature vector and will never will
//    // remove it
//    if (bufi - m_batch_size < 0)
//        return false;
//
//    if (ArraySize(xypair.X) < m_xtrain_dim)
//        xypair.Resize(m_xtrain_dim);
//
//    int xifeature = 0;
//    // something like
//    // double[Expert_BufferSize][nsignal_features] would be awesome
//    // easier to copy to X also to create cross-features
//    // using a constant on the second dimension is possible
//    //bufi = BufferTotal()-1-bufi;
//    bufi -= m_batch_size;
//    int batch_end_idx = bufi + m_batch_size;
//    for (int timeidx = bufi; timeidx < batch_end_idx; timeidx++) {
//        // from past to present
//        // features from band signals
//        for (int i = 0; i < m_nbands; i++, xifeature++) {
//            xypair.X[xifeature] = m_rbandsgs[i][timeidx];
//        }
//        // fracdif feature
//        xypair.X[xifeature++] = m_fd_mbarp[timeidx];
//        // some indicators might contain EMPTY_VALUE == DBL_MAX values
//        // verified volumes with EMPTY VALUES but I suppose that also might happen
//        // with others in this case better drop the training sample?
//        // sklearn throws an exception because of that
//        // anyhow that's not a problem for the code since the python exception
//        // is catched behind the scenes and a valid model will only be available
//        // once correct samples are available
//    }
//    xypair.isready = true;
//    return true; // suceeded
//}
//



////////////////////////////////////////////////
///////////////// Python API //////////////////
///////////////////////////////////////////////


extern py::array_t<MoneyBar> *pymbars;

// or by Python
int pyAddTicks(py::array_t<MqlTick> ticks)
{
    return AddTicks(ticks.data(), ticks.size());
}

std::shared_ptr<py::array_t<MoneyBar>> ppymbars;

py::array_t<MoneyBar> pyGetMoneyBars() {
    MoneyBar* pbuf = (MoneyBar*)ppymbars->request().ptr;
    for (size_t idx = 0; idx<Expert_BufferSize; idx++)
        pbuf[idx] = m_bars->m_data[idx];
    return *ppymbars;
}
