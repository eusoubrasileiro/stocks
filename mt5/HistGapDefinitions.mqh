#define EXPERT_MAGIC 120986  // MagicNumber of the expert

#import "cpparm.dll"
int Unique(double &arr[], int n);
// bins specify center of bins
int  Histsma(double &data[], int n,  double &bins[], int nb, int nwindow);
#import

//+------------------------------------------------------------------+
//| Inputs                                                           |
//+------------------------------------------------------------------+
// input string targetSymbol = "PETR4"; // Symbol for negotiation // use current Symbol
input double minimumGapSize = 30; // Minimal Gap size to enter (entry condition)
//foresee some profit
input double maximumGapSize = 300; // Maximum Gap size to enter (entry condition)
//  where we expect it to close
input double discountGap = 15; // Discount on gap in case it is not reached in full
// that's applied on take profit discountGap*ticksize'
input int orderSize = 1; // Number of contracts to buy for each direction/orderSize
input double orderDeviation = 5; // Price orderDeviation accepted in tickSizes
// expert operations end (no further sells or buys) close all positions
input double expertEndHour = 15.5; // Operational window maximum day hour
input int expertnDaysPivots = 6; // Number of previous days to calc. pivots
input int expertUseCurrentDay = 0; // 0 means use - 1 means don't (CopyRates)
// looking at formulas means all 4/5 pivots calculated will be equal to open price
// if daily bar ohlc is all equal but depending on the time it's called ohlc for
// current day not completed ohlc will not be equal
input int expertEntries = 2; // Number of orders placed per day
input double expertPositionExpireHours = 1.5; // Time to expire a position (close it)
int positionExpireTime; // expertPositionExpireHours converted to seconds on Expert Init
input int  trailingEmaWindow = 5; //  EMA Trailing Stop Window in M1
input int histPeakIntervalOrder = 15; // Min Interval Between Peaks in Hist (typePivots=4)
// handle for EMA of trailing stop smooth
// the price variation on this EMA controls to change on the trailling stop
int    hema;
double lastema=1; // last value of the EMA 1 minute
double lastemachange=0; // last EMA value when there was a change on stop loss positive
double tickSize; // will be initiliazed on Expert Init

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


int searchGreatEqual( double &arr[], double value){
    int size = ArraySize(arr);
    for(int i=0; i<size; i++)
        if(arr[i] >= value)
            return i;
    return -1;
}


int searchLess(double &arr[], double value){
    int size = ArraySize(arr);
    for(int i=size; i>=0; i--)
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


int pivotsHistSMA(double &data[], double binsize, double &pivots[]){
    double bins[]; // first bins after calling Histsma bin counts (histogram) smoothed
    double histsma[];
    double vmin = data[ArrayMinimum(data)];
    double vmax = data[ArrayMaximum(data)];

    int nbins = arange(vmin-binsize, vmax+binsize, binsize, bins);
    ArrayCopy(histsma, bins, 0, 0, WHOLE_ARRAY); // copy bins to histsma will be input bellow
    // for win futures 15 smooth equals 15*5 = 75 points between maximums
    // instead of histPeakIntervalOrder it was 15 of smooth window
    Histsma(data, ArraySize(data), histsma, nbins, 5); // window 1 means no smooth at all

    double threshold = percentile(histsma, 0.35); // will this work when ...
    ArrayResize(pivots, 5000); // storing selected peaks, if more exception

    bool flag = false;
    int ipivots = 0; // counter of peaks
    for(int i=histPeakIntervalOrder; i<ArraySize(histsma)-histPeakIntervalOrder; i++){
        if(histsma[i] < threshold) // no peaks smaller than this
            continue;
        flag = true;
        for(int j=1; j<histPeakIntervalOrder+1; j++){
            if(histsma[i] <= histsma[i-j] || histsma[i] <= histsma[i+j]){
                flag = false;
                break; // not a maximum
            }
        }
        if(flag){
            pivots[ipivots] = bins[i]; // or histsma[i] for histogram count for that
            ipivots++;
        }
    }

    ArrayResize(pivots, ipivots);
    ArraySort(pivots);

    for(int i=0; i<ipivots; i++)
        Print(i+": "+StringFormat("%G", pivots[i]));

    return ipivots;
}
