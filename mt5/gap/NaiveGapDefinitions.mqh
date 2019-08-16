#define EXPERT_MAGIC 120986  // MagicNumber of the expert

#import "cpparm.dll"
int Unique(double &arr[], int n);
// bins specify center of bins
int  Histsma(double &data[], int n,  double &bins[], int nb, int nwindow);
#import

//+------------------------------------------------------------------+
//| Inputs                                                           |
//+------------------------------------------------------------------+
input double minimumGapSize = 0.06; // Minimal Gap size to enter (entry condition)
//foresee some profit
input double maximumGapSize = 0.35; // Maximum Gap size to enter (entry condition)
//  where we expect it to close
input double discountGap = 0.02; // Discount on gap in case it is not reached in full
// that's applied on take profit discountGap*ticksize'
input double orderSize = 100e3; // each order Size in $$ for stocks or future contracts
// based on tick-value
input double orderDeviation = 3; // Price orderDeviation accepted in tickSizes
input int typePivots = 1; // type of pivots 1 classic, 2 camarilla, 3 fibo
// expert operations end (no further sells or buys) close all positions
input double expertEndHour = 15.5; // Operational window maximum day hour
input int expertnDaysPivots = 6; // Number of previous days to calc. pivots
input double expertRewardRiskRatio = 0.25; // Pending orders can't have rewar-risk smaller than this
// looking at formulas means all 4/5 pivots calculated will be equal to open price
// if daily bar ohlc is all equal but depending on the time it's called ohlc for
// current day not completed ohlc will not be equal
input int expertnOdersperDay = 2; // Number of orders placed per day
input double expertPositionExpireHours = 1.5; // Time to expire a position (close it)
int positionExpireTime; // expertPositionExpireHours converted to seconds on Expert Init
input int  trailingEmaWindow = 5; //  EMA Trailing Stop Window in M1
// handle for EMA of trailing stop smooth
// the price variation on this EMA controls to change on the trailling stop
int    hema;
double lastema=1; // last value of the EMA 1 minute
double lastemachange=0; // last EMA value when there was a change on stop loss positive
double tickSize; // will be initiliazed on Expert Init
double tickValue; // value of minimum variation in price in $$$

// round price value to tick size of symbol
double roundTickSize(double price){
    return MathFloor(price/tickSize)*tickSize;
}

//--- object for performing trade operations - Trade Class
CTrade  trade;

void ClosePositionbyTime(){
   datetime timenow= TimeCurrent();
   datetime dayend = dayEnd(timenow);
   // NET MODE only ONE buy or ONE sell at once
   int total = PositionsTotal(); // number of open positions
   if(total < 1) // nothing to do
      return;
   datetime positiontime  = (datetime) PositionGetInteger(POSITION_TIME);
   //--- if the MagicNumber matches MagicNumber of the position
   if(PositionGetInteger(POSITION_MAGIC) != EXPERT_MAGIC)
      return;
   if(positiontime +  positionExpireTime > timenow && timenow < dayend )
        return;
    // close whatever position is open NET_MODE
    trade.PositionClose(Symbol());
    Print("Closed by Time");
}

datetime dayEnd(datetime timenow){
    // set end of THIS day for operations, 15 minutes before closing the stock market
    // http://www.b3.com.br/en_us/solutions/platforms/puma-trading-system/for-members-and-traders/trading-hours/derivatives/indices/
    datetime day0hour;
    MqlDateTime mqltime;
    TimeToStruct(timenow, mqltime);
    // calculate begin of the day
    day0hour = timenow - (mqltime.hour*3600+mqltime.min*60+mqltime.sec);
    return day0hour+int(expertEndHour*3600);
}


