#define EXPERT_MAGIC 120986  // MagicNumber of the expert

#import "cpparm.dll"
int Unique(double &arr[], int n);
// bins specify center of bins
int  Histsma(double &data[], int n,  double &bins[], int nb, int nwindow);
#import

//string isname = "WING19"; // symbol for orders
// string sname = "WIN@"; // symbol for indicators
// const double MinGap = 30; // minimal Gap to foresee some profit
// const double MaxGap = 300; // maximum Gap where we expect it to close
// const double before_tp = 15; // discount on gap in case it is no reached in full
// // that's applied on take profit before_tp*ticksize'
// // number of contracts to buy for each direction/quantity
// int quantity = 1;
// // tick-size
// const double ticksize=5; // minicontratos ibovespa 5 points price variation
// // deviation accept by price in tick sizes
// const double deviation=5;

//+------------------------------------------------------------------+
//| Inputs                                                           |
//+------------------------------------------------------------------+
//--- inputs for expert
input string Inp_Expert_Title            ="ExpertGapCloser";

input string sname = "PETR4"; // symbol for indicators
input double MinGap = 0.05; // minimal Gap to foresee some profit
input double MaxGap = 0.20; // maximum Gap where we expect it to close
input double before_tp = 0.02; // discount on gap in case it is not reached in full
// that's applied on take profit before_tp*ticksize'
input int quantity = 1000; // number of contracts to buy for each direction/quantity
input double ticksize=0.01; // ticksize points price variation
input double deviation=5; // deviation accept by price in tick sizes

// expert operations end (no further sells or buys) close all positions
input int endhour=16;
input int endminute=00;
const int pivot_order=15;
input int nentry_pivots = 2; // number of orders that will be placed on pivot points
// time to expire a position (close it)
input int expiretime=(1.5)*60*60;

const double trailing_value = 1*ticksize; // independent stoploss for trailing
// handle for EMA of trailing stop smooth
// the price variation on this EMA controls to change on the trailling stop
int    hema=0;
input int  windowema=5; //  EMA for trailing stop 5 minutes
double lastema=1; // last value of the EMA 1 minute
double lastemachange=0; // last EMA value when there was a change on stop loss positive


//--- object for performing trade operations - Trade Class
CTrade  trade;

void ClosePositionbyTime(){
   datetime timenow= TimeCurrent();
   datetime dayend = dayEnd(timenow);
   // NET MODE only ONE buy or ONE sell at once
   int total = PositionsTotal(); // number of open positions
   if(total < 1) // nothing to do
      return;
   ulong  position_ticket = PositionGetTicket(0);  // ticket of the position
   datetime positiontime  = (datetime) PositionGetInteger(POSITION_TIME);
   double volume = PositionGetDouble(POSITION_VOLUME);
   //--- if the MagicNumber matches MagicNumber of the position
   if(PositionGetInteger(POSITION_MAGIC) != EXPERT_MAGIC)
      return;

   if(positiontime +  expiretime > timenow && timenow < dayend )
        return;
    // close whatever position is open NET_MODE
    trade.PositionClose(sname);
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
    return day0hour+endhour*3600+endminute*60;
}


// utils for sorted arrays
int searchGreat( double &arr[], double value){
    int size = ArraySize(arr);
    for(int i=0; i<size; i++)
    if(arr[i] > value)
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

void changeStop(double change) {
   // modify stop loss
   double sl, tp;

   sl = PositionGetDouble(POSITION_SL)  +  change;
   sl = MathFloor(sl/ticksize)*ticksize;
   tp = PositionGetDouble(POSITION_TP);
   tp = MathFloor(tp/ticksize)*ticksize;

    if(!trade.PositionModify(sname, sl, tp))
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
    Histsma(data, ArraySize(data), histsma, nbins, 15); // window 1 means no smooth at all

    double threshold = percentile(histsma, 0.35); // will this work when ...
    ArrayResize(pivots, 5000); // storing selected peaks, if more exception

    bool flag = false;
    int ipivots = 0; // counter of peaks
    for(int i=pivot_order; i<ArraySize(histsma)-pivot_order; i++){
        if(histsma[i] < threshold) // no peaks smaller than this
            continue;
        flag = true;
        for(int j=1; j<pivot_order+1; j++){
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

void classic_pivotPoints(MqlRates &rates[], double &pivots[]){
    // S3 and R3 are too low or too high normally never reached I found
    int size = ArraySize(rates);
    ArrayResize(pivots, size*4);
    double pivot;

    // for every day in mqlrate calculate the pivot points
    for(int i=0; i<size; i++){
        pivot = (rates[i].high + rates[i].low + rates[i].close)/3;
        pivots[i*4] = pivot*2 - rates[i].low; // R1
        pivots[i*4+1] = pivot + rates[i].high - rates[i].low; // R2
        pivots[i*4+2] = pivot*2 - rates[i].high; // S1
        pivots[i*4+3] = pivot - rates[i].high + rates[i].low; // S2
    }

    ArraySort(pivots);
    //size = Unique(pivots, size*4);
    //ArrayResize(pivots, size);
    size = ArraySize(pivots);

    for(int i=0; i<size; i++)
        Print(i+": "+StringFormat("%G", pivots[i]));

}

void camarilla_pivotPoints(MqlRates &rates[], double &pivots[]){
    // S3 and R3 are too low or too high normally never reached I found
    int size = ArraySize(rates);
    ArrayResize(pivots, size*5);
    double pivot, range;

    // for every day in mqlrate calculate the pivot points
    for(int i=0; i<size; i++){
        pivot = (rates[i].high + rates[i].low + rates[i].close)/3;
        range = (rates[i].high - rates[i].low);
        //pivots[i*5] = rates[i].close - range*(1.1/12); // S1
        //pivots[i*5+1] = rates[i].close - range*(1.1/6); // S2
        pivots[i*5] = rates[i].close - range*(1.1/4); // S3
        pivots[i*5+1] = rates[i].close - range*(1.1/2); // S4
//        pivots[i*5+2] = rates[i].close + range*(1.1/12); // R1
//        pivots[i*5+3] = rates[i].close + range*(1.1/6); // R2
        pivots[i*5+2] = pivot;
        pivots[i*5+3] = rates[i].close + range*(1.1/4); // R3
        pivots[i*5+4] = rates[i].close + range*(1.1/2); // R4
    }

    ArraySort(pivots);
    //size = Unique(pivots, size*4);
    //ArrayResize(pivots, size);
    size = ArraySize(pivots);

    for(int i=0; i<size; i++)
        Print(i+": "+StringFormat("%G", pivots[i]));

}


// Camarilla
// Define: Range = High - Low
// Pivot Point (P) = (High + Low + Close) / 3
//
// Support 1 (S1) = Close - Range * (1.1 / 12)
//
// Support 2 (S2) = Close - Range * (1.1 / 6)
//
// Support 3 (S3) = Close - Range * (1.1 / 4)
//
// Support 4 (S4) = Close - Range * (1.1 / 2)
//
// Support 5 (S5) = Close - (R5 - Close)
//
// Resistance 1 (R1) = Close + Range * (1.1 / 12)
//
// Resistance 2 (R2) = Close + Range * (1.1 / 6)
//
// Resistance 3 (R3) = Close + Range * (1.1 / 4)
//
// Resistance 4 (R4) = Close + Range * (1.1 / 2)
//
// Resistance 5 (R5) = (High/Low) * Close
