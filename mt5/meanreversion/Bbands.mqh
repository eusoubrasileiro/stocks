#include "..\Util.mqh"
#include "..\Buffers.mqh"


// number of samples needed/used for training 5*days?
const int                Expert_BufferSize      = 60*7*5; // indicators buffer needed

class XyPair
{ // single element needed for training
public:
    int xdim; // dimension of X feature vector
    double X[]; // X feature vector
    int y; // class value
    datetime time; // when that happend datetime
    // used to assembly X feature vector
    int cbuffindex; // buffer index value of y class
    // bool ready; // if XyPair is ready for training

    XyPair(void): xdim(16), time(0) {
        ArrayResize(X, xdim);
    }

    XyPair(int vy, datetime vtime, int vcbuffindex){
        y = vy;
        time = vtime;
        cbuffindex = vcbuffindex;
        xdim=0;
    }

    ~XyPair(void){ if(xdim >0) ArrayFree(X); };

    void Resize(const int size){
        ArrayResize(X, size);
        xdim = size;
    }
};

class CExpertXBands : public CExpertX
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
    // profit or stop loss calculation
    double m_ordersize;
    double m_amount; // tick value * quantity bought in $$ per order
    double m_stoploss; // stop loss value in $$$ per order
    double m_targetprofit; //targetprofit in $$$ per order

  public:

   CExpertXBands(void){
      m_bands = new CIndicators;
      m_oindfeatures = new CIndicators;
      m_bbwindow_inc = 0.5;

      // use all those series
      m_used_series = USE_SERIES_OPEN|USE_SERIES_CLOSE|USE_SERIES_HIGH|USE_SERIES_LOW|USE_SERIES_TIME;
   };

  void CExpertXBands::Initialize(int nbands, int bbwindow,
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
    // m_nbands*6 so far
    m_nsignal_features = m_nbands*6;
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

  bool CExpertXBands::Refresh(void)
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

    CreateTargetClasses();


//--- ok
   return(true);
  }

  protected:

    bool hasEntrySignalBands(){
        // verify an entry signal in any band
        for(int i=0; i<m_nbands; i++)
            if(m_raw_signal[i].GetData(0) != 0)
                return true; // last index is (0) in GetData
        return false;
    }

    void CreateTargetClasses();
    void CreateBuyClasses(CBuffer<double> &bandsg_raw, CObjectBuffer<XyPair> &xypairs);

    void CreateOtherFeatureIndicators();

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
    // should be called only once
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

void CExpertXBands::CreateOtherFeatureIndicators(){
        MqlParam params[8];
        double inc = m_bbwindow_inc;

        // order is important to assembly afte feature arrays

        // so first all EMA indicators than
        int type_price[2]= {PRICE_HIGH, PRICE_LOW};
        for(int i=0; i<m_nbands; i++){
            for(int j=0; j<2; j++){ // for HIGH and LOW
                // two EMA for each band
                params[0].type = TYPE_INT; // enums are all 4 bytes according to mql5 web docs
                params[0].integer_value = m_bbwindow*inc; // ma_period
                params[1].type = TYPE_INT;
                params[1].integer_value = 0; // ma_shift
                params[2].type = TYPE_INT;
                params[2].integer_value = MODE_EMA; // ma_method
                params[3].type = TYPE_INT;
                params[3].integer_value = type_price[j]; // applied    --- could be a handle of another
                // indicator
                m_oindfeatures.Create(m_symbol.Name(), m_period, IND_MA, 4, params);
            }
        }
        // macd indicators
        for(int i=0; i<m_nbands; i++){
            for(int j=0; j<2; j++){ // for HIGH and LOW
                // two MACD for each band
                params[0].type = TYPE_INT;
                params[0].integer_value = m_bbwindow*inc*0.5; // fast_ema_period
                params[1].type = TYPE_INT;
                params[1].integer_value = m_bbwindow*inc*0.75; // slow_ema_period
                params[2].type = TYPE_INT;
                params[2].integer_value = m_bbwindow*inc*0.25; // signal_period
                params[3].type = TYPE_INT;
                params[3].integer_value = type_price[j]; // applied    --- could be a handle of another
                // indicator
                m_oindfeatures.Create(m_symbol.Name(), m_period, IND_MACD, 4, params);
            }
        }
        // volume indicators
        int type_volume[2]= {VOLUME_REAL, VOLUME_TICK};
        for(int i=0; i<m_nbands; i++){
            for(int j=0; j<2; j++){ // for VOLUME_REAL and VOLUME_TICK
                // two VOLUME indicators for each band
                params[0].type = TYPE_INT;
                params[0].integer_value = type_volume[j];
                // indicator
                m_oindfeatures.Create(m_symbol.Name(), m_period, IND_VOLUMES, 4, params);
            }
        }

        // total of other indicators are m_nbands*2*(EMA+MACD+VOLUME)= m_nbands*2*3
        // m_nbands*6 so far

        m_oindfeatures.BufferResize(Expert_BufferSize);
    }



    // # bandsg, yband, ask, bid, day, amount, targetprofit, stoploss
    // bars = obars.copy()
    // for j in range(nbands): # for each band traverse it
    //     ibandsg = bars.columns.get_loc('bandsg'+str(j))
    //     # being pessimistic ... right
    //     ybandsell = traverseSellBand(bars.iloc[:, ibandsg].values.astype(int),
    //                                     bars.H.values, bars.L.values, bars.date.values,
    //                                     amount, targetprofit, stoploss)
    //     ybandbuy = traverseBuyBand(bars.iloc[:, ibandsg].values.astype(int),
    //                                     bars.H.values, bars.L.values, bars.date.values,
    //                                     amount, targetprofit, stoploss)
    //     bars['y'+str(j)] = mergebandsignals(ybandsell, ybandbuy)
    //
    // return bars

