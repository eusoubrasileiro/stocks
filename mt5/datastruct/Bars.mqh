#include "../Buffers.mqh"
#include "CBufferMqlTicks.mqh"
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

class MoneyBarBuffer : public CCStructBuffer<MoneyBar>
{
protected:
    double m_count_money;
    double m_point_value;
    double m_moneybarsize;
    MoneyBar m_bar; // temp variable


public:
    int m_nnew; // number of bars nnew on last call
    double m_last[]; // lastest added last prices
    double m_times[]; // ... . ...... times in msc

    ~MoneyBarBuffer(){ ArrayFree(m_last); ArrayFree(m_times); }

    int indexesNewBars(int &parts[]){
        return indexesData(Count()-m_nnew, m_nnew, parts);
    }

    // copy last prices of new bars m_nnew
    void RefreshArrays(){
      ArrayResize(m_last, m_nnew);
      ArrayResize(m_times, m_nnew);
      int parts[4]; int k; int nparts;
      nparts = indexesNewBars(parts);
      for(int i=0, k=0; i<nparts;i++){
        // money bars circular buffer copy in two parts
        start = parts[i*2];
        end = parts[1+i*2];
        for(int j=start; j<end; j++, k++){
            m_last[k] = m_bars.m_data[j].last; // moneybar.last price
            m_times[k] = m_bars.m_data[j].time_msc); //moneybar.time_msc
        }
      }
    }

    MoneyBarBuffer(double tickvalue, double ticksize, double moneybarsize){
      m_point_value = tickvalue/ticksize;
      m_moneybarsize = moneybarsize;
      m_count_money = 0; // count_money amount to form 1 money bar
      m_nnew = 0;
    }

    // add one tick and create as many money bars as needed (or 0)
    // return number created
    int AddTick(MqlTick &tick){
      m_nnew = 0;
      // flags bellow < 16 are only re-quotes
      if(tick.flags > 12){ // at last volume changed - so there was a deal
          m_count_money += tick.volume_real*tick.last*m_point_value;
          while(m_count_money>=m_moneybarsize){ // may need to create many bars for one tick
              // if(ibars == nticks)// ibars may explode if moneybarsize too small
              //   return -1; // dont explode this is a buffer delete older ones
              m_bar.bid = tick.bid;
              m_bar.ask = tick.ask;
              m_bar.last = tick.last;
              m_bar.time_msc = tick.time_msc;
              Add(m_bar);
              m_count_money -= m_moneybarsize;
              m_nnew++;
          }
      }
      return m_nnew;
    }

    // add ticks from bg_idx until size
    int AddTicks(MqlTick &ticks[], int bg_idx, int size){
      int nnew = 0;
      for(int i=bg_idx; i<bg_idx+size; i++)
        nnew += AddTick(ticks[i]);
      m_nnew = nnew; // overwrite internal added increment
      return m_nnew;
    }

    int AddTicks(CCBufferMqlTicks &ticks)
    {
      int nnew = 0;
      int start1, start2, end1, end2;
      ticks.indexesNewTicks(start1, end1, start2, end2);
      for(int i=start1; i<end1; i++)
          nnew += AddTick(ticks.m_data[i]);
      for(int i=start2; i<end2; i++)
          nnew += AddTick(ticks.m_data[i]);
      m_nnew = nnew; // overwrite internal added increment
      return m_nnew;
    }


};
