// Ignoring for while ask and bid due Clear Server not providing those very often
#include "..\Buffers.mqh"

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
void MoneyBars(MqlTick &ticks[], double tickvalue, double moneybarsize, MoneyBar &bars[]){
  double count_money = 0;
  int i, ibars, nticks = ArraySize(ticks);
  ArrayResize(bars, nticks); // start big resize after
  ibars = 0;
  for(i=0; i<nticks; i++){
      if(count_money >= moneybarsize){
          count_money -= moneybarsize;
        bars[ibars].last = ticks[i].last;
        bars[ibars++].time_msc = ticks[i].time_msc;
      }
      count_money += ticks[i].volume_real*ticks[i].last*tickvalue;
  }
  ArrayResize(bars, ibars);
}

class MoneyBarBuffer : public CStructBuffer<MoneyBar>
{
protected:
    double m_count_money;
    
public:

    MoneyBarBuffer(void){
    }
};

// Add or Increment Array of Money/Dollar/Real Bars from Array of MqlTicks
// moneybarsize value in $$ to form a bar
void AddMoneyBar(double count_money, MqlTick &ticks, double tickvalue, double moneybarsize){
//  double count_money = 0;
//
//    if(count_money >= moneybarsize){
//        count_money -= moneybarsize;
//      bars[ibars].last = ticks[i].last;
//      bars[ibars++].time_msc = ticks[i].time_msc;
//    }
//    count_money += ticks.volume_real*ticks.last*tickvalue;
//
//  ArrayResize(bars, ibars);
}


