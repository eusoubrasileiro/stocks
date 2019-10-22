#include "..\..\Util.mqh"
#include "..\..\Buffers.mqh"
#include "CBufferMqlTicks.mqh"
#include "..\bbands\XyVectors.mqh"
#include "..\bbands\BbandsPython.mqh"
#include "..\..\datastruct\Bars.mqh"

// number of samples needed/used for training 5*days?
const int                Expert_BufferSize      = 5000; // indicators buffer needed
const int                Expert_MaxFeatures     = 100; // max features allowed
const double             Expert_MoneyBar_Size   = 500e3; // R$ to form 1 money bar

// Using Dolar/Real Bars and consequently tick data
// only for tick time-frame
class CExpertRBarBands : public CExpertX
{
    CBufferMqlTicks m_ticks; // buffer of ticks w. control to download unique ones.
    MoneyBarBuffer m_bars; // buffer of money bars base for everything
    // created from ticks

    // bollinger bands configuration
    CTaBBANDS m_bands[]; // array of indicators for each bollinger band
    // upper and down and middle
    int m_nbands; // number of bands
    int m_bbwindow; // reference size of bollinger band indicator others
    double m_bbwindow_inc;   // are multiples of m_bbwindow_inc
    // store buy|sell|hold signals for each bband
    CBuffer<int> m_raw_signal[];
    MqlDateTime m_mqldt_now;
    MqlDateTime m_last_time; // last time after refresh
    CMqlDateTimeBuffer m_mqltime; // time buffer
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
      m_oindfeatures = new CIndicators;
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
    m_ticks = new CBufferMqlTicks(m_symbol.Name);
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
    // total of other indicators are m_nbands*2*(EMA+MACD+VOLUME)= m_nbands*2*3
    // m_nbands*6 so far + m_nbands  band signals
    m_nsignal_features = m_nbands*6 + m_nbands;
    //m_nsignal_features = m_nbands + m_nbands;
    m_xtrain_dim = m_nsignal_features*m_batch_size;

    // allocate array of bbands indicators
    ArrayResize(m_bands, m_nbands);

    // group of samples and signal vectors to train the model
    // resize buffer of training pairs
    m_xypairs = new CObjectBuffer<XyPair>;
    m_xypairs.Resize(m_ntraining);
    // resize x feature vector inside it
    // but they do not exist yet cant resize something that doesnt exist
    // only the space available for them are there
    //for(int i=0; i<m_ntraining; i++)
    //    m_xypairs[i].Resize(m_xtrain_dim);
    m_model_refresh = model_refresh;

    ArrayResize(m_raw_signal, m_nbands); // one for each band
    ArrayResize(m_last_raw_signal, m_nbands);
    //ArrayResize(m_raw_signal_time, m_nbands); // one for each band
    CreateBBands();
    CreateOtherFeatureIndicators();

  }

  bool Refresh(void)
  {

    // also updates m_bands
    if(!CExpert::Refresh())
        return (false);
    // features indicators
    m_oindfeatures.Refresh();
    // insert current time in the time buffer
    // must be here so all buffers are aligned
    TimeCurrent(m_last_time);
    m_mqltime.Add(m_last_time);
   // called after CExpert garantees not called twice
   // due TimeframesFlags(time)
   // also garantees indicators refreshed
    RefreshRawBandSignals();

    // update all bands indicators 
    for(int i=0; i<m_nbands; i++){
        m_nbands[i].Refresh(); // 1-Exponential type

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
      // refresh whatever possible
      if(Refresh()){ // sucess of updating everything w. new data
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
          Xforecast.time = m_last_time;
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

      for(int j=0; j<m_nbands; j++){
          m_raw_signal[j] = new CBuffer<int>;
          m_raw_signal[j].Resize(Expert_BufferSize);
      }

      double inc=m_bbwindow_inc;
      for(int i=0; i<m_nbands; i++){ // create multiple increasing bollinger bands
          m_nbands[i] = new CTaBBANDS(m_bbwindow*inc, 2.5, 1); // 1-Exponential type
          m_nbands[i].Resize(Expert_BufferSize);
          inc += m_bbwindow_inc;
      }
  }

  void RefreshRawBandSignals(void){
    // should be called only once per refresh
    for(int i=0; i<m_nbands; i++){
        //    Based on a bollinger band defined by upper-band and lower-band
        //    return signal:
        //        buy   1 : crossing down-outside it's buy
        //        sell -1 : crossing up-outside it's sell
        //        hold  0 : nothing usefull happend
        // remember order is reversed yonger first older later
        if( m_low.GetData(0) >= ((CiBands*) m_bands.At(i)).Upper(0))
            m_raw_signal[i].Add(-1);
        else
        if( m_high.GetData(0) <= ((CiBands*) m_bands.At(i)).Lower(0))
            m_raw_signal[i].Add(1); // buy
        else
            m_raw_signal[i].Add(0); // nothing
    }
  }


  // Global Buffer Size (all buffers MUST have same size)
  // that's why they all use same ResizeBuffer code
  // Real Buffer Size since Expert_BufferSize is just
  // an initilization parameter and Resize functions make
  // buffer a little bigger
  int BufferSize(){ return m_mqltime.BufferSize();}
  int BufferTotal(){ return m_raw_signal[0].Size(); }


};
