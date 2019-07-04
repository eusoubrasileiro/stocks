// Trailing stop only for buy positions
// must be included after Defitions file include
// where must be defined:
// - sname : symbol name
// - ticksize : tick size for rounding orders
// - EXPERT_MAGIC
// - starthour
// - startminute
// - endhour
// - endminute
// - perdt

//
//class TrailingStopEma{
//    // handle for EMA of trailing stop smooth
//    // the price variation on this EMA controls to change on the trailling stop
//    int  hema;
//    int  windowema; // 10 minutes
//    double lastema; // last value of the EMA 1 minute
//    double lastemachange; // last EMA value when there was a change on stop loss positive
//
//    void changeStop(double change) {
//       // modify stop loss
//       MqlTradeRequest request;
//       MqlTradeResult  result;
//       //   double stops=0;
//       request.action = TRADE_ACTION_SLTP;
//       request.symbol = Symbol();
//       request.sl = PositionGetDouble(POSITION_SL)  +  change;
//       request.sl = MathFloor(request.sl/ticksize)*ticksize;
//       request.tp = PositionGetDouble(POSITION_TP) + change;
//       request.tp = MathFloor(request.tp/ticksize)*ticksize;
//        if(!OrderSend(request, result))
//            Print("OrderSend error ",GetLastError());
//    }
//
//    public:
//        void TrailingStopEma(int window, enum ENUM_TIMEFRAMES, enum ENUM_APPLIED_PRICE){
//            // ENUM_TIMEFRAMES can be PERIOD_M1 etc.
//            windowema = window;
//            lastema = 1;
//            lastemachange = 0;
//            hema = iMA(sname, ENUM_TIMEFRAMES, windowema, 0, MODE_EMA, ENUM_APPLIED_PRICE);
//
//            if(hema == INVALID_HANDLE)
//                Print("Error creating indicator trailing-stop");
//        }
//        void TrailingStopEma(); // default constructor mandatory
//
//        bool trailStop(){
//           double ema[1]; // actual ema value
//           bool trailled = false;
//
//           if (CopyBuffer(hema, 0,  0,  1, ema) != 1)
//              Print("CopyBuffer for Trailing Stop failed");
//           if( PositionsTotal() > 0){        // net mode
//                 if( PositionGetInteger(POSITION_TYPE) == POSITION_TYPE_BUY &&
//                    PositionGetInteger(POSITION_MAGIC) == EXPERT_MAGIC){
//                    if(ema[0] > lastema && ema[0] > lastemachange) {
//                     // it is going up steady  compared with last ema value  and with last changed stop
//                       changeStop(ema[0]-lastema);
//                       trailled = true;
//                       lastemachange = ema[0];
//                    }
//                 }
//           }
//           lastema = ema[0];
//           return trailled;
//        }
//
//        void clean(){
//            lastemachange = 0;
//        }
//}

void ClosePositionbyTime(){
   MqlTradeRequest request;
   MqlTradeResult  result;
   datetime timenow= TimeCurrent();
   datetime dayend = dayEnd(timenow);
   datetime daybegin= dayBegin(timenow);
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
    request.price   = SymbolInfoDouble(sname, SYMBOL_BID);
    request.type    =  ORDER_TYPE_SELL;

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

datetime dayBegin(datetime timenow){
    datetime day0hour;
    MqlDateTime mqltime;
    TimeToStruct(timenow, mqltime);
    // no orders on the first 30 minutes
    day0hour = timenow - (mqltime.hour*3600+mqltime.min*60+mqltime.sec);
    return day0hour+starthour*3600+startminute*60;
}

//  number of orders oppend on the last n minutes defined on Definitons.mqh
int nlastDeals(){
    int lastnopen=0; // number of orders openned on the last n minutes
    //--- request trade history
    datetime now = TimeCurrent();
    // on the last 15 minutes count the openned orders
    HistorySelect(now-60*perdt,now);
    ulong ticket;
    long entry;
    uint  total=HistoryDealsTotal(); // total deals on the last n minutes
    //--- for all deals
   for(uint i=0;i<total;i++){
      //--- try to get deals ticket
      ticket=HistoryDealGetTicket(i);
      if(ticket>0){ // get the deal entry property
         entry =HistoryDealGetInteger(ticket, DEAL_ENTRY);
         if(entry==DEAL_ENTRY_IN) // a buy or a sell (entry not closing/exiting)
            lastnopen++;
      }
    }
    return lastnopen;
}

int ndealsDay(){
    int open=0; // number of orders openned
    //--- request trade history
    datetime now = TimeCurrent();
    // on the last 15 minutes count the openned orders
    HistorySelect(dayBegin(now), now);
    ulong ticket;
    long entry;
    uint  total=HistoryDealsTotal(); // total deals
    //--- for all deals
   for(uint i=0;i<total;i++){
      //--- try to get deals ticket
      ticket=HistoryDealGetTicket(i);
      if(ticket>0){ // get the deal entry property
         entry =HistoryDealGetInteger(ticket,DEAL_ENTRY);
         if(entry==DEAL_ENTRY_IN) // a buy or a sell (entry not closing/exiting)
            open++;
      }
    }
    return open;
}

void pivotPoints(MqlRates &rates[], double &pivots[]){
    // S3 and R3 are too low or too high normally never reached I found
    int size = ArraySize(rates);
    ArrayResize(pivots, size*4);

    // for every day in mqlrate calculate the pivot points
    for(int i=0; i<size; i++){
        pivot = (rates[i].high + rates[i].low + rates[i].close)/3;
        pivots[i*4] = pivot*2 - rates[i].low; // R1
        pivots[i*4+1] = pivot + rates[i].high - rates[i].low; // R2
        pivots[i*4+2] = pivot*2 - rates[i].high; // S1
        pivots[i*4+3] = pivot - rates[i].high + rates[i].low; // S2
    }

    ArraySort(pivots);
}
