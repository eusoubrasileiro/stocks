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
const int                Expert_BufferSize      = 100e3; // indicators buffer needed
const int                Expert_MaxFeatures     = 100; // max features allowed - not used
const double             Expert_MoneyBar_Size   = 250e3; // R$ to form 1 money bar
const double             Expert_Fracdif         = 0.6; // fraction for fractional difference
const double             Expert_Fracdif_Window  = 512; // window size fraction fracdif


// Using Dolar/Real Bars and consequently tick data
// only for tick time-frame
class CExpertRBarBands : public CExpertX
{
    // Base Data
    CBufferMqlTicks *m_ticks; // buffer of ticks w. control to download unique ones.

    // All Buffer-Derived classes bellow have indexes alligned
    // except for CObjectBuffer<XyPair>
    MoneyBarBuffer *m_bars; // buffer of money bars base for everything
    double m_mlast[]; // moneybar.last values of last added money bars
    // created from ticks
    CTimeDayBuffer *m_times; // time buffer from moneybar time-ms
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
    CBuffer<int> *m_raw_signal[];

    int m_last_raw_signal[]; // last raw signal in all m_nbands
    int m_last_raw_signal_index; // related to m_bars buffer on refresh()

    // features and training vectors
    // array of buffers for each signal feature
    //CBuffer<double> *m_signal_features[];
    int m_nsignal_features;
    int m_batch_size;
    int m_ntraining;
    int m_xtrain_dim; // dimension of x train vector
    CObjectBuffer<XyPair> *m_xypairs;

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
      m_last_raw_signal_index = -1;
   };

  void Initialize(int nbands, int bbwindow,
        int batch_size, int ntraining,
        double ordersize, double stoploss, double targetprofit,
        double run_stoploss, double run_targetprofit, bool recursive,
        int model_refresh=15)
    {
    // create money bars
    m_ticks = new CBufferMqlTicks(m_symbol.Name());
    m_ticks.Resize(Expert_BufferSize);
    m_bars = new MoneyBarBuffer(m_symbol.TickValue(),     
                    m_symbol.TickSize(), Expert_MoneyBar_Size);
    m_bars.Resize(Expert_BufferSize);
    m_times = new CTimeDayBuffer();
    m_times.Resize(Expert_BufferSize);

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
    int end = start+count;
    ArrayResize(m_mlast, count);
    // money bars
    for(int i=start; i<end; i++){
        m_mlast[i-start] = m_bars.m_data[i].last; // moneybar.last price
        // insert current times in the times buffer
        // must be here so all buffers are aligned
        m_times.Add(m_bars.m_data[i].time_msc); //moneybar.time_msc
    }
    bool result = true;
    
    // all bellow must be called to maintain alligment of buffers
    // by creating empty samples
    
    // features indicators
    // fracdif on bar prices order of && matter
    // first is what you want to do and second what you want to try
    result = m_fd_mbarp.Refresh(m_mlast, 0, count) && result;

    // update all bollinger bands indicators
    for(int j=0; j<m_nbands; j++){
        result = m_bands[j].Refresh(m_mlast, 0, count) && result;
    }

   // called after m_ticks.Refresh() and Refreshs() above
   // garantes not called twice and bband indicators available
    RefreshRawBandSignals(start, end);

    return result;
  }

  // will be called every < 1 second
  // OnTick + OnTimer garantee a better refresh rate
  // than 1 second
  // Since OnTick not enquees calls
  // OnTick is not called again if the first OnTick
  // has not being processed yet
  void CheckTicks(void)
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
    // update indicators (trailing stop) 
    // m_symbol.RefreshRates();
    m_indicators.Refresh();

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

    // has an entry signal in any band? on the lastest
    // raw signals added bars
    if(lastRawSignals() != -1)
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
          Xforecast.time = m_times[m_last_raw_signal_index];
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

  // verify an entry signal in any band
  // on the last m_added bars
  // get the latest signal
  // return index on buffer of existent entry signal
  // only if more than one signal
  // all in the same direction
  // otherwise returns -1
  // only call when there's at least one signal
  // stored in the buffer of raw signal bands
  int lastRawSignals(){
    int begin_added = m_bars.BeginAdded();
    int count = m_bars.m_added;
    int direction = 0;
    m_last_raw_signal_index = -1;

    for(int i=begin_added; i<count; i++){
      for(int j=0; j<m_nbands; j++){
          m_last_raw_signal[j] = m_raw_signal[j][i];
          if(m_last_raw_signal[j] != 0){
            if(direction==0){
                direction = m_last_raw_signal[j];
                m_last_raw_signal_index = i;
            }
            else
            // two signals with oposite directions on last added bars
            // lets keep it simple now - lets ignore them
            if(m_last_raw_signal[j] != direction){
                m_last_raw_signal_index = -1;
                return -1;
            }
            // same direction only update index to get last
            m_last_raw_signal_index = i;
          }
      }
    }
    return m_last_raw_signal_index;
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

  void RefreshRawBandSignals(int start, int end){
    // should be called only once per refresh
    // use the last added samples
    // same of number of new indicator samples due refresh() true
    // of indicator
    for(int j=0; j<m_nbands; j++){
        //    Based on a bollinger band defined by upper-band and lower-band
        //    return signal:
        //        buy   1 : crossing down-outside it's buy
        //        sell -1 : crossing up-outside it's sell
        //        hold  0 : nothing usefull happend
        for(int i=start; i<end; i++){
            if( m_bars[i].last >= m_bands[j].m_upper[i])
                m_raw_signal[j].Add(-1.); // sell
            else
            if( m_bars[i].last <= m_bands[j].m_down[i])
                m_raw_signal[j].Add(+1.); // buy
            else
                m_raw_signal[j].Add(0); // nothing
        }
    }
  }


  // Global Buffer Size (all buffers MUST have same size)
  // that's why they all use same ResizeBuffer code
  // Real Buffer Size since Expert_BufferSize is just
  // an initilization parameter and Resize functions make
  // buffer a little bigger
  int BufferSize(){ return m_times.BufferSize();}
  int BufferTotal(){ return m_times.Size(); }


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
        int last_buff_index = m_times.QuickSearch(xypairs.GetData(0).time);
        if(last_buff_index < 0){
            Print("This was not implemented and should never happen!");
            return;
        }
        // next signal after this!
        i = last_buff_index + 1;
      }
      // net mode ONLY
      for(; i<bandsg_raw.Size(); i--){ // from past to present
          time = m_times[i].ms;
          day = m_times[i].day; // unique day identifier
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
                  xypairs.Add(new XyPair(1, m_times[i], band_number)); //  save this buy
              }
              else{ // another buy in sequence
                  // or after a sell in the same minute
                  // the previous batch of signals will be saved with this class (hold)
                  xypairs.Last().y = 0; // reclassify the previous buy/sell as hold
                  // new buy signal
                  // maybe we can have profit here
                  // in a fast movement
                  entryprice = m_bars[i].last;
                  xypairs.Add(new XyPair(1, m_times[i], band_number)); //  save this buy
              }
              quantity = roundVolume(m_ordersize/entryprice);
              history=1;
          }
          else
          if(bandsg_raw[i] == -1){
              if(history == 0){
                  entryprice = m_bars[i].last;
                  xypairs.Add(new XyPair(-1, m_times[i], band_number)); //  save this sell
              }
              else{ // another sell in sequence
                   // or after a buy  in the same minute
                  // the previous batch of signals will be saved with this class (hold)
                  xypairs.Last().y = 0; // reclassify the previous buy/sell as hold
                  // new buy sell
                  // maybe we can have profit here
                  // in a fast movement
                  entryprice = m_bars[i].last;
                  xypairs.Add(new XyPair(-1, m_times[i], band_number)); //  save this buy
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



void CExpertRBarBands::CreateXFeatureVectors(CObjectBuffer<XyPair> &xypairs)
{ // using the pre-filled buffer of y target classes
  // assembly the X array of features for it
  // from more recent to oldest
  // if it is already filled (ready) stop
  for(int i=0; i<xypairs.Size(); i++){
    // no need to assembly any other if this is already ready
    // olders will also be ready
    if(!CreateXFeatureVector(xypairs.GetData(i)))
        xypairs.RemoveData(i);
  }

}

bool CExpertRBarBands::CreateXFeatureVector(XyPair &xypair)
{
  if(xypair.isready) // no need to assembly
    return true;

  // find xypair.time current position on all buffers (have same size)
  // time is allways sorted
  int bufi = m_times.QuickSearch(xypair.time);

  // cannot assembly X feature train with such an old signal not in buffer anymore
  // such should be removed ...
  if(bufi < 0){ // not found -1 : this should never happen?
    Print("ERROR: This should never happen!");
    return false;
  }
  // not enough : bands raw signal, features in time to form a batch
  // cannot create a X feature vector and will never will
  // remove it
  if( bufi - m_batch_size < 0)
    return false;

  if(ArraySize(xypair.X) < m_xtrain_dim)
    xypair.Resize(m_xtrain_dim);

  int xifeature = 0;
  // something like
  // double[Expert_BufferSize][nsignal_features] would be awesome
  // easier to copy to X also to create cross-features
  // using a constant on the second dimension is possible
  bufi = BufferTotal()-1-bufi;
  int batch_end_idx = bufi+m_batch_size;
  for(int timeidx=bufi; timeidx<batch_end_idx; timeidx++){
    // from past to present
    // fracdif feature
    //for(int i=0; i<m_nbands; i++, xifeature++)
    xypair.X[xifeature] = m_fd_mbarp[timeidx];
    // some indicators might contain EMPTY_VALUE == DBL_MAX values
    // verified volumes with EMPTY VALUES but I suppose that also might happen
    // with others in this case better drop the training sample?
    // sklearn throws an exception because of that
    // anyhow that's not a problem for the code since the python exception
    // is catched behind the scenes and a valid model will only be available
    // once correct samples are available
  }
  xypair.isready = true;
  return true; // suceeded
}



bool CExpertRBarBands::PythonTrainModel(){
  // create input params for Python function
  double X[]; // 2D as 1D strided array less code to call python
  int y[];
  XyPair xypair;
  ArrayResize(X, m_ntraining*m_xtrain_dim);
  ArrayResize(y, m_ntraining);
  for(int i=0; i<m_ntraining;i++){ // get the most recent
    // X order doesnt matter pattern identification
     xypair = m_xypairs.GetData(i);
     ArrayCopy(X, xypair.X, i*m_xtrain_dim, 0, m_xtrain_dim);
     y[i] = xypair.y;
   }
   m_model.pymodel_size = pyTrainModel(X, y, m_ntraining, m_xtrain_dim,
                  m_model.pymodel, m_model.MaxSize());
  //Print(")
  ArrayFree(X);
  ArrayFree(y);
  return (m_model.pymodel_size > 0);
}

int CExpertRBarBands::PythonPredict(XyPair &xypair){
  if(!m_model.isready || !xypair.isready)
    return -5;
  // create input params for Python function
  double X[]; // 2D as 1D strided array less code to call python
  ArrayResize(X, m_xtrain_dim);
  ArrayCopy(X, xypair.X, 0, 0, m_xtrain_dim);
  int y_pred = pyPredictwModel(X, m_xtrain_dim,
                  m_model.pymodel, m_model.pymodel_size);
  ArrayFree(X);
  return y_pred;
}

void CExpertRBarBands::BuySell(int sign){
    double amount, sl, tp;
    if(sign == 1){
        amount = roundVolume(m_ordersize/m_symbol.Ask());
        sl = m_symbol.NormalizePrice((double) ((amount*m_symbol.Ask())-m_run_stoploss)/amount);
        tp = m_symbol.NormalizePrice((double) ((amount*m_symbol.Ask())+m_run_targetprofit)/amount);
        m_trade.Buy(amount, NULL, 0, sl, tp);
    }
    else
    if(sign == -1){
        amount = roundVolume(m_ordersize/m_symbol.Ask());
        sl = m_symbol.NormalizePrice((double) ((amount*m_symbol.Ask())+m_run_stoploss)/amount);
        tp = m_symbol.NormalizePrice((double) ((amount*m_symbol.Ask())-m_run_targetprofit)/amount);
        m_trade.Sell(amount, NULL, 0, sl, tp);
    }
}
