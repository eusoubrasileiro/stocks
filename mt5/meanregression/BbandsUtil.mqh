#include <Indicators\Indicator.mqh>

class RawSignalBand : public CIndicator, iOpen 
  {
protected:
   int               m_ma_period;
   double            m_deviation;
   int               m_applied;

public:
                     CiBands(void);
                    ~CiBands(void);
   //--- method of creation
   bool              Create(const string symbol,const ENUM_TIMEFRAMES period,
                            const int ma_period,const int ma_shift,
                            const double deviation,const int applied);
   //--- methods of access to indicator data
   int            Main(const int index) const;
   //--- method of identifying
   virtual int       Type(void) const { return(IND_BANDS); }

protected:
   //--- methods of tuning
   bool              Initialize(const string symbol,const ENUM_TIMEFRAMES period,
                                const int ma_period,const int ma_shift,
                                const double deviation,const int applied);
  };
//+------------------------------------------------------------------+
//| Constructor                                                      |
//+------------------------------------------------------------------+
CiBands::CiBands(void) : m_ma_period(-1),
                         m_deviation(EMPTY_VALUE),
                         m_applied(-1)
  {
  }
//+------------------------------------------------------------------+
//| Destructor                                                       |
//+------------------------------------------------------------------+
CiBands::~CiBands(void)
  {
  }
//+------------------------------------------------------------------+
//| Create indicator "Bollinger Bands"                               |
//+------------------------------------------------------------------+
bool CiBands::Create(const string symbol,const ENUM_TIMEFRAMES period,
                     const int ma_period,
                     const double deviation,const int applied)
  {
//--- check history
   if(!SetSymbolPeriod(symbol,period))
      return(false);
//--- create
   m_handle=iBands(symbol,period,ma_period,0,deviation,applied);
//--- check result
   if(m_handle==INVALID_HANDLE)
      return(false);
//--- indicator successfully created
   if(!Initialize(symbol,period,ma_period,ma_shift,deviation,applied))
     {
      //--- initialization failed
      IndicatorRelease(m_handle);
      m_handle=INVALID_HANDLE;
      return(false);
     }
//--- ok
   return(true);
  }
//+------------------------------------------------------------------+
//| Initialize indicator with the special parameters                 |
//+------------------------------------------------------------------+
bool CiBands::Initialize(const string symbol,const ENUM_TIMEFRAMES period,
                         const int ma_period,
                         const double deviation,const int applied)
  {
   if(CreateBuffers(symbol,period,3))
     {
      //--- string of status of drawing
      m_name  ="RawSignalBands";
      m_status="("+symbol+","+PeriodDescription()+","+
               IntegerToString(ma_period)+","+IntegerToString(0)+","+
               DoubleToString(deviation)+","+PriceDescription(applied)+") H="+IntegerToString(m_handle);
      //--- save settings
      m_ma_period=ma_period;
      m_deviation=deviation;
      m_applied  =applied;
      //--- create buffers
      ((CIndicatorBuffer*)At(0)).Name("BASE_LINE");
      ((CIndicatorBuffer*)At(0)).Offset(0);
      ((CIndicatorBuffer*)At(1)).Name("UPPER_BAND");
      ((CIndicatorBuffer*)At(1)).Offset(0);
      ((CIndicatorBuffer*)At(2)).Name("LOWER_BAND");
      ((CIndicatorBuffer*)At(2)).Offset(0);
      //--- ok
      return(true);
     }
//--- error
   return(false);
  }

//+------------------------------------------------------------------+
//| Access to Upper buffer of "Bollinger Bands"                      |
//+------------------------------------------------------------------+
double CiBands::Main(const int index) const
  {
   CIndicatorBuffer *upbuffer=At(1);
   CIndicatorBuffer *lwbuffer=At(2);
//--- check
   if(upbuffer==NULL||lwbuffer==NULL)
      return(EMPTY_VALUE);
      
   if()
   return(buffer.At(index));
  }
//+------------------------------------------------------------------+
//| Access to Lower buffer of "Bollinger Bands"                      |
//+------------------------------------------------------------------+
double CiBands::Lower(const int index) const
  {
   
//--- check
   if(buffer==NULL)
      return(EMPTY_VALUE);
//---
   return(buffer.At(index));
  }
