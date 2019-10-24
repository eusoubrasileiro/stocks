#include "..\..\Util.mqh"
#include "..\..\Buffers.mqh"
#include "..\..\indicators\CTalibIndicators.mqh"
#include "..\..\indicators\CSpecialIndicators.mqh"
#include "XyVectors.mqh"
#include "..\bbands\BbandsPython.mqh"
#include "..\..\datastruct\Bars.mqh"
#include "..\..\datastruct\Time.mqh"
#include "..\..\datastruct\CBufferMqlTicks.mqh"

// number of samples needed/used for training 5*days?
const int                Expert_BufferSize      = 5000; // indicators buffer needed
const int                Expert_MaxFeatures     = 100; // max features allowed
const double             Expert_MoneyBar_Size   = 500e3; // R$ to form 1 money bar
const double             Expert_Fracdif         = 0.6; // fraction for fractional difference
const double             Expert_Fracdif_Window  = 512; // window size fraction fracdif


// Using Dolar/Real Bars and consequently tick data
// only for tick time-frame
class CExpertRBarBands : public CExpertX
{
    // Base Data
    CBufferMqlTicks *m_ticks; // buffer of ticks w. control to download unique ones.
    MoneyBarBuffer *m_bars; // buffer of money bars base for everything
    double m_mlast[]; // moneybar.last values of last added money bars
    // created from ticks
    CTimeDayBuffer *m_time_ms; // time buffer from moneybar time-ms
    //MqlDateTime m_mqldt_now;
    //MqlDateTime m_last_time; // last time after refresh

    // bollinger bands configuration
    CTaBBANDS *m_bands[]; // array of indicators for each bollinger band
    // upper and down and middle
    int m_nbands; // number of bands
    int m_bbwindow; // reference size of bollinger band indicator others
    double m_bbwindow_inc;   // are multiples of m_bbwindow_inc

    // feature indicators
    CFracDiffIndicator *m_fd_mbarp; // frac diff on money bar prices

    // store buy|sell|hold signals for each bband
    CBuffer<int> m_raw_signal[];

    int m_last_raw_signal[]; // last raw signal in all m_nbands

    // features and training vectors
    // array of buffers for each signal feature
    CBuffer<double> m_signal_features[];
    int m_nsignal_features;
    int m_batch_size;
    int m_ntraining;
    int m_xtrain_dim; // dimension of x train vector
    CObjectBuffer<XyPair> m_xypairs;

    // execution of positions and orders
    // profit or stop loss calculation for training
    double m_ordersize;
    double m_stoploss; // stop loss value in $$$ per order
    double m_targetprofit; //targetprofit in $$$ per order
    // profit or stop loss calculation for execution
    double m_run_stoploss; // stop loss value in $$$ per order
    double m_run_targetprofit; //targetprofit in $$$ per order

    // python sklearn model trainning
    bool m_recursive;
    sklearnModel m_model;
    unsigned int m_model_refresh; // how frequent to update the model
    // (in number of new training samples)
    // helpers to count training samples
    unsigned long m_xypair_count; // counter to help train/re-train model
    unsigned long m_model_last_training; // referenced on m_xypair_count's

  public:

   void CExpertRBarBands(void){
      m_bbwindow_inc = 0.5;
      m_model = new sklearnModel;
      m_xypair_count = 0;
      m_model_last_training =0;
   };

  void Initialize(int nbands, int bbwindow,
        int batch_size, int ntraining,
        double ordersize, double stoploss, double targetprofit,
        double run_stoploss, double run_targetprofit, bool recursive,
        int model_refresh=15)
    {
    // create money bars
    m_ticks = new CBufferMqlTicks(m_symbol.Name());
    m_bars = new MoneyBarBuffer(m_symbol.TickValue(),  Expert_MoneyBar_Size);


    m_recursive = recursive;
      // Call only after init indicators
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
    // total of indicar features is 1
    // 1 - fracdiff on prices
    m_nsignal_features = 1;
    m_xtrain_dim = m_nsignal_features*m_batch_size;

    // allocate array of bbands indicators
    ArrayResize(m_bands, m_nbands);

    // group of samples and signal vectors to train the model
    // resize buffer of training pairs
    m_xypairs = new CObjectBuffer<XyPair>;
    m_xypairs.Resize(m_ntraining);
    // resize x feature vector inside it
    // but they do not exist cant resize something that doesnt exist
    // only the space available for them
    m_model_refresh = model_refresh;

    ArrayResize(m_raw_signal, m_nbands); // one for each band
    ArrayResize(m_last_raw_signal, m_nbands);

    CreateBBands();
    CreateOtherFeatureIndicators();
  }

