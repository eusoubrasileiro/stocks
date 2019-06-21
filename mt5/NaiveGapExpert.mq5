#property copyright "Andre L. Ferreira"
#property version   "1.01"
#include "GapDefinitions.mqh"

// 5 days simple moving average window for stdev calculation on H4 time frame
const int  window=5*2;
int hstdev;
MqlDateTime previousday;
bool alreadyin = false; // // did not entry yet

//| Expert initialization function
int OnInit(){

   EventSetTimer(5);
   hstdev = iStdDev(sname, PERIOD_H4, window, 0, MODE_SMA, PRICE_CLOSE);

   if(hstdev == INVALID_HANDLE){
      Print("Gap Expert : Error creating indicator");
      return(INIT_FAILED);
   }

   // start day is the previous day
   TimeCurrent(previousday);
   alreadyin = false;

   // time in seconds from 1970 current time
   Print("Begining Gap Expert now: ", TimeCurrent());
   return(INIT_SUCCEEDED);
}


//// will not be needed
//double percentile(double &data[], double perc)
//    double sorted[];
//    ArrayCopy(sorted, data, 0, 0, WHOLE_ARRAY)
//    as = ArraySize(sorted)
//    ArraySort(sorted)
//    int n = MathMax(MathRound(perc * as + 0.5), 2)
//    return sorted[n-2]

// bool isBuyPattern(){ // is up trend
//     double stdh[5];
//     double stdl[5];
//     double emah[6];
//     double emal[6];
//     bool isbuypattern = true;
//
//     if (CopyBuffer(hstdevh, 0, 0, 5, stdh) != 5 ||
//         CopyBuffer(hstdevl, 0, 0, 5, stdl) != 5 ||
//         CopyBuffer(hemah, 0, 0, 6, emah) != 6 || // 5+1 due diff
//         CopyBuffer(hemal, 0, 0, 6, emal) != 6
//         ){
//        Print("CopyBuffer from isPattern failed");
//        return false;
//     }
//     for(int i=1; i<6; i++)
//         if(emah[i]-emah[i-1] <= 0 || emal[i]-emal[i-1] <=0 ||
//            stdh[i-1] <= 15 || stdh[i-1] >= 90 ||
//            stdl[i-1] <= 15 || stdl[i-1] >= 90 || stdl[i-1]  > stdh[i-1] + 40 || stdh[i-1]  > stdl[i-1] + 40 ){
//                isbuypattern = false;
//                break;
//            }
//     return isbuypattern;
// }

double stopStdev(){
    double stdev[1];
    if (CopyBuffer(hstdev, 0, 0, 1, stdev) != 1){
       Print("CopyBuffer from Stdev failed");
       return 0;
       }
    return stdev[0]*2;
}

void PlaceOrder(int sign, double tp){
    MqlTradeRequest request={0};
    MqlTradeResult result={0};
    int ncontracts = 1;
    double stop;

    // stop loss based on standard deviation of last 5 days on H4 time-frame
    stop = stopStdev();
    if(stop == 0)
      stop = 1000;

    //--- parameters of request
    request.action=TRADE_ACTION_DEAL;      // type of trade operation
    request.symbol=sname;                               // symbol
    //+ postive buy order
    if(sign > 0){
      request.price= SymbolInfoDouble(request.symbol, SYMBOL_ASK); // ask price
      request.type=ORDER_TYPE_BUY;
      // stop loss and take profit 3:1 rount to 5
      request.tp = tp;
      request.tp = MathFloor(request.tp/ticksize)*ticksize;
      double range = request.tp - request.price;
      //request.sl = request.price-stop;
      request.sl = request.price - 5*range; // 1:3
      request.sl = MathFloor(request.sl/ticksize)*ticksize;
    }
    else{
      request.price= SymbolInfoDouble(request.symbol, SYMBOL_BID); // ask price
      request.type=ORDER_TYPE_SELL;
      // stop loss and take profit 3:1 rount to 5
      request.tp = tp;
      request.tp = MathFloor(request.tp/ticksize)*ticksize;
      double range =  request.price - request.tp;
      //request.sl = request.price+stop;
      request.sl = request.price + 5*range; // 1:3
      request.sl = MathFloor(request.sl/ticksize)*ticksize;
    }

    request.volume=quantity*ncontracts; // volume executed in contracts
    request.deviation=deviation*ticksize;    //  allowed deviation from the price
    request.magic=EXPERT_MAGIC;   // MagicNumber for this Expert
    if(!OrderSend(request,result))
        Print("BuyNow OrderSend error ",GetLastError());
    //--- information about the operation
    Print("retcode ",result.retcode,"  deal ",result.deal);
    //--- output information about the closure by opposite position
}



//| Timer function -- Every 5 minutes
void OnTimer() {
    double pdayclose[2];
    double todayopen[1]; // previous day close and open today
    MqlDateTime todaynow;
    int copied, gapsign;
    double gap;

    TimeCurrent(todaynow);

    if(todaynow.day != previousday.day){

      copied = CopyClose(sname, PERIOD_D1, 0, 2, pdayclose);
      if( copied == -1){
        Print("Failed to get previous day data");
        return;
      }

      copied = CopyOpen(sname, PERIOD_M5, 0, 1, todayopen);
      if( copied == -1){
        Print("Failed to get today data");
        return;
      }

      previousday = todaynow;  // just entered a position today
      
      gap = todayopen[0] - pdayclose[0];
      // positive gap will close so it is a sell order
      // negative gap will close si it is a by order
      if (MathAbs(gap) < 250 && MathAbs(gap) > 50 ){ // Go in
        gapsign = gap/MathAbs(gap);
        // place take profit 25 points before gap close
        PlaceOrder(-gapsign, pdayclose[0]+(gapsign*25));        
      }

      
    }

    // in case did not reach the take profit close it by the day end
    // check to see if we should close any position
    DayEndingClosePositions();
 }

// Useless stuff
//| Expert deinitialization function                                 |
void OnDeinit(const int reason){
    EventKillTimer();
}

//| Expert tick function                                             |
void OnTick(){

}

//| Trade function                                                   |
void OnTrade(){
     // when position is closed zeroed the lastemachange
    //if(PositionsTotal()==0 || PositionGetInteger(POSITION_MAGIC) != EXPERT_MAGIC)
    //    tsop.clean();
}

//| TradeTransaction function                                        |
void OnTradeTransaction(const MqlTradeTransaction &trans,
                        const MqlTradeRequest &request,
                        const MqlTradeResult &result){



}

//| TesterInit function
void OnTesterInit(){
}

//+------------------------------------------------------------------+
void OnTesterDeinit(){
}
