#include "CExpertRBarBBands.mqh"
//#define USE_DEBUG

#ifdef _DEBUG
#ifdef USE_DEBUG
#define FILE_DEBUG
#endif
#endif

CExpertRBarBands::CExpertRBarBands(void){
   m_bbwindow_inc = 0.5;
   m_model = new sklearnModel;
   m_xypair_count = 0;
   m_model_last_training =0;
   m_last_raw_signal_index = -1;
   m_last_positions = 0; // number of 'positions' openend
   m_last_volume = 0;
   m_volume = 0;
}

CExpertRBarBands::~CExpertRBarBands(void){
   Deinit();
}

void CExpertRBarBands::Deinit(void){
  CExpertMain::Deinit();
  #ifdef FILE_DEBUG
  FileClose(file_io_hnd);
  #endif
}

void CExpertRBarBands::Initialize(int nbands, int bbwindow,
     int batch_size, int ntraining,
     double ordersize, double stoploss, double targetprofit,
     double run_stoploss, double run_targetprofit, bool recursive,
     int max_positions=-1)
{

}
// will be called every < 1 second
// OnTick + OnTimer garantee a better refresh rate
// than 1 second
// Since OnTick not enquees calls
// OnTick is not called again if the first OnTick
// has not being processed yet
void CExpertRBarBands::CheckTicks(void)
{
  if(m_ticks.Refresh() > 0){
  #ifdef FILE_DEBUG
    //FileSeek(file_io_hnd, 0, SEEK_SET);
    FileWriteArray(file_io_hnd, m_ticks.m_data,
        m_ticks.beginNewTicks(), m_ticks.nNew());
    FileFlush(file_io_hnd);
  #endif
      if(m_bars.AddTicks(m_ticks) > 0)
      {
        // some new ticks arrived and new bars created
        // at least one new money bar created
        // refresh whatever possible with new bars
        if(Refresh()){
          // sucess of updating everything w. new data
            verifyEntry();
        }
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

void CExpertRBarBands::verifyEntry(){
  //--- updated quotes and indicators
  if(!isInsideDay())
      return; // no work outside day scope

  // has an entry signal in any band? on the lastest
  // raw signals added bars
  if(lastRawSignals() != -1)
  {
      int m_xypair_count_old = m_xypairs.Count();
      // only when a new entry signal recalc classes
      // only when needed
      CreateYTargetClasses();
      CreateXFeatureVectors(m_xypairs);
      // add number of xypairs added (cannot decrease)
      m_xypair_count += (m_xypairs.Count()-m_xypair_count_old);
      // update/train model if needed/possible
      if(m_xypairs.Count() >= m_ntraining){
        // first time training
        if(!m_model.isready){
         // time to train the model for the first time
          m_model.isready = PythonTrainModel();
        }
        //else // time to update the model?
        //if((m_xypair_count - m_model_last_training) >= m_model_refresh){
        // if diference in training samples to the last training is bigger than
        // refresh criteria - time to train again
        //  m_model.isready = PythonTrainModel();
        //}
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
        if(m_recursive && (y_pred == 0 || y_pred == 1 || y_pred == -1)){
          // modify the band raw signals by the ones predicted
          for(int i=0; i<m_nbands; i++)
            // change from -1 to 1 or to 0
            m_raw_signal[i].m_data[m_raw_signal[i].Count()-1] = y_pred;
        }
      }
   }
}


void CExpertRBarBands::BuySell(int sign){
    double amount, sl, tp;
    m_symbol.Refresh();
    m_symbol.RefreshRates();
    if(sign == 1){
        amount = roundVolume(m_ordersize/m_symbol.Ask());
        sl = m_symbol.NormalizePrice((double) ((amount*m_symbol.Ask())-m_run_stoploss)/amount);
        tp = m_symbol.NormalizePrice((double) ((amount*m_symbol.Ask())+m_run_targetprofit)/amount);
        m_trade.Buy(amount, NULL, 0, sl, tp);
        m_volume += amount;
    }
    else
    if(sign == -1){
        amount = roundVolume(m_ordersize/m_symbol.Ask());
        sl = m_symbol.NormalizePrice((double) ((amount*m_symbol.Ask())+m_run_stoploss)/amount);
        tp = m_symbol.NormalizePrice((double) ((amount*m_symbol.Ask())-m_run_targetprofit)/amount);
        m_trade.Sell(amount, NULL, 0, sl, tp);
        m_volume -= amount;
    }
}

////////
// controlling maximum number of 'positions'
//////
bool CExpertRBarBands::TradeEventPositionVolumeChanged(void){
    if(m_volume > m_last_volume) // one more 'position'
        m_last_positions++;
    else
    if(m_volume < m_last_volume) // one less 'position'
        m_last_positions--;

    m_last_volume = m_volume;
    return true;
}

bool CExpertRBarBands::TradeEventPositionOpened(void){
    m_last_positions=1;
    m_last_volume = m_volume;
    return true;
}

bool CExpertRBarBands::TradeEventPositionClosed(void){
    m_last_positions=0;
    m_volume=0;
    m_last_volume=0;
    return true;
}