// utils for sorted arrays
int searchGreat( double &arr[], double value){
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


void CloseOpenOrders(){
    ulong ticket;
    int total = OrdersTotal();
    bool result =  false;
    for(int i=total-1; i>=0; i--)
    {
        ticket=OrderGetTicket(i);
        result = trade.OrderDelete(ticket);
        if(!result)
            Print("DeleteOrder error ", GetLastError());
    }
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
    prices[i] = roundTickSize(prices[i]);

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

// modify stop loss
void changeStop(double change) {
   double sl, tp;

   sl = PositionGetDouble(POSITION_SL)  +  change;
   sl = roundTickSize(sl);
   tp = PositionGetDouble(POSITION_TP);
   tp = roundTickSize(tp);

    if(!trade.PositionModify(Symbol(), sl, tp))
        Print("OrderSend  Change SL error ",GetLastError());
}

bool TrailingStopLossEma(){
   double ema[1]; // actual ema value
   bool trailled = false;

   if (CopyBuffer(hema, 0,  0,  1, ema) != 1){
      Print("CopyBuffer from Stdev failed");
   }
   if( PositionsTotal() > 0){        // net mode
         if( PositionGetInteger(POSITION_TYPE) == POSITION_TYPE_BUY &&
            PositionGetInteger(POSITION_MAGIC) == EXPERT_MAGIC){
            if(ema[0] > lastema && ema[0] > lastemachange) {
             // it is going up steady  compared with last ema value  and with last changed stop
               changeStop(ema[0]-lastema);
               trailled = true;
               lastemachange = ema[0];
            }
         }
         if( PositionGetInteger(POSITION_TYPE) == POSITION_TYPE_SELL &&
            PositionGetInteger(POSITION_MAGIC) == EXPERT_MAGIC){
            if(ema[0] < lastema && ema[0] < lastemachange) {
             // it is going up steady  compared with last ema value  and with last changed stop
               changeStop(ema[0]-lastema);
               trailled = true;
               lastemachange = ema[0];
            }
         }
   }
   lastema = ema[0];
   return trailled;
}

int classic_pivotPoints(MqlRates &rates[], double &pivots[]){
    // S3 and R3 are too low or too high normally never reached I found
    int size = ArraySize(rates);
    ArrayResize(pivots, size*4);
    double pivot;

    // for every day in mqlrate calculate the pivot points
    for(int i=0; i<size; i++){
        pivot = (rates[i].high + rates[i].low + rates[i].close)/3;
        pivots[i*4] = roundTickSize(pivot*2 - rates[i].low); // R1
        pivots[i*4+1] = roundTickSize(pivot + rates[i].high - rates[i].low); // R2
        pivots[i*4+2] = roundTickSize(pivot*2 - rates[i].high); // S1
        pivots[i*4+3] = roundTickSize(pivot - rates[i].high + rates[i].low); // S2
    }

    ArraySort(pivots);
    size = ArraySize(pivots);
    return size;
}

// R3 = PP + ((High – Low) x 1.000)
// R2 = PP + ((High – Low) x .618)
// R1 = PP + ((High – Low) x .382)
// PP = (H + L + C) / 3
// S1 = PP – ((High – Low) x .382)
// S2 = PP – ((High – Low) x .618)
// S3 = PP – ((High – Low) x 1.000)
int fibonacci_pivotPoints(MqlRates &rates[], double &pivots[]){
    // S3 and R3 are too low or too high normally never reached I found
    int size = ArraySize(rates);
    ArrayResize(pivots, size*5);
    double pivot, range;

    // for every day in mqlrate calculate the pivot points
    for(int i=0; i<size; i++){
        pivot = (rates[i].high + rates[i].low + rates[i].close)/3;
        range = (rates[i].high - rates[i].low);
        pivots[i*4] = roundTickSize(pivot - range*1); // S3
        pivots[i*4+1] = roundTickSize(pivot - range*0.618); // S2
        pivots[i*4+2] = roundTickSize(pivot + range*0.618); // R2
        pivots[i*4+3] = roundTickSize(pivot + range*1); // R3
    }

    ArraySort(pivots);
    size = ArraySize(pivots);

    return size;
}

// camarilla is the only one using the pivot point as an pivot value
int camarilla_pivotPoints(MqlRates &rates[], double &pivots[]){
    // S3 and R3 are too low or too high normally never reached I found
    int size = ArraySize(rates);
    ArrayResize(pivots, size*5);
    double pivot, range;

    // for every day in mqlrate calculate the pivot points
    for(int i=0; i<size; i++){
        pivot = roundTickSize((rates[i].high + rates[i].low + rates[i].close)/3);
        range = roundTickSize((rates[i].high - rates[i].low));
        //pivots[i*5] = rates[i].close - range*(1.1/12); // S1
        //pivots[i*5+1] = rates[i].close - range*(1.1/6); // S2
        pivots[i*5] = roundTickSize(rates[i].close - range*(1.1/4)); // S3
        pivots[i*5+1] = roundTickSize(rates[i].close - range*(1.1/2)); // S4
//        pivots[i*5+2] = rates[i].close + range*(1.1/12); // R1
//        pivots[i*5+3] = rates[i].close + range*(1.1/6); // R2
        pivots[i*5+2] = roundTickSize(pivot);
        pivots[i*5+3] = roundTickSize(rates[i].close + range*(1.1/4)); // R3
        pivots[i*5+4] = roundTickSize(rates[i].close + range*(1.1/2)); // R4
    }

    ArraySort(pivots);
    size = ArraySize(pivots);

    return size;
}

// Camarilla
// Define: Range = High - Low
// Pivot Point (P) = (High + Low + Close) / 3
// Support 1 (S1) = Close - Range * (1.1 / 12)
// Support 2 (S2) = Close - Range * (1.1 / 6)
// Support 3 (S3) = Close - Range * (1.1 / 4)
// Support 4 (S4) = Close - Range * (1.1 / 2)
// Support 5 (S5) = Close - (R5 - Close)
// Resistance 1 (R1) = Close + Range * (1.1 / 12)
// Resistance 2 (R2) = Close + Range * (1.1 / 6)
// Resistance 3 (R3) = Close + Range * (1.1 / 4)
// Resistance 4 (R4) = Close + Range * (1.1 / 2)
// Resistance 5 (R5) = (High/Low) * Close