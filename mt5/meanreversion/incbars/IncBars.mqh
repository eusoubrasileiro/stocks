#include "CExpertIncBars.mqh"

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
}

void CExpertIncBars::Initialize(int nbands, int bbwindow,
     int batch_size, int ntraining,
     double ordersize, double stoploss, double targetprofit,
     double run_stoploss, double run_targetprofit, bool recursive,
     int max_positions=-1)
{
    // C++ init



    // Ticks download control
    ArrayResize(m_copied_ticks, Max_Tick_Copy);
    m_ncopied = 0;
    // first m_cmpbegin_time must be done here others
    // will come from C++
    // time now in ms will work 'coz next tick.ms value will be bigger
    // and will take its place
    m_cmpbegin_time = long (TimeCurrent())*1000; // turn in ms
    m_cmpbegin = 0;
}

// will be called every < 1 second
// OnTick + OnTimer garantee a better refresh rate
// than 1 second
// Since OnTick not enquees calls
// OnTick is not called again if the first OnTick
// has not being processed yet
void CExpertIncBars::CheckTicks(void)
{
  // copy all ticks from last copy time - 1 milisecond to now
  // to avoid missing ticks on same ms)
  m_ncopied = CopyTicksRange(m_symbol, m_copied_ticks,
       COPY_TICKS_ALL, m_cmpbegin_time, 0);

  if(m_ncopied < 1){
     int error = GetLastError();
     if(error == ERR_HISTORY_TIMEOUT)
       // ticks synchronization waiting time is up, the function has sent all it had.
       Print("ERR_HISTORY_TIMEOUT");
     else
     if(error == ERR_HISTORY_SMALL_BUFFER)
       //static buffer is too small. Only the amount the array can store has been sent.
       Print("ERR_HISTORY_SMALL_BUFFER");
     else
     if(error == ERR_NOT_ENOUGH_MEMORY)
       // insufficient memory for receiving a history from the specified range to the dynamic
       // tick array. Failed to allocate enough memory for the tick array.
       Print("ERR_NOT_ENOUGH_MEMORY");
     // better threatment here ??...better ignore and wait for next call
     return -1;
  }

  // call C++ passing ticks
  m_cmpbegin_time = OnTicks(m_copied_ticks, m_ncopied);

  // check to see if we should close any position
  if(SelectPosition()){ // if any open position
    CloseOpenPositionsbyTime();
    CheckTrailingStop();
  }
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
