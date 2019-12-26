#include "Util.mqh"
// custom Expert class with additional functionalities
// - Day Trade
//      - Expire positions by time
// - Rounding Volumes and Prices

class CExpertMain : public CExpert
{
  int m_positionExpireTime; // to close a position by time (expire time)
  double m_expertDayEndHour; // Operational window maximum HOUR OF DAY (in hours)
  double m_expertDayStartHour;
  public:

  void setDayTradeParams(double positionExpireHours=1.5, // default close  positions by time 1.5H
                              double dayStartHour=10.25,
                              double dayEndHour=16.57){  // default day end 16:37 hs                              
      m_positionExpireTime = (int) (positionExpireHours*3600); // hours to seconds
      m_expertDayEndHour = dayEndHour;
      m_expertDayStartHour = dayStartHour;
  }

  // round price value to tick size of symbol replaced by m_symbol.NormalizePrice
  // double roundTickSize(double price){
  //     return int(price/m_symbol.TickSize())*m_symbol.TickSize();
  // }
  // round price minimum volume of symbol 100's stocks 1 for future contracts
  double roundVolume(double vol){
      // LotsMin same as SYMBOL_VOLUME_MIN
      return int(vol/m_symbol.LotsMin())*m_symbol.LotsMin();
  }

  bool CloseOpenPositionsbyTime(){
     // Refresh() and SelectPosition() MUST have been called prior to this
     // NET MODE only ONE buy or ONE sell at once
     datetime timenow= TimeCurrent();
     datetime dayend = dayEnd(timenow);
     datetime positiontime  = m_position.Time();
     // Magic check inside CTrade PositionClose
     if(positiontime +  m_positionExpireTime > timenow
                && timenow < dayend )
          return false;
      // close whatever position is open NET_MODE
      m_trade.PositionClose(m_symbol.Name());
      return true;
  }

  bool isInsideDay(){
    // check inside daytrade allowed period
    datetime now = TimeCurrent();
    if(now >= dayEnd(now) || now < dayStart(now) )
        return false;
    return true;
  }

  protected:

  datetime dayEnd(datetime timenow){
      // set end of THIS day for operations, based on _expertDayEndHour
      // http://www.b3.com.br/en_us/solutions/platforms/
      //puma-trading-system/for-members-and-traders/trading-hours/derivatives/indices/
      return GetDayZeroHour(timenow)+int(m_expertDayEndHour*3600);
  }

  datetime dayStart(datetime timenow){
      // set end of THIS day for operations, based on _expertDayEndHour
      // http://www.b3.com.br/en_us/solutions/platforms/
      //puma-trading-system/for-members-and-traders/trading-hours/derivatives/indices/
      return GetDayZeroHour(timenow)+int(m_expertDayStartHour*3600);
  }


};
