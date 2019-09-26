#include "..\Util.mqh"
#include "..\Buffers.mqh"
#include "..\XyVectors.mqh"
#include "BbandsPython.mqh"

// number of samples needed/used for training 5*days?
const int                Expert_BufferSize      = 60*7*5; // indicators buffer needed
const int                Expert_MaxFeatures     = 1000; // max features allowed

class CExpertBands : public CExpertX
{
    int m_nbands; // number of bands
    int m_bbwindow; // reference size of bollinger band indicator others
    double m_bbwindow_inc;   // are multiples of m_bbwindow_inc
    // store buy|sell|hold signals for each bband
    CBuffer<int> m_raw_signal[];
    MqlDateTime m_mqldt_now;
    MqlDateTime m_last_time; // last time after refresh
    CMqlDateTimeBuffer m_mqltime; // time buffer
    int m_last_raw_signal[]; // last raw signal in all m_nbands
    //CBuffer<datetime> m_raw_signal_time[];
    // 1|-1|0 = buy|sell|hold
    CIndicators m_bands; // collection of bbands indicators
    CIndicators m_oindfeatures; // other 'feature' indicators used to identify patterns
    int m_batch_size;
    int m_ntraining;
    int m_nsignal_features;
    int m_xtrain_dim; // dimension of x train vector
    CObjectBuffer<XyPair> m_xypairs;
    //CBuffer<double*> m_X;
    //double m_buffXfeatures[][Expert_MaxFeatures];
    // profit or stop loss calculation for training
    double m_ordersize;
    double m_stoploss; // stop loss value in $$$ per order
    double m_targetprofit; //targetprofit in $$$ per order
    // profit or stop loss calculation for execution
    double m_run_stoploss; // stop loss value in $$$ per order
    double m_run_targetprofit; //targetprofit in $$$ per order
    // python sklearn model
    bool m_recursive;
    sklearnModel m_model;
    unsigned int m_model_refresh; // how frequent to update the model
    // (in number of new training samples)
    // helpers to count training samples
    unsigned long m_xypair_count; // counter to help train/re-train model
    unsigned long m_model_last_training; // referenced on m_xypair_count's

  public:

   CExpertBands(void){
      m_bands = new CIndicators;
      m_oindfeatures = new CIndicators;
      m_bbwindow_inc = 0.5;
      // use all those series
      m_used_series = USE_SERIES_OPEN|USE_SERIES_CLOSE|USE_SERIES_HIGH|USE_SERIES_LOW;
      m_model = new sklearnModel;
      m_xypair_count = 0;
      m_model_last_training =0;
   };

  void CExpertBands::Initialize(int nbands, int bbwindow,
        int batch_size, int ntraining,
        double ordersize, double stoploss, double targetprofit,
        double run_stoploss, double run_targetprofit, bool recursive,
        int model_refresh=15){
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

  bool CExpertBands::Refresh(void)
  {
    TimeCurrent(m_mqldt_now);
    if(IsEqualMqldt_M1(m_last_time, m_mqldt_now))
        return false;
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
    return(true);
  }

  void CExpertBands::OnTimer(){
    //--- updated quotes and indicators
    if(!Refresh() || !isInsideDay())
        return; // no work without correct data

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

    // in case did not reach the take profit close it by the day end
    // check to see if we should close any position
    if(SelectPosition()){ // if any open position
        CloseOpenPositionsbyTime();
        CheckTrailingStop();
    }

  }

  ~CExpertBands(){}; // dont care for now memory leaks on expert execution

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

      // call only after refresh / resize all internal series buffers to BufferSize
      m_open.BufferResize(Expert_BufferSize);
      m_close.BufferResize(Expert_BufferSize);
      m_high.BufferResize(Expert_BufferSize);
      m_low.BufferResize(Expert_BufferSize);
      m_mqltime.Resize(Expert_BufferSize); // time is allawys sorted

      double inc=m_bbwindow_inc;
      for(int i=0; i<m_nbands; i++){ // create multiple increasing bollinger bands
          CiBands *band = new CiBands;
          band.Create(m_symbol.Name(), PERIOD_M1, m_bbwindow*inc, 0, 2.5, PRICE_TYPICAL);
          band.BufferResize(Expert_BufferSize);
          m_indicators.Add(band); // needed to be refreshed by CExpert
          m_bands.Add(band);
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
