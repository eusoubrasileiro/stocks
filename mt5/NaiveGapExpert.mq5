#property copyright "Andre L. Ferreira"
#property version   "1.01"
#include <Arrays\ArrayDouble.mqh>
#include "GapDefinitions.mqh"

// 5 days simple moving average window for stdev calculation on H4 time frame
const int  window=5*2;
int hstdev;
MqlDateTime previousday;
bool alreadyin = false; // // did not entry yet
int positions; // number of openned positions

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
   positions = PositionsTotal(); // number of openned positions

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

void PlaceLimitOrder(double entry, double sl, double tp, int sign){ // sign > 0 buy sell otherwise
    MqlTradeRequest request={0};
    MqlTradeResult result={0};

    request.action=TRADE_ACTION_PENDING;      // type of trade operation
    request.price = MathFloor(entry/ticksize)*ticksize;; // ask price
    request.tp = tp;
    request.tp = MathFloor(request.tp/ticksize)*ticksize;
    request.sl = sl;
    request.sl = MathFloor(request.sl/ticksize)*ticksize;

    if(sign > 0)
        request.type = ORDER_TYPE_BUY_LIMIT;
    else
        request.type = ORDER_TYPE_SELL_LIMIT;

    request.volume=quantity; // volume executed in contracts
    request.deviation=deviation*ticksize;    //  allowed deviation from the price
    request.magic=EXPERT_MAGIC;   // MagicNumber for this Expert

    if(!OrderSend(request,result))
        Print("Place Limit Order OrderSend error ",GetLastError());
    //--- information about the operation
    Print("retcode ",result.retcode,"  deal ", result.deal);

}

void PlaceOrders(int sign, double tp){
    MqlTradeRequest request={0};
    MqlTradeResult result={0};
    int pos;
    MqlRates rates[];
    double pivots[];
    CArrayDouble *cpivots = new CArrayDouble;
    double sl;

    CopyRates(sname, PERIOD_D1, 0, 5, rates); // 5 last days
    pivotPoints(rates, pivots);
    cpivots.AddArray(pivots);

    // // stop loss based on standard deviation of last 5 days on H4 time-frame
    // stop = stopStdev();
    // if(stop == 0)
    //   stop = 1000;

    //--- parameters of request
    request.action=TRADE_ACTION_DEAL;      // type of trade operation
    request.symbol=sname;                               // symbol

    request.tp = tp;
    request.tp = MathFloor(request.tp/ticksize)*ticksize;

    //+ postive buy order
    if(sign > 0){

      //Place Buy Limit Orders
      // search all supports bellow the asking price
      // array is ascending sorted so first element greater
      // them the  ask price defines the boundary of the supports
      // all supports for this ask price are before this position one element
      pos = cpivots.SearchGreat(SYMBOL_ASK)-1; // position of first pivot bigger than ask price
      // everything before are supports
      sl = pivots[0]; // and the first support is the SL
      // for every support bellow ask price (except first) put a buy limit order
      // with the same target
      for(int i=1; i<pos; i+=1)
        PlaceLimitOrders(pivots[i], sl, tp, sign);

      // Finally place the first buy order Now!
      request.price= SymbolInfoDouble(request.symbol, SYMBOL_ASK); // ask price
      request.type=ORDER_TYPE_BUY;
      //request.sl = request.price-stop;
    }
    else{

      pos = cpivots.SearchGreat(SYMBOL_BID)-1; // position of first pivot bigger than bid price
      // everything including this above are resistences
      sl = pivots[ArraySize(pivots)-1]; // and the last resistence is the SL
      // for every resistence above the bid price (except last) put a sell limit order
      // with the same target
      for(int i=pos; i<pos-1; i+=1)
        PlaceLimitOrders(pivots[i], sl, tp, sign);

        // Finally place the first  sell Now!
        request.price= SymbolInfoDouble(request.symbol, SYMBOL_BID); // bid price
        request.type=ORDER_TYPE_SELL;
    }

    request.sl = sl; // 1:3
    request.sl = MathFloor(request.sl/ticksize)*ticksize;
    request.volume=quantity; // volume executed in contracts
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
        PlaceOrders(-gapsign, pdayclose[0]+(gapsign*25));
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

    // just closed a position by time, stop or whatever
    if(PositionsTotal() == 0 && previous_positions == 1){
        CloseOpenOrders(); // close all pending limit orders
        previous_positions = 0;
    }
    else
        previous_positions = PositionsTotal();

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
