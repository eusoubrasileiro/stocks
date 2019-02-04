#property copyright "Andre L. Ferreira"
#property version   "1.01"
#include "NaiveDefinitions.mqh"
#include "Util.mqh"

//TrailingStopEma tsop;
const int  window=3; // 3 minutes

//| Expert initialization function
int OnInit(){

   EventSetTimer(10);
   hstdevh = iStdDev(sname, PERIOD_M1, window, 0, MODE_SMA, PRICE_HIGH);
   hstdevl = iStdDev(sname, PERIOD_M1, window, 0, MODE_SMA, PRICE_LOW);
   hemah = iMA(sname, PERIOD_M1, window, 0, MODE_EMA, PRICE_HIGH);
   hemal = iMA(sname, PERIOD_M1, window, 0, MODE_EMA, PRICE_LOW);

   if(hstdevh == INVALID_HANDLE ||
      hstdevl == INVALID_HANDLE ||
      hemah == INVALID_HANDLE ||
      hemal == INVALID_HANDLE){
      Print("Naive Expert : Error creating indicator");
      return(INIT_FAILED);
   }

   //tsop = TrailingStopEma();

   // time in seconds from 1970 current time
   Print("Begining Naive Expert now: ", TimeCurrent());
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

bool isBuyPattern(){ // is up trend
    double stdh[5];
    double stdl[5];
    double emah[6];
    double emal[6];
    bool isbuypattern = true;

    if (CopyBuffer(hstdevh, 0, 0, 5, stdh) != 5 ||
        CopyBuffer(hstdevl, 0, 0, 5, stdl) != 5 ||
        CopyBuffer(hemah, 0, 0, 6, emah) != 6 || // 5+1 due diff
        CopyBuffer(hemal, 0, 0, 6, emal) != 6 
        ){
       Print("CopyBuffer from isPattern failed");
       return false;
    }
    for(int i=1; i<6; i++)
        if(emah[i]-emah[i-1] <= 0 || emal[i]-emal[i-1] <=0 ||
           stdh[i-1] <= 15 || stdh[i-1] >= 90 ||
           stdl[i-1] <= 15 || stdl[i-1] >= 90 ){
               isbuypattern = false;
               break;
           }
    return isbuypattern;
}

void BuyNow(){
    MqlTradeRequest request={0};
    MqlTradeResult result={0};
    int ncontracts = 1;
    double stop = 90;

    //--- parameters of request
    request.action=TRADE_ACTION_DEAL;      // type of trade operation
    request.symbol=sname;                               // symbol
    //+ postive buy order
    request.price=SymbolInfoDouble(request.symbol,SYMBOL_ASK); // price for opening
    request.type=ORDER_TYPE_BUY;                        // order type
    // stop loss and take profit 1.5:1 rount to 5
    request.tp =request.price+stop*3;
    request.tp = MathFloor(request.tp/ticksize)*ticksize;
    request.sl = request.price-stop*3;
    request.sl = MathCeil(request.sl/ticksize)*ticksize;
    request.volume=quantity*ncontracts; // volume executed in contracts
    request.deviation=deviation*ticksize;    //  allowed deviation from the price
    request.magic=EXPERT_MAGIC;   // MagicNumber for this Expert
    if(!OrderSend(request,result))
        Print("BuyNow OrderSend error ",GetLastError());
    //--- information about the operation
    Print("retcode ",result.retcode,"  deal ",result.deal);
    //--- output information about the closure by opposite position
}


//| Timer function -- Every 1 minutes
void OnTimer() {
    datetime timenow = TimeCurrent();
    datetime dayend=dayEnd(timenow); // 15 minutes before closing the stock market
    datetime daybegin=dayBegin(timenow); // 2 hours after openning

    //trailingStopLossEma();
    // check to see if we should close any order
    ClosePositionbyTime();
    // we can work
    if(timenow > dayend || timenow < daybegin ||
       nlastDeals() > dtndeals || ndealsDay() > maxdealsday)
      return;
       // deals are only ENTRY_IN deals that means entering a position
       // do not place orders in the end of the day
       // do not place orders in the begin of the day
       // cannot make more than`dtndeals` deals per dt
       // dont open more than `maxdealsday` positions per day
    if(isBuyPattern() && PositionsTotal() ==0)
        BuyNow();
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
