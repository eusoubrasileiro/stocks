#include "..\Util.mqh"
#include "..\Buffers.mqh"
#include "..\XyVectors.mqh"

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
    //CBuffer<datetime> m_raw_signal_time[];
    // 1|-1|0 = buy|sell|hold
    CIndicators m_bands; // collection of bbands indicators
    CIndicators m_oindfeatures; // other 'feature' indicators used to identify patterns
    int m_batch_size;
    int m_ntraining;
    int m_nsignal_features;
    int m_xtrain_dim; // dimension of x train vector
    CObjectBuffer<XyPair> m_xypairs;
    //double m_buffXfeatures[][Expert_MaxFeatures];
    // profit or stop loss calculation
    double m_ordersize;
    double m_amount; // tick value * quantity bought in $$ per order
    double m_stoploss; // stop loss value in $$$ per order
    double m_targetprofit; //targetprofit in $$$ per order

  public:

   CExpertBands(void){
      m_bands = new CIndicators;
      m_oindfeatures = new CIndicators;
      m_bbwindow_inc = 0.5;
      // use all those series
      m_used_series = USE_SERIES_OPEN|USE_SERIES_CLOSE|USE_SERIES_HIGH|USE_SERIES_LOW|USE_SERIES_TIME;
   };

  void CExpertBands::Initialize(int nbands, int bbwindow,
        int batch_size, int ntraining,
        double ordersize, double stoploss, double targetprofit){
      // Call only after init indicators
    m_nbands = nbands;
    m_bbwindow = bbwindow;
    // order size is in $$
    // m_order_size is in contracts
    // need a conversion here TickValue for stocks
    m_ordersize = ordersize;
    m_amount = m_symbol.TickValue()*m_ordersize;
    m_stoploss = stoploss; // value in $$$
    m_targetprofit = targetprofit;

    m_batch_size = batch_size;

    m_ntraining = ntraining; // minimum number of X, y training pairs
    // total of other indicators are m_nbands*2*(EMA+MACD+VOLUME)= m_nbands*2*3
    // m_nbands*6 so far + 6 band signals
    m_nsignal_features = m_nbands*6 + m_nbands;
    m_xtrain_dim = m_nsignal_features*m_batch_size;

    // group of samples and signal vectors to train the model
    // resize buffer of training pairs
    m_xypairs = new CObjectBuffer<XyPair>;
    m_xypairs.Resize(m_ntraining);
    // resize x feature vector inside it
    for(int i=0; i<m_ntraining; i++)
        m_xypairs[i].Resize(m_xtrain_dim);

    ArrayResize(m_raw_signal, m_nbands); // one for each band
    //ArrayResize(m_raw_signal_time, m_nbands); // one for each band
    CreateBBands();
    CreateOtherFeatureIndicators();

  }

  bool CExpertBands::Refresh(void)
  {
    // also updates m_bands
    if(!CExpert::Refresh())
        return (false);

    // features indicators
    m_oindfeatures.Refresh();

   // called after CExpert garantees not called twice
   // due TimeframesFlags(time)
   // also garantees indicators refreshed
    RefreshRawBandSignals();

    // Has an entry signal in any band?
    if(hasEntrySignalBands())
    {

    }
    else
    {


    }

    CreateYTargetClasses();


//--- ok
   return(true);
  }

  ~CExpertBands(){};

  protected:

  void CreateYTargetClasses();
  void BandCreateYTargetClasses(CBuffer<int> &bandsg_raw, CObjectBuffer<XyPair> &xypairs);
  void CreateXFeatureVectors(CObjectBuffer<XyPair> &xypairs);
  bool CreateXFeatureVector(XyPair &xypair);
  void CreateOtherFeatureIndicators();

  bool hasEntrySignalBands(){
      // verify an entry signal in any band
      for(int i=0; i<m_nbands; i++)
          if(m_raw_signal[i].GetData(0) != 0)
              return true; // last index is (0) in GetData
      return false;
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
      m_time.BufferResize(Expert_BufferSize);

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
        // remember order is reversed yonger firs older later
        if( m_low.GetData(0) >= ((CiBands*) m_bands.At(i)).Upper(0))
            m_raw_signal[i].Add(-1);
        else
        if( m_high.GetData(0) <= ((CiBands*) m_bands.At(i)).Lower(0))
            m_raw_signal[i].Add(1); // buy
        else
            m_raw_signal[i].Add(0); // nothing
        //m_raw_signal_time[i].Add(m_time.GetData(0))
    }
  }

};