  // start index and count of new bars that just arrived
  bool Refresh(int start, int count)
  {
    ArrayResize(m_mlast, count);
    // money bars
    for(int i=start; i<count; i++){
        m_mlast[i] = m_bars.m_data[i].last; // moneybar.last price
        // insert current times in the times buffer
        // must be here so all buffers are aligned
        m_time_ms.Add(m_bars.m_data[i].time_msc); //moneybar.time_msc
    }
    //if(!CExpert::Refresh()) // useless on Tick timeframe
    // features indicators
    // fracdif on bar prices
    m_fd_mbarp.Refresh(m_mlast, start, count);

    // update all bollinger bands indicators
    for(int j=0; j<m_nbands; j++){
        m_bands[j].Refresh(m_mlast, start, count);
    }

   // called after m_ticks.Refresh() garantees not called twice
    RefreshRawBandSignals();

    return(true);
  }

  // simpler to use OnTimer instead of onTick
  // using a timer of 1 second
  void OnTimer(void) // overwrite it - will be called every 1 second
   {
    if(m_ticks.Refresh() > 0 &&
      m_bars.AddTicks(m_ticks.m_data,
        m_ticks.beginNewTicks(), m_ticks.nNew()) > 0)
    { // some new ticks arrived and new bars created
      // at least one new money bar created
      // refresh whatever possible with new bars
      if(Refresh(m_bars.Size()-m_bars.m_added, m_bars.m_added)){
        // sucess of updating everything w. new data
          verifyEntry();
      }
    }
    // check to see if we should close any position
    if(SelectPosition()){ // if any open position
        CloseOpenPositionsbyTime();
        CheckTrailingStop();
    }
  }

  void verifyEntry(){
    //--- updated quotes and indicators
      if(!isInsideDay())
        return; // no work outside day scope

    // has an entry signal in any band? on the last raw signal
    if(lastRawSignal())
    {
        int m_xypair_count_old = m_xypairs.Size();
        // only when a new entry signal recalc classes
        // only when needed
        CreateYTargetClasses();
        CreateXFeatureVectors(m_xypairs);
        // add number of xypairs added (cannot decrease)
        m_xypair_count += (m_xypairs.Size()-m_xypair_count_old);
        // update/train model if needed/possible
        if(m_xypairs.Size() >= m_ntraining){
          // first time training
          if(!m_model.isready){
           // time to train the model for the first time
            m_model.isready = PythonTrainModel();
          }
          else // time to update the model?
          if((m_xypair_count - m_model_last_training) >= m_model_refresh){
          // if diference in training samples to the last training is bigger than
          // refresh criteria - time to train again
            m_model.isready = PythonTrainModel();
          }
          // record when last training happened
          m_model_last_training = m_xypair_count;
        }
        // Call Python if model is already trainned
        if(m_model.isready){
          // x forecast vector
          XyPair Xforecast = new XyPair;
          Xforecast.time = m_time_ms.m_last;
          CreateXFeatureVector(Xforecast);
          int y_pred = PythonPredict(Xforecast);
          BuySell(y_pred);
          if(m_recursive && (y_pred == 0 || y_pred == 1 | y_pred == -1)){
            // modify the band raw signals by the ones predicted
            for(int i=0; i<m_nbands; i++){
              int signal = m_raw_signal[i].GetData(0);
              signal *= (y_pred==0)? 0: 1; // change from -1 to 1 or to 0
              m_raw_signal[i].SetData(0, signal);
            }
          }
        }
    }



  }

  ~CExpertRBarBands(){}; // dont care for now memory leaks on expert execution

  protected:

