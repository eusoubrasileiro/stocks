// Ignoring for while ask and bid due Clear Server not providing those very often
#include "../Buffers.mqh"

struct MoneyBar
  {
//   datetime     time;          // Time of the last prices update 1.
//   double       bid;           // Current Bid price
//   double       ask;           // Current Ask price
   double       last;          // Price of the last deal (Last)
//   ulong        volume;        // Volume for the current Last price
   long         time_msc;      // Time of a price last update in milliseconds == 1. but with milliseconds precision
//   uint         flags;         // Tick flags
//   double       volume_real;   // Volume for the current Last price with greater accuracy
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
    MoneyBar m_bar;

public:
    MoneyBarBuffer(double tickvalue, double moneybarsize){
      m_tickvalue = tickvalue;
      m_moneybarsize = moneybarsize;
      m_count_money = 0; // count_money amount to form 1 money bar
    }

    // add one tick and create as many money bars as needed (or 0)
    // return number created or -1 on error
    int AddTick(MqlTick &tick){
      int added = 0;
      m_count_money += tick.volume_real*tick.last*m_tickvalue;
      while(m_count_money>=m_moneybarsize){ // may need to create many bars for one tick
          // if(ibars == nticks)// ibars may explode if moneybarsize too small
          //   return -1; // dont explode this is a buffer delete older ones
          m_bar.last = tick.last;
          m_bar.time_msc = tick.time_msc;
          CStructBuffer<MoneyBar>::Add(m_bar);
          m_count_money -= m_moneybarsize;
          added++;
      }
      return added;
    }

};
