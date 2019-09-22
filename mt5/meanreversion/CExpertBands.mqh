#include "CExpertBandsh.mqh"

void CExpertBands::CreateOtherFeatureIndicators(){
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
                m_oindfeatures.Create(m_symbol.Name(), m_period, IND_VOLUMES, 1, params);
            }
        }

        // total of other indicators are m_nbands*2*(EMA+MACD+VOLUME)= m_nbands*2*3
        // m_nbands*6 so far

        m_oindfeatures.BufferResize(Expert_BufferSize);
}


// Create target class by analysing raw bollinger band signals
// those that went true receive 1 buy or -1 sell
// those that wen bad receive 0 hold
// A target class exist for each band.
// An essemble of bands together and summed are a better classifier.
// A day integer identifier must exist prior call.
// That is used to blocking positions from passing to another day.
void CExpertBands::CreateYTargetClasses(){
    for(int i=0; i<m_nbands; i++){
      BandCreateYTargetClasses(m_raw_signal[i], m_xypairs);
    }
}

void CExpertBands::BandCreateYTargetClasses(CBuffer<int> &bandsg_raw, CObjectBuffer<XyPair> &xypairs)
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
      double quantity = 0; // number of contracts or stocks shares 
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
          // useless : signal 0 - history = 0 - total total 3x3 = 9
          if(bandsg_raw.GetData(i) == 1){
              if(history == 0){
                  entryprice = m_high.GetData(i);
                  xypairs.Add(new XyPair(1, time, i)); //  save this buy
              }
              else{ // another buy in sequence
                  // or after a sell in the same minute
                  // the previous batch of signals will be saved with this class (hold)
                  xypairs.Last().y = 0; // reclassify the previous buy/sell as hold
                  // new buy signal
                  // maybe we can have profit here
                  // in a fast movement
                  entryprice = m_high.GetData(i);
                  xypairs.Add(new XyPair(1, time, i)); //  save this buy
              }
              quantity = roundVolume(m_ordersize/entryprice);
              history=1;
          }
          else
          if(bandsg_raw.GetData(i) == -1){
              if(history == 0){
                  entryprice = m_low.GetData(i);
                  xypairs.Add(new XyPair(-1, time, i)); //  save this sell
              }
              else{ // another sell in sequence
                   // or after a buy  in the same minute
                  // the previous batch of signals will be saved with this class (hold)
                  xypairs.Last().y = 0; // reclassify the previous buy/sell as hold
                  // new buy sell
                  // maybe we can have profit here
                  // in a fast movement
                  entryprice = m_low.GetData(i);
                  xypairs.Add(new XyPair(-1, time, i)); //  save this buy
              }
              quantity = roundVolume(m_ordersize/entryprice);
              history=-1;
          }
          else // signal 0 history 1
          if(history == 1){ // equivalent to + && bandsg_raw[i] == 0
              profit = (m_low.GetData(i)-entryprice)*quantity;// # current profit
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
              profit = (entryprice-m_high.GetData(i))*quantity;// # current profit
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


void CExpertBands::CreateXFeatureVectors(CObjectBuffer<XyPair> &xypairs)
{ // using the pre-filled buffer of y target classes
  // assembly the X array of features for it
  // from more recent to oldest
  // if it is already filled (ready) stop
  for(int i=0; i<xypairs.Size(); i++){
    // no need to assembly any other if this is already ready
    // olders will also be ready
    if(!CreateXFeatureVector(xypairs.GetData(i)))
        break;
  }

}

bool CExpertBands::CreateXFeatureVector(XyPair &xypair)
{
  if(xypair.isready) // no need to assembly
    return false; // did not suceed
  
  // find xypair.time current position on all buffers (have same size)
  // problem with CTimeBuffer not allowing to use the Search method of CLong Class
  // will have to create my own buffer for time
  int bufidx = ((CTimeBuffer *) m_time.At(0)).Search(xypair.time);
  
  // not found -1 :
  // cannot assembly X feature train with such an old signal not in buffer anymore
  // such should be removed ...
  if(bufidx == -1) 
    return false;
  
  // not enough : bands raw signal, features in time 
  // cannot create a X feature vector
  // check in the first band because at least this must exist
  if(bufidx < m_batch_size) 
    return false;
  
  if(ArraySize(xypair.X) == 0)
    xypair.Resize(m_xtrain_dim);    
  

  int xifeature = 0;
  // something like
  // double[Expert_BufferSize][nsignal_features] would be awesome
  // easier to copy to X also to create cross-features
  // using a constant on the second dimension is possible
  
  for(int timeidx=0; timeidx<m_batch_size; timeidx++){ // from present to past
    // features from band signals
    for(int i=0; i<m_nbands; i++, xifeature++){
      xypair.X[xifeature] = m_raw_signal[i].GetData(timeidx);
    }
    // features from other indicators
    int indfeatcount=0;
    for(; indfeatcount<m_nbands*2; indfeatcount++, xifeature++) // EMA's
      xypair.X[xifeature] = ((CiMA*) m_oindfeatures.At(indfeatcount)).Main(timeidx);
    for(; indfeatcount<m_nbands*4; indfeatcount++, xifeature++) // MACD's
        xypair.X[xifeature] = ((CiMACD*) m_oindfeatures.At(indfeatcount)).Main(timeidx);
    for(; indfeatcount<m_nbands*6; indfeatcount++, xifeature++) // Volume's
        xypair.X[xifeature] = ((CiVolumes*) m_oindfeatures.At(indfeatcount)).Main(timeidx);
  }
  xypair.isready = true;
  return true; // suceeded
}