  void CreateYTargetClasses();
  void BandCreateYTargetClasses(CBuffer<int> &bandsg_raw,
        CObjectBuffer<XyPair> &xypairs, int band_number);
  void CreateXFeatureVectors(CObjectBuffer<XyPair> &xypairs);
  bool CreateXFeatureVector(XyPair &xypair);
  void CreateOtherFeatureIndicators();
  void BuySell(int sign);

  bool PythonTrainModel();
  int  PythonPredict(XyPair &xypair);

  // return true if has any entry signal
  // verify an entry signal in any band
  // only call when there's at least one signal
  // stored in the buffer of raw signal bands
  bool lastRawSignal(){
    bool result = false;
    // if(m_raw_signal) will not check null (not now)
    for(int i=0; i<m_nbands; i++){
        m_last_raw_signal[i] = m_raw_signal[i].GetData(0);
        if(m_last_raw_signal[i] != 0)
            result = true;
    }
    return result;
  }

  void CreateBBands(){
      // raw signal storage
      for(int j=0; j<m_nbands; j++){
          m_raw_signal[j] = new CBuffer<int>;
          m_raw_signal[j].Resize(Expert_BufferSize);
      }

      // ctalib bbands
      double inc=m_bbwindow_inc;
      for(int i=0; i<m_nbands; i++){ // create multiple increasing bollinger bands
          m_bands[i] = new CTaBBANDS(m_bbwindow*inc, 2.5, 1); // 1-Exponential type
          m_bands[i].Resize(Expert_BufferSize);
          inc += m_bbwindow_inc;
      }
  }

  void RefreshRawBandSignals(void){
    // should be called only once per refresh
    // use the last added samples
    for(int j=0; j<m_nbands; j++){
        //    Based on a bollinger band defined by upper-band and lower-band
        //    return signal:
        //        buy   1 : crossing down-outside it's buy
        //        sell -1 : crossing up-outside it's sell
        //        hold  0 : nothing usefull happend
         // int i = m_bands[0].Size() - m_bands[0].m_calculated;
        int i = m_bars.m_added; // number of added bar is
        // same of number of new indicator samples due refresh() true
        // of indicators
        for(; i<m_bands[0].Size(); i++){
            if( m_bars[i].last >= m_bands[j].m_upper[i])
                m_raw_signal[i].Add(-1.); // sell
            else
            if( m_bars[i].last <= m_bands[j].m_down[i])
                m_raw_signal[i].Add(+1.); // buy
            else
                m_raw_signal[i].Add(0); // nothing
        }
    }
  }


  // Global Buffer Size (all buffers MUST have same size)
  // that's why they all use same ResizeBuffer code
  // Real Buffer Size since Expert_BufferSize is just
  // an initilization parameter and Resize functions make
  // buffer a little bigger
  int BufferSize(){ return m_time_ms.BufferSize();}
  int BufferTotal(){ return m_raw_signal[0].Size(); }


};


void CExpertRBarBands::CreateOtherFeatureIndicators(){
    // only fracdiff on prices
    m_fd_mbarp = new CFracDiffIndicator(Expert_Fracdif_Window, Expert_Fracdif);
    m_fd_mbarp.Resize(Expert_BufferSize);
}


// Create target class by analysing raw bollinger band signals
// those that went true receive 1 buy or -1 sell
// those that wen bad receive 0 hold
// A target class exist for each band.
// An essemble of bands together and summed are a better classifier.
// A day integer identifier must exist prior call.
// That is used to blocking positions from passing to another day.
void CExpertRBarBands::CreateYTargetClasses(){
    for(int i=0; i<m_nbands; i++){
      BandCreateYTargetClasses(m_raw_signal[i], m_xypairs, i);
    }
}

