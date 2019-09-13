#include <Expert\ExpertTrailing.mqh>
#include <Trade\PositionInfo.mqh>


class CTrailingMA : public CExpertTrailing
  {
protected:
   //--- input parameters
   int               m_ma_period;
   ENUM_MA_METHOD    m_ma_method;
   ENUM_APPLIED_PRICE m_ma_applied;
   double m_MA_last_used; // last MA indicator value used to change SL
   long last_pos_id; // last position trailled (PositionGetInteger(POSITION_IDENTIFIER))

public:
    CiMA             *m_MA; // MA indicator
    CTrailingMA(void);
    ~CTrailingMA(void){};
    CTrailingMA(ENUM_MA_METHOD ma_method,
          ENUM_APPLIED_PRICE ma_applied, int m_ma_period);

   virtual bool      InitIndicators(CIndicators *indicators);
   virtual bool      ValidationSettings(void);
   //---
   virtual bool      CheckTrailingStopLong(CPositionInfo *position,double &sl,double &tp);
   virtual bool      CheckTrailingStopShort(CPositionInfo *position,double &sl,double &tp);
protected:
  // round price value to tick size of symbol
  double roundTickSize(double price){
      return int(price/m_symbol.TickSize())*m_symbol.TickSize();
  }
  // round price minimum volume of symbol 100's stocks 1 for future contracts
  double roundVolume(double vol){
      // LotsMin same as SYMBOL_VOLUME_MIN
      return int(vol/m_symbol.LotsMin())*m_symbol.LotsMin();
  }
};

// Default Constructor
void CTrailingMA::CTrailingMA(void) : m_MA(NULL),
                                      m_ma_period(12),
                                      m_ma_method(MODE_EMA),
                                      m_ma_applied(PRICE_CLOSE),
                                      m_MA_last_used(EMPTY_VALUE),
                                      last_pos_id(EMPTY_VALUE)
{}
// Destructor
// void CTrailingMA::~CTrailingMA(void){}
// Non Default Constructor
CTrailingMA::CTrailingMA(ENUM_MA_METHOD ma_method,
      ENUM_APPLIED_PRICE ma_applied, int ma_period){
    m_ma_applied = ma_applied;
    m_ma_period = ma_period;
    m_ma_method = ma_method;
    m_MA_last_used = EMPTY_VALUE;
    last_pos_id = EMPTY_VALUE;
}

// Validation settings protected data.
bool CTrailingMA::ValidationSettings(void)
{
   if(!CExpertTrailing::ValidationSettings())
      return(false);
   return(true);
}
// add MA indicator to the collection of indicator internal call
bool CTrailingMA::InitIndicators(CIndicators *indicators)
{
//--- check
 if(indicators==NULL)
    return(false);
//--- create MA indicator
 if(m_MA==NULL)
    if((m_MA=new CiMA)==NULL)
      {
       printf(__FUNCTION__+": error creating object CTrailingMA indicator");
       return(false);
      }
//--- add MA indicator to the internal CExertBase collection
 if(!indicators.Add(m_MA))
   {
    printf(__FUNCTION__+": error adding object CTrailingMA indicator");
    delete m_MA;
    return(false);
   }
//--- initialize MA indicator (no shift!!)
 if(!m_MA.Create(m_symbol.Name(), m_period, m_ma_period, 0, m_ma_method, m_ma_applied))
   {
    printf(__FUNCTION__+": error initializing object");
    return(false);
   }
 m_MA.BufferResize(2); //  just need 2 samples, now and before

 return(true);
}

// Checking trailing stop and/or profit for long position.  CExpert Call
bool CTrailingMA::CheckTrailingStopLong(CPositionInfo *position, double &sl, double &tp)
{
    //--- check
    if(position==NULL)
      return(false);

      if(position.Identifier() != last_pos_id){ // new position
        m_MA_last_used = EMPTY_VALUE; // reset last used MA
        last_pos_id = position.Identifier();
      }

      double new_sl, ma_now, ma_before;
      ma_now = m_MA.Main(0);// last/or actual indicator value
      ma_before = m_MA.Main(1); // one sample (1 Minute) before the actual indicator

      sl = EMPTY_VALUE;  // due to CExpert checks
      tp = EMPTY_VALUE;

      if(ma_now == EMPTY_VALUE || ma_before == EMPTY_VALUE) // nothing todo without data
        return false;

      if(ma_now > ma_before && (ma_now > m_MA_last_used
                || m_MA_last_used == EMPTY_VALUE)){ // first time called)

        if(m_MA_last_used == EMPTY_VALUE) // first time called)
            m_MA_last_used = ma_before;

        double position_sl=position.StopLoss();
        double position_tp=position.TakeProfit();
        // it is going up compared with last ema value  and with last changed stop
        // increase SL based on the up-change of MA
        new_sl = position.StopLoss() + (ma_now - m_MA_last_used);
        sl = m_symbol.NormalizePrice(new_sl);

        m_MA_last_used = ma_now; // just used it
        return true;
      }

    // double point =  m_symbol.Point();
    // double digits = m_symbol.Digits(); // Forex Trade and Metatrader 4 old code? useless
    // StopsLevel() - step minimum for change in stops - is that really set by CEXpert?
    // double level =NormalizeDouble(m_symbol.Bid()-m_symbol.StopsLevel()*m_symbol.Point(),m_symbol.Digits());
    // double new_sl=NormalizeDouble(m_MA.Main(1),m_symbol.Digits());

    return false;
}

// Checking trailing stop and/or profit for short position. CExpert Call
bool CTrailingMA::CheckTrailingStopShort(CPositionInfo *position,double &sl,double &tp)
{
//--- check
  if(position==NULL)
    return(false);

    if(position.Identifier() != last_pos_id){ // new position
      m_MA_last_used = EMPTY_VALUE;
      last_pos_id = position.Identifier();
    }

      double new_sl, ma_now, ma_before;
      ma_now = m_MA.Main(0);// last/or actual indicator value
      ma_before = m_MA.Main(1); // one sample (1 Minute) before the actual indicator

      sl = EMPTY_VALUE;  // due to CExpert checks
      tp = EMPTY_VALUE;

      if(ma_now == EMPTY_VALUE || ma_before == EMPTY_VALUE) // nothing todo without data
        return false;

  if(ma_now < ma_before && (ma_now < m_MA_last_used
            || m_MA_last_used == EMPTY_VALUE)){ // first time called

    if(m_MA_last_used == EMPTY_VALUE) // first time called
        m_MA_last_used = ma_before;

    double position_sl=position.StopLoss();
    double position_tp=position.TakeProfit();
    // it is going down compared with last ema value  and with last changed stop
    // decrease SL based on the down-change of MA
    new_sl = position.StopLoss() + (ma_now - m_MA_last_used);
    sl = m_symbol.NormalizePrice(new_sl);

    m_MA_last_used = ma_now; // just used it
    return true;
  }

  return false;
}
//+------------------------------------------------------------------+
