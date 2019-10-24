#include "../Buffers.mqh"
//#include "Time.mqh"
// ask and bid prices are not present on summary symbols like WIN@ WDO@ WIN$
// but are present on WINV19 etc. stocks PETR4 etc...

// Money Bar solves many problems of analysis of stocks data
// stationarity and also empty volumes
struct MoneyBar
  {
//   datetime     time;          // Time of the last prices update 1.
   double       bid;           // Current Bid price - might be 0 WHEN Equal the LAST
   double       ask;           // Current Ask price - might be 0 WHEN Equal the LAST
   double       last;          // Price of the last deal (Last) - never 0 when volume not 0 - zero when no match between ask-bid
//   ulong        volume;        // Volume for the current Last price 2.
   long         time_msc;       // Time of a price last update in milliseconds == 1. but with milliseconds precision
//   int          day;             // day of year
//   uint         flags;         // Tick flags - no reason to use this on money-bar - shows what changed from the previous tick
//   double       volume_real;   // Volume for the current Last price with greater accuracy same as 2. but better
  };

// Create Array of Money/Dollar/Real Bars from Array of MqlTicks
// moneybarsize value in $$ to form a bar
int MoneyBars(MqlTick &ticks[], double tickvalue, double moneybarsize, MoneyBar &bars[]){
  double count_money = 0;
  int i, ibars, nticks = ArraySize(ticks);
  ArrayResize(bars, nticks); // start big resize after
  ibars = 0;
  for(i=0; i<nticks; i++){
      count_money += ticks[i].volume_real*ticks[i].last*tickvalue;
      while(count_money>=moneybarsize){ // may need to create many bars for one tick
          if(ibars == nticks)// ibars may explode if moneybarsize too small
            return -1;
          bars[ibars].bid = ticks[i].bid;
          bars[ibars].ask = ticks[i].ask;
          bars[ibars].last = ticks[i].last;
          bars[ibars++].time_msc = ticks[i].time_msc;
          count_money -= moneybarsize;
      }

  }
  ArrayResize(bars, ibars);
  return ibars;
}

class MoneyBarBuffer : public CStructBuffer<MoneyBar>
{
protected:
    double m_count_money;
    double m_tickvalue;
    double m_moneybarsize;
    MoneyBar m_bar; // temp variable


public:
    int m_added; // number of bars added on last call

    MoneyBarBuffer(double tickvalue, double moneybarsize){
      m_tickvalue = tickvalue;
      m_moneybarsize = moneybarsize;
      m_count_money = 0; // count_money amount to form 1 money bar
      m_added = 0;
    }

    // add one tick and create as many money bars as needed (or 0)
    // return number created or -1 on error
    int AddTick(MqlTick &tick){
      m_added = 0;
      m_count_money += tick.volume_real*tick.last*m_tickvalue;
      while(m_count_money>=m_moneybarsize){ // may need to create many bars for one tick
          // if(ibars == nticks)// ibars may explode if moneybarsize too small
          //   return -1; // dont explode this is a buffer delete older ones
          m_bar.bid = tick.bid;
          m_bar.ask = tick.ask;
          m_bar.last = tick.last;
          m_bar.time_msc = tick.time_msc;
          CStructBuffer<MoneyBar>::Add(m_bar);
          m_count_money -= m_moneybarsize;
          m_added++;
      }
      return m_added;
    }

    // add ticks from bg_idx until size
    int AddTicks(MqlTick &ticks[], int bg_idx, int size){
      int added = 0;
      for(int i=bg_idx; i<size; i++)
        added += AddTick(ticks[i]);
      m_added = added; // overwrite internal added increment
      return m_added;
    }

};
