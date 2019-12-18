#include "CExpertIncBars.mqh"
//#define USE_DEBUG

#ifdef _DEBUG
#ifdef USE_DEBUG
#define FILE_DEBUG
#endif
#endif

CExpertIncBars::CExpertIncBars(void){
   m_last_positions = 0; // number of 'positions' openend
   m_last_volume = 0;
   m_volume = 0;
}

CExpertIncBars::~CExpertIncBars(void){
   Deinit();
}

void CExpertIncBars::Deinit(void){
  CExpertMain::Deinit();
  #ifdef FILE_DEBUG
  FileClose(file_io_hnd);
  #endif
}

void CExpertIncBars::Initialize(int nbands, int bbwindow,
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
void CExpertIncBars::CheckTicks(void)
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

void CExpertIncBars::verifyEntry(){

}


void CExpertIncBars::BuySell(int sign){
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
bool CExpertIncBars::TradeEventPositionVolumeChanged(void){
    if(m_volume > m_last_volume) // one more 'position'
        m_last_positions++;
    else
    if(m_volume < m_last_volume) // one less 'position'
        m_last_positions--;

    m_last_volume = m_volume;
    return true;
}

bool CExpertIncBars::TradeEventPositionOpened(void){
    m_last_positions=1;
    m_last_volume = m_volume;
    return true;
}

bool CExpertIncBars::TradeEventPositionClosed(void){
    m_last_positions=0;
    m_volume=0;
    m_last_volume=0;
    return true;
}
