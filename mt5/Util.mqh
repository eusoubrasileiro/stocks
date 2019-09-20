#include "Expert.mqh"
#include "TrailingMA.mqh"
#include <Arrays\ArrayDouble.mqh>
#include <Trade\Trade.mqh>

#import "cpparm.dll"
int Unique(double &arr[],int n);
// bins specify center of bins
int  Histsma(double &data[],int n,double &bins[],int nb,int nwindow);
#import

// custom Expert class with additional functionalities
// - Day Trade
//      - Expire positions by time
// - Rounding Volumes and Prices

class CExpertX : public CExpert
{
  int m_positionExpireTime; // to close a position by time (expire time)
  double m_expertDayEndHour; // Operational window maximum HOUR OF DAY (in hours)

  public:
  CTrade *trader;
  CSymbolInfo *symbol;

                     CExpertX(void){};
                    ~CExpertX(void){};

  void setDayTradeParams(double positionExpireHours=100, // default dont close  positions by time
                              double dayEndHour=16.5){ // default day end 16:30 hs
      m_positionExpireTime = (int) (positionExpireHours*3600); // hours to seconds
      m_expertDayEndHour = dayEndHour;
  }

  bool setPublic(){ // make protect usefull variables public
    // can only be called after CExpertInit()
    if(m_trade != NULL && m_symbol != NULL){
      trader = GetPointer(m_trade);
      symbol = GetPointer(m_symbol);
      return true;
    }
    return false;
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
     //--- if the MagicNumber matches MagicNumber of the position
     // Magic check inside CTrade PositionClose
     //if(PositionGetInteger(POSITION_MAGIC) != m_magic)
     //   return false;
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
    if(now >= dayEnd(now))
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

};

// calculate begin of the day Zero Hour
// TODO OPTIMIZE it to make use of the 8 bytes
// UNIX timestamp in seconds since 1970 only 8 bytes
datetime GetDayZeroHour(datetime time){ // can be a unique day identifier
    MqlDateTime mqltime;
    TimeToStruct(time, mqltime);
    return time - (mqltime.hour*3600+mqltime.min*60+mqltime.sec);
}

// utils for sorted arrays
int searchGreat(double &arr[], double value){
    int size = ArraySize(arr);
    for(int i=0; i<size; i++)
        if(arr[i] > value)
            return i;
    return -1;
}

// Search for a value smaller than value in a sorted array
int searchLess(double &arr[], double value){
    int size = ArraySize(arr);
    for(int i=size-1; i>=0; i--) // start by the end
        if(arr[i] < value)
            return i;
    return -1;
}


int arange(double start, double stop, double step, double &arr[]){
    int size = MathFloor((stop-start)/step)+1;
    ArrayResize(arr, size);
    for(int i=0; i<size; i++)
        arr[i] = start + step*i;
    return size;
}

  // a naive histogram were empty bins are not taken in account
  // prices must be sorted
  // prices will be rounded to tickSize to define bins
  // empty bins will not be taken in account
int naivehistPriceTicksize(double &prices[], double &bins[], int &count_bins[]){
  int nprices = ArraySize(prices);
  // bins[] bins with unique price values
  // round prices to tickSize
  for(int i=0; i<nprices; i++)
    prices[i] = prices[i];

  ArrayCopy(bins, prices); // 'Unique' armadillo overwrites input array
  int nbins = Unique(bins, nprices); // get unique price values
  ArrayResize(bins, nbins); // bins based on unique values
  ArrayResize(count_bins, nbins);

  // naive histogram of counts pretty fast because prices is sorted
  // and rounded
  for(int i=0; i<nbins; i++){
      count_bins[i] = 0;
      for(int j=0; j<nprices; j++)
          if(prices[j] == bins[i])
              count_bins[i] += 1;
  }
  return nbins;
}

double percentile(double &data[], double perc){
    double sorted[];
    ArrayCopy(sorted, data, 0, 0, WHOLE_ARRAY);
    int asize = ArraySize(sorted);
    ArraySort(sorted);
    int n = MathMax(MathRound(perc * asize + 0.5), 2);
    return sorted[n-2];
}
