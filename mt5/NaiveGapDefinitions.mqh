#define EXPERT_MAGIC 1  // MagicNumber of the expert

#import "cpparm.dll"
int Unique(double &arr[], int n);
#import

//string isname = "WING19"; // symbol for orders
string sname = "WDO$"; // symbol for indicators
const double MinGap = 15.5; // minimal Gap to foresee some profit
const double MaxGap = 220.50; // maximum Gap where we expect it to close
const double before_tp = 3.5; // discount on gap in case it is no reached in full 
// that's applied on take profit before_tp*ticksize'
// number of contracts to buy for each direction/quantity
int quantity = 1;
// tick-size
const double ticksize=0.5; // minicontratos ibovespa 5 points price variation
// deviation accept by price in tick sizes
const double deviation=5;
// expert operations end (no further sells or buys) close all positions
const int endhour=15;
const int endminute=30;

//--- object for performing trade operations - Trade Class
CTrade  trade;

void DayEndingClosePositions(){
   MqlTradeRequest request;
   MqlTradeResult  result;
   // NET MODE only ONE buy or ONE sell at once
   int total = PositionsTotal(); // number of open positions
   if(total < 1) // nothing to do
      return;
   datetime timenow= TimeCurrent();
   datetime dayend = dayEnd(timenow);
   ulong  position_ticket = PositionGetTicket(0);  // ticket of the position

   double volume = PositionGetDouble(POSITION_VOLUME);
   //--- if the MagicNumber matches MagicNumber of the position
   if(PositionGetInteger(POSITION_MAGIC) != EXPERT_MAGIC)
      return;

   if(timenow < dayend )
        return;
    // close whatever volume is open
    //--- zeroing the request and result values
    ZeroMemory(request);
    ZeroMemory(result);
    //--- setting the operation parameters
    request.action   = TRADE_ACTION_DEAL;        // type of trade operation
    request.position = position_ticket;          // ticket of the position
    request.symbol = sname;          // symbol
    request.volume = volume;                   // volume of the position
    request.deviation = deviation*ticksize;                        // 7*0.01 tick size : 7 cents
    request.magic = EXPERT_MAGIC;             // MagicNumber of the position

    int position_type = PositionGetInteger(POSITION_TYPE);

    if(position_type == POSITION_TYPE_BUY){
        request.price   = SymbolInfoDouble(sname, SYMBOL_BID);
        request.type    =  ORDER_TYPE_SELL;
    }
    else{
        request.price   = SymbolInfoDouble(sname, SYMBOL_ASK);
        request.type    =  ORDER_TYPE_BUY;
    }

    if(!OrderSend(request,result))
      Print("OrderSend error - Close by Time ", GetLastError());
    //--- information about the operation
    Print("Closed by Time - retcode ",result.retcode," deal ",result.deal);
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


void pivotPoints(MqlRates &rates[], double &pivots[]){
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
    size = Unique(pivots, size*4);
    ArrayResize(pivots, size);
    size = ArraySize(pivots);

    for(int i=0; i<size; i++)
        Print(i+": "+StringFormat("%G", pivots[i]));
        
}