// Create target class by analysing raw bollinger band signals
// those that went true receive 1 buy or -1 sell
// those that wen bad receive 0 hold
// A target class exist for each band.
// An essemble of bands together and summed are a better classifier.
// A day integer identifier column name 'date' must exist prior call.
// That is used to hold positions from passing to another day.
void CExpertXBands::CreateTargetClasses(){
    // ReclassifyBuyBandSignals();
    // ReclassifySellBandSignals();
}

void CExpertXBands::CreateBuyClasses(CBuffer<double> &bandsg_raw, CObjectBuffer<XyPair> &xypairs)
{
      // buy at ask = High at minute time-frame (being pessimistic)
      // sell at bid = Low at minute time-frame
      // classes are [0, 1] = [hold, buy]
      // stoploss - to not loose much money (positive)
      // targetprofit - profit to close open position
      // amount - tick value * quantity bought in $$
      double entryprice = 0;
      int history = 0; // nothing, buy = 1, sell = -1
      datetime previous_day = 0;
      datetime time, day = 0;
      double profit = 0;
      // net mode ONLY
      // net mode ONLY
      for(int i=bandsg_raw.Size()-1; i>=0; i--){ // from past to present
          time = m_time.GetData(i);
          day = GetDayZeroHour(time); // unique day identifier
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
          if(bandsg_raw[i] == 1){
              if(history == 0){
                  entryprice = m_high.GetData(i);
                  xypairs.Add(new XyPair(1, time, i)); //  save this buy
              }
              else{ // another buy in sequence
                  // or after a sell in the same minute
                  // the previous batch of signals will be saved with this class (hold)
                  xypairs.Last().y = 0; // reclassify the previous buy/sell as hold
                  // new buy signal
                  entryprice = m_high.GetData(i);
                  xypairs.Add(new XyPair(1, time, i)); //  save this buy
              }
              history=1;
          }
          else
          if(bandsg_raw[i] == -1){
              if(history == 0){
                  entryprice = m_low.GetData(i);
                  xypairs.Add(new XyPair(-1, time, i)); //  save this sell
              }
              else{ // another sell in sequence
                   // or after a buy  in the same minute
                  // the previous batch of signals will be saved with this class (hold)
                  xypairs.Last().y = 0; // reclassify the previous buy/sell as hold
                  // new buy sell
                  entryprice = m_low.GetData(i);
                  xypairs.Add(new XyPair(-1, time, i)); //  save this buy
              }
              history=-1;
          }
          else // signal 0 history 1
          if(history == 1){ // equivalent to + && bandsg_raw[i] == 0
              profit = (m_low.GetData(i)-entryprice)*m_amount;// # current profit
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
              profit = (entryprice-m_high.GetData(i))*m_amount;// # current profit
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
      }
      //# reached the end of data buffer did not close one buy previouly open
      if(history == 1 || history == -1) // don't know about the future cannot train with this guy
          xypairs.RemoveLast();
    // will only have 0 (false positive) or 1's
}


    // @jit(nopython=True) # 1000x faster than using pandas for loop
    // def traverseBuyBand(bandsg, high, low, day, amount, targetprofit, stoploss):
    //     """
    //     buy at ask = High at minute time-frame (being pessimistic)
    //     sell at bid = Low at minute time-frame
    //     classes are [0, 1] = [hold, buy]
    //     stoploss - to not loose much money (positive)
    //     targetprofit - profit to close open position
    //     amount - tick value * quantity bought in $$
    //     """
    //     buyprice = 0
    //     history = 0 # nothing, buy = 1, sell = -1
    //     buyindex = 0
    //     previous_day = 0
    //     ybandsg = np.empty(bandsg.size)
    //     ybandsg.fill(np.nan)
    //     for i in range(bandsg.size):
    //         if day[i] != previous_day: # a new day reset everything
    //             if history == 1:
    //                 # the previous batch o/f signals will be saved with Nan don't want to train with that
    //                 ybandsg[buyindex] = np.nan
    //             buyprice = 0
    //             history = 0 # nothing, buy = 1, sell = -1
    //             buyindex = 0
    //             previous_day = day[i]
    //         if int(bandsg[i]) == 1:
    //             if history == 0:
    //                 buyprice = high[i]
    //                 buyindex = i
    //                 ybandsg[i] = 1 #  save this buy
    //             else: # another buy in sequence -> cancel the first (
    //                 # the previous batch of signals will be saved with this class (hold)
    //                 ybandsg[buyindex] = 0 # reclassify the previous buy as hold
    //                 # new buy signal
    //                 buyprice = high[i]
    //                 buyindex = i
    //                 #print('y: ', 0)
    //             history=1
    //             # net mode
    //         elif int(bandsg[i]) == -1: # a sell, cancel the first buy
    //             ybandsg[buyindex] = 0 # reclassify the previous buy as hold
    //             #print('y: ', 0)
    //             history=0
    //         elif history == 1:
    //             profit = (low[i]-buyprice)*amount # current profit
    //             #print('profit: ', profit)
    //             if profit >= targetprofit:
    //                 # ybandsg[buyindex] = 1 # a real (buy) index class nothing to do
    //                 history = 0
    //                 #print('y: ', 1)
    //             elif profit <= (-stoploss): # reclassify the previous buy as hold
    //                 ybandsg[buyindex] = 0  # not a good deal, was better not to entry
    //                 history = 0
    //                 #print('y: ', 0)
    //     # reached the end of data but did not close one buy previouly open
    //     if history == 1: # don't know about the future cannot train with this guy
    //         ybandsg[buyindex] = np.nan # set it to be ignored
    //     return ybandsg # will only have 0 (false positive) or 1's
    //
    // @jit(nopython=True) # 1000x faster than using pandas for loop
    // def traverseSellBand(bandsg, high, low, day, amount, targetprofit, stoploss):
    //     """
    //     same as traverseSellBand but for sell positions
    //     buy at high = High at minute time-frame (being pessimistic)
    //     sell at low = Low at minute time-frame
    //     classes are [0, -1] = [hold, sell]
    //     stoploss - to not loose much money (positive)
    //     targetprofit - profit to close open position
    //     amount - tick value * quantity bought in $$
    //     """
    //     sellprice = 0
    //     history = 0 # nothing, buy = 1, sell = -1
    //     sellindex = 0
    //     previous_day = 0
    //     ybandsg = np.empty(bandsg.size)
    //     ybandsg.fill(np.nan)
    //     for i in range(bandsg.size):
    //         if day[i] != previous_day: # a new day reset everything
    //             if history == -1: # was selling when day ended
    //                 # the previous batch o/f signals will be saved with Nan don't want to train with that
    //                 ybandsg[sellindex] = np.nan
    //             sellprice = 0
    //             history = 0 # nothing, buy = 1, sell = 2
    //             sellindex = 0
    //             previous_day = day[i]
    //         if int(bandsg[i]) == -1:
    //             if history == 0:
    //                 sellprice = low[i]
    //                 sellindex = i
    //                 ybandsg[i] = -1
    //             else: # another sell in sequence -> cancel the first
    //                 # the previous batch of signals will be saved with this class (hold)
    //                 ybandsg[sellindex] = 0 # reclassify the previous sell as hold
    //                 # new buy signal
    //                 sellprice = low[i]
    //                 sellindex = i
    //             history = -1
    //         elif int(bandsg[i]) == 1: # a buy
    //             ybandsg[sellindex] = 0 # reclassify the previous sell as hold
    //             history = 0
    //         elif history == -1:
    //             profit = (sellprice-high[i])*amount # current profit
    //             if profit >= targetprofit:
    //                 # ybandsg[sellindex] = -1 # a real (sell) index class nothing to do
    //                 history = 0
    //             elif profit <= (-stoploss): # reclassify the previous buy as hold
    //                 ybandsg[sellindex] = 0  # not a good deal, was better not to entry
    //                 history = 0
    //     # reached the end of data but did not close one buy previouly open
    //     if history == -1: # don't know about the future cannot train with this guy
    //         ybandsg[sellindex] = np.nan # set it to be ignored
    //     return ybandsg
