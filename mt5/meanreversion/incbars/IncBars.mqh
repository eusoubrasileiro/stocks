#include "CExpertIncBars.mqh"


#import "incbars.dll"

void CppExpertInit(int nbands, int bbwindow, double devs, int batch_size, int ntraining,
    double start_hour, double end_hour, double expire_hour,
    double ordersize, double stoploss, double targetprofit, int incmax,
    double lotsmin, double ticksize, double tickvalue,
    double moneybar_size,  // R$ to form 1 money bar
    // ticks control
    bool isbacktest, uchar &symbol[], int symboln, long mt5_timenow,
    short debug_level);
    
long CppOnTicks(MqlTick &mt5_ticks[], int mt5_nticks);
    
bool CppRefresh(void);

#import

const double             Expert_MoneyBar_Size   = 500e3; // R$ to form 1 money bar
const int                Expert_Max_Tick_Copy   = 10e3; // max ticks copied at every CopyTicksRange call
const double             Expert_Stddevs   = 1.25; // R$ to form 1 money bar
// expert operations end (no further sells or buys) close all positions
// 10:15 to 16:30 , expires in 0:15 min
const double             Expert_Day_Start = 10.25; // Operational window maximum day hour
const double             Expert_Day_End = 16.5; // Operational window maximum day hour
const double             Expert_Expire = 15./60; // Time to expire a position (close it in hours)
const double             Expert_Increases = 3;


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
     int max_positions)
{
    m_max_positions = max_positions;
    // Ticks download control
    ArrayResize(m_copied_ticks, Expert_Max_Tick_Copy);
    m_ncopied = 0;
    // first m_cmpbegin_time must be done here others
    // will come from C++
    // time now is in seconds unix timestamp
    m_cmpbegin_time = TimeCurrent(); 
    
    setDayTradeParams(Expert_Expire, Expert_Day_End);
    
    uchar csymbol[100];
    string symbolname = m_symbol.Name();
    int nsymbol = StringToCharArray(symbolname, csymbol,  
                       0, WHOLE_ARRAY, CP_ACP); // ANSI
    nsymbol--; // null terminated string remove tha last character
   
    // C++ init
    CppExpertInit(nbands, bbwindow, Expert_Stddevs, batch_size, ntraining,
        Expert_Day_Start, Expert_Day_End, Expert_Expire,
        ordersize, stoploss, targetprofit, Expert_Increases,
        m_symbol.LotsMin(), m_symbol.TickSize(), m_symbol.TickValue(),
        Expert_MoneyBar_Size,  // R$ to form 1 money bar
        // ticks control
        MQL_TESTER, csymbol, nsymbol, m_cmpbegin_time,
        0); // debug level 0 or 1 (a lot of messages (every OnTick))

    m_cmpbegin_time*=1000; // to ms next CopyTicksRange call
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
  m_ncopied = CopyTicksRange(m_symbol.Name(), m_copied_ticks,
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
     return;
  }
  
//  datetime dtime=D'02.10.2019 16:54:00'; 
//  m_check_time = (datetime) (m_cmpbegin_time*0.001);
//  
//  if(m_check_time >= dtime)
//    Print("Here");
    
  // call C++ passing ticks
  m_cmpbegin_time = CppOnTicks(m_copied_ticks, m_ncopied);
  
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
        sl = m_symbol.NormalizePrice((double) ((amount*m_symbol.Ask())-m_stoploss)/amount);
        tp = m_symbol.NormalizePrice((double) ((amount*m_symbol.Ask())+m_targetprofit)/amount);
        m_trade.Buy(amount, NULL, 0, sl, tp);
        m_volume += amount;
    }
    else
    if(sign == -1){
        amount = roundVolume(m_ordersize/m_symbol.Ask());
        sl = m_symbol.NormalizePrice((double) ((amount*m_symbol.Ask())+m_stoploss)/amount);
        tp = m_symbol.NormalizePrice((double) ((amount*m_symbol.Ask())-m_targetprofit)/amount);
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