void CExpertRBarBands::BandCreateYTargetClasses(CBuffer<int> &bandsg_raw,
        CObjectBuffer<XyPair> &xypairs, int band_number)
{
      // buy at ask = High at minute time-frame (being pessimistic)
      // sell at bid = Low at minute time-frame
      // classes are [0, 1] = [hold, buy]
      // stoploss - to not loose much money (positive)
      // targetprofit - profit to close open position
      // amount - tick value * quantity bought in $$
      double entryprice = 0;
      int history = 0; // nothing, buy = 1, sell = -1
      int day, previous_day = 0;
      long time;
      double profit = 0;
      double quantity = 0; // number of contracts or stocks shares
      // Starts from the past (begin of buffer of signals)
      int i=0;// bandsg_raw.Size()-1;
      // OR
      // For not adding again the same signal
      // Starts where it stop the last time
      if(xypairs.Size() > 0){
        int last_buff_index = m_time_ms.QuickSearch(xypairs.GetData(0).time);
        if(last_buff_index < 0){
            Print("This was not implemented and should never happen!");
            return;
        }
        // next signal after this!
        i = last_buff_index + 1;
      }
      // net mode ONLY
      for(; i<bandsg_raw.Size(); i--){ // from past to present
          time = m_time_ms[i].ms;
          day = m_time_ms[i].day; // unique day identifier
          if(day != previous_day){ // a new day reset everything
              if(history == 1 || history == -1){
                  // the previous batch o/f signals will not be saved. I don't want to train with that
                  xypairs.RemoveLast();
              }
              entryprice = 0;
              history = 0; //# 0= nothing, buy = 1, sell = -1
              previous_day = day;
          }
          // 4 main cases
          // signal  1 - history = 0 || != 0  (total 1/0, 1/-1,, 1/1) = 3
          // signal -1 - history = 0 || != 0  (total -1/0, -1/1,, -1/-1) = 3
          // signal  0 - history = 1          (total 0/1) = 1
          // signal  0 - history = -1         (total 0/-1) = 1
          // useless : signal 0 - history = 0 - total total 3x3 = 9
          if(bandsg_raw[i] == 1){
              if(history == 0){
                  entryprice = m_bars[i].last; // last negotiated price
                  xypairs.Add(new XyPair(1, m_time_ms[i], band_number)); //  save this buy
              }
              else{ // another buy in sequence
                  // or after a sell in the same minute
                  // the previous batch of signals will be saved with this class (hold)
                  xypairs.Last().y = 0; // reclassify the previous buy/sell as hold
                  // new buy signal
                  // maybe we can have profit here
                  // in a fast movement
                  entryprice = m_bars[i].last;
                  xypairs.Add(new XyPair(1, m_time_ms[i], band_number)); //  save this buy
              }
              quantity = roundVolume(m_ordersize/entryprice);
              history=1;
          }
          else
          if(bandsg_raw[i] == -1){
              if(history == 0){
                  entryprice = m_bars[i].last;
                  xypairs.Add(new XyPair(-1, m_time_ms[i], band_number)); //  save this sell
              }
              else{ // another sell in sequence
                   // or after a buy  in the same minute
                  // the previous batch of signals will be saved with this class (hold)
                  xypairs.Last().y = 0; // reclassify the previous buy/sell as hold
                  // new buy sell
                  // maybe we can have profit here
                  // in a fast movement
                  entryprice = m_bars[i].last;
                  xypairs.Add(new XyPair(-1, m_time_ms[i], band_number)); //  save this buy
              }
              quantity = roundVolume(m_ordersize/entryprice);
              history=-1;
          }
          else // signal 0 history 1
          if(history == 1){ // equivalent to + && bandsg_raw[i] == 0
              profit = (m_bars[i].last-entryprice)*quantity;// # current profit
              if(profit >= m_targetprofit){
                  // xypairs.Last().y = 0 // a real (buy) index class nothing to do
                  history = 0;
              }
              else
              if(profit <= (-m_stoploss)){ // reclassify the previous buy as hold
                  xypairs.Last().y =0; // not a good deal, was better not to entry
                  history = 0;
              }
          }
          else // signal 0 history -1
          if(history == -1){ // equivalent to + && bandsg_raw[i] == 0
              profit = (entryprice-m_bars[i].last)*quantity;// # current profit
              if(profit >= m_targetprofit){
                  // xypairs.Last().y = 0 // a real (sell) index class nothing to do
                  history = 0;
              }
              else
              if(profit <= (-m_stoploss)){ // reclassify the previous sell as hold
                  xypairs.Last().y =0; // not a good deal, was better not to entry
                  history = 0;
              }
          }
          // else signal 0 history 0
      }
      //# reached the end of data buffer did not close one buy previouly open
      if(history == 1 || history == -1) // don't know about the future cannot train with this guy
          xypairs.RemoveLast();
}
