#property copyright "Andre L. Ferreira"
#property version   "1.01"
#include <Arrays\ArrayDouble.mqh>
#include <Trade\Trade.mqh>
#include "NaiveGapDefinitions.mqh"

// 5 days simple moving average window for stdev calculation on H4 time frame
const int  window=5*2;
int hstdev;
MqlDateTime previousday;

int previous_positions; // number of openned positions

//| Expert initialization function
int OnInit(){

   EventSetTimer(5);

   trade.SetExpertMagicNumber(EXPERT_MAGIC);
   trade.SetDeviationInPoints(deviation*ticksize);
   //--- what function is to be used for trading: true - OrderSendAsync(), false - OrderSend()
   trade.SetAsyncMode(true);

   hstdev = iStdDev(sname, PERIOD_H4, window, 0, MODE_SMA, PRICE_CLOSE);

   if(hstdev == INVALID_HANDLE){
      Print("Gap Expert : Error creating indicator");
      return(INIT_FAILED);
   }

   // start day is the previous day
   TimeCurrent(previousday);
   previous_positions = PositionsTotal(); // number of openned positions

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
    bool result = false;
    entry = MathFloor(entry/ticksize)*ticksize;; // ask price
    tp = MathFloor(tp/ticksize)*ticksize;
    sl = MathFloor(sl/ticksize)*ticksize;

    if(sign > 0)
        result = trade.BuyLimit(quantity, entry, sname, sl, tp);
    else
        result = trade.SellLimit(quantity, entry, sname, sl, tp);

    if(!result)
          Print("Buy()/Sell() Limit method failed. Return code=",trade.ResultRetcode(),
                ". Code description: ",trade.ResultRetcodeDescription());
    else
         Print("Buy()/Sell() Limit method executed successfully. Return code=",trade.ResultRetcode(),
                " (",trade.ResultRetcodeDescription(),")");
}

void PlaceOrders(int sign, double tp){
    int pos, size;
    MqlRates rates[];
    double pivots[];
    double sl;
    bool result;

    CopyRates(sname, PERIOD_D1, 0, 5, rates); // 5 last days
    pivotPoints(rates, pivots);
    size = ArraySize(pivots);
    // // stop loss based on standard deviation of last 5 days on H4 time-frame
    // stop = stopStdev();
    // if(stop == 0)
    //   stop = 1000;
    double bid, ask;
    bid = SymbolInfoDouble(sname,SYMBOL_BID);
    ask = SymbolInfoDouble(sname,SYMBOL_ASK);
    tp = MathFloor(tp/ticksize)*ticksize;

    //+ postive buy order
    if(sign > 0){

      //Place Buy Limit Orders
      // search all supports bellow the asking price
      // array is ascending sorted so first element greater
      // them the  ask price defines the boundary of the supports
      // all supports for this ask price are before this position one element
      pos = searchGreat(pivots, ask); // position of first pivot bigger than ask price
      if(pos == -1)
        return; // this is an error somthing is wrong
        //pos = ArraySize(pivots);
      // everything before are supports
      sl = pivots[MathMax(0, pos-3)]; // and the first support is the SL
     // sl = ask-500;
      sl = MathFloor(sl/ticksize)*ticksize;
      // for every support bellow ask price (except first) put a buy limit order
      // with the same target
      for(int i=MathMax(0, pos-2); i<pos; i+=1)
        PlaceLimitOrder(pivots[i], sl, tp, sign);

      // Finally place the first buy order Now!
      //result = trade.Buy(quantity, sname, ask,sl, tp);
    }
    else{

      pos = searchGreat(pivots, bid); // position of first pivot bigger than bid price
      // so everything above including this are resistences
      if(pos == -1)
        return; // something is wrong

      // everything including this above are resistences
      sl = pivots[MathMin(size-1, pos+3)]; // and the last resistence is the SL
      //sl = bid+500;
      // for every resistence above the bid price (except last) put a sell limit order
      // with the same target
      sl = MathFloor(sl/ticksize)*ticksize;

      for(int i=pos; i<MathMin(size, pos+2); i+=1)
        PlaceLimitOrder(pivots[i], sl, tp, sign);

        // Finally place the first  sell Now!
       // result = trade.Sell(quantity, sname, bid, sl, tp);
    }

    if(!result)
          Print("Buy()/Sell() method failed. Return code=",trade.ResultRetcode(),
                ". Code description: ",trade.ResultRetcodeDescription());
    else
         Print("Buy()/Sell() method executed successfully. Return code=",trade.ResultRetcode(),
                " (",trade.ResultRetcodeDescription(),")");
}


double gap_tp;  // gap take profit
bool on_gap=false;

//| Timer function -- Every 5 minutes
void OnTimer() {
    double pdayclose[2];
    MqlRates ratesnow[1]; // previous day close and open today
    MqlDateTime todaynow;
    int copied, gapsign;
    double gap;

    TimeCurrent(todaynow);

    copied = CopyRates(sname, PERIOD_M5, 0, 1, ratesnow);
    if( copied == -1){
      Print("Failed to get today data");
      return;
    }

    // check if gap take profit was reached if so close all pending orders
    if(gap_tp >= ratesnow[0].low && gap_tp <= ratesnow[0].high && on_gap){
        CloseOpenOrders();
        on_gap = false;
    }

    if(todaynow.day != previousday.day){

      copied = CopyClose(sname, PERIOD_D1, 0, 2, pdayclose);
      if( copied == -1){
        Print("Failed to get previous day data");
        return;
      }

      previousday = todaynow;  // just entered a position today

      gap = ratesnow[0].open - pdayclose[0];
      // positive gap will close so it is a sell order
      // negative gap will close si it is a by order
      if (MathAbs(gap) < 320 && MathAbs(gap) > 80){ // Go in
        gapsign = gap/MathAbs(gap);

        // place take profit 15 points before gap close
        gap_tp = pdayclose[0]+gapsign*15;
        PlaceOrders(-gapsign, gap_tp);
      }
      on_gap = true;
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



}

//| TradeTransaction function                                        |
void OnTradeTransaction(const MqlTradeTransaction &trans,
                        const MqlTradeRequest &request,
                        const MqlTradeResult &result){
                        if ( trans.type == TRADE_TRANSACTION_DEAL_ADD){
                            // just closed a position by time, stop or whatever
                            if(PositionsTotal() == 0 && previous_positions == 1){
                                CloseOpenOrders(); // close all pending limit orders
                                previous_positions = 0;
                                on_gap = false;
                            }
                                previous_positions = PositionsTotal();
                        }
}

//| TesterInit function
void OnTesterInit(){
}

//+------------------------------------------------------------------+
void OnTesterDeinit(){
}
