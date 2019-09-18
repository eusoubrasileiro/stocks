#include <Arrays\ArrayDouble.mqh>
#include <Trade\Trade.mqh>
#include <Indicators\TimeSeries.mqh>
#include <Indicators\Indicators.mqh>

const int                Expert_BufferSize      = 60*7*5; // indicators buffer needed


class CiRawSignalBands : public CIndicator
  {
protected:
    // band definition
   int               m_ma_period;
   double            m_deviation;
   int               m_applied;
   CiOpen           *m_open;           // pointer to the object for access to open prices of bars
   CiHigh           *m_high;           // pointer to the object for access to high prices of bars
   CiLow            *m_low;            // pointer to the object for access to low prices of bars
   CiClose          *m_close;          // pointer to the object for access to close prices of bars
   CiBands          *m_bands;          // pointer to the object for access to bollinger bands

public:
                     CiRawSignalBands(void);
                    ~CiRawSignalBands(void);
   //--- method of creation
   bool              Create(const string symbol,const ENUM_TIMEFRAMES period, const int ma_period, 
                            const int buffer_size, const double deviation, const int applied);
   //--- methods of access to indicator data
   double            (const int index) const;
   //--- method of identifying
   virtual int       Type(void) const { return(IND_CUSTOM); }
   // --- the signal at any index position
   double            Signal(const int index) const;

protected:
   //--- methods of tuning
   bool              Initialize(const string symbol,const ENUM_TIMEFRAMES period,
                                const int ma_period, const int buffer_size
                                const double deviation, const int applied);
};

CiRawSignalBands::CiRawSignalBands(void) : m_ma_period(PERIOD_M1),
                         m_deviation(2.5),
                         m_applied(PRICE_TYPICAL)
  {
  }
//CiBands::~CiBands(void){}


bool CiRawSignalBands::Create(const string symbol,const ENUM_TIMEFRAMES period,
                     const int ma_period,const int buffer_size,
                     const double deviation,const int applied)
  {
//--- check history
   if(!SetSymbolPeriod(symbol,period))
      return(false);
//--- create
    m_bands = new CiBands;
    m_open = new CiOpen;
    m_close = new CiClose;
    m_high = new CiHigh;
    m_low = new CiLow;    
    
    if(!m_bands.Create(symbol, period, ma_period, 0, deviation, applied))
        return (false);    
        
    m_open.Create(symbol, period);
    m_close.Create(symbol, period);
    m_high.Create(symbol, period);
    m_low.Create(symbol, period);

//--- indicator successfully created
   if(!Initialize(symbol,period,ma_period,ma_shift,deviation,applied))
     {
      return(false);
     }
//--- ok
   return(true);
  }

//+------------------------------------------------------------------+
//| Initialize indicator with the special parameters                 |
//+------------------------------------------------------------------+
bool CiRawSignalBands::Initialize(const string symbol,const ENUM_TIMEFRAMES period,
                                const int ma_period, const int buffer_size
                                const double deviation, const int applied)
  {
   // one array of buffers only for integer signal for this band
   if(CreateBuffers(symbol, period, 1))
     {
      //--- string of status of drawing
      m_name  = "BandsSignal";
      m_status="("+symbol+","+PeriodDescription()+","+
               IntegerToString(ma_period)+","+
               DoubleToString(deviation)+","+PriceDescription(applied);
      //--- save settings
      m_ma_period=ma_period;
      m_deviation=deviation;
      m_applied  =applied;      

      //--- created buffer 1 buffer name it
      ((CIndicatorBuffer*)At(0)).Name("RAW_SIGNAL_BAND"); 
      ((CIndicatorBuffer*)At(0)).Offset(0);
      
      return(true);
     }
     
     if(!BufferResize(buffer_size))
        return (false);
//--- error
   return(false);
  }
  
bool CiRawSignalBands::BufferResize(const int size)
  {
   if(size>m_buffer_size && !CIndicator::BufferResize(size))
      return(false);      

    // only one CIndicatorBuffer/CDoubleBuffer internal for CiBands
    if(!m_bands.BufferResize(size))
        return (false);   
    if(!m_open.BufferResize(size))
        return (false);         
    if(!m_close.BufferResize(size))
        return (false);         
    if(!m_high.BufferResize(size))
        return (false);         
    if(!m_low.BufferResize(size))
        return (false);         
        
//--- ok
   return(true);
  }  
  
void CiRawSignalBands::Refresh(const int flags)
  {
   int               i;
   CIndicatorBuffer *buff;
//--- refreshing buffer only one internal
   buff=At(i);
   
   if(m_redrawer)
          buff.Refresh(m_handle,i);            
   if(!(flags&m_timeframe_flags)){
      if(m_refresh_current)
          buff.RefreshCurrent(m_handle,i);
   }
   else
     buff.Refresh(m_handle,i);
     
     
    m_open.Refresh(); // they do nothing but to return true
    m_close.Refresh();
    m_high.Refresh();
    m_low.Refresh();     
  }  
//+------------------------------------------------------------------+
//| Access to Base buffer of "Bollinger Bands"                       |
//+------------------------------------------------------------------+
double CiRawSignalBands::Signal(const int index) const
{
   CIndicatorBuffer *buffer=At(0); // first buffer can be At(1); or At(2); or forth
//--- check
   if(buffer==NULL)
      return(EMPTY_VALUE);
//---
   return(buffer.At(index));
}
