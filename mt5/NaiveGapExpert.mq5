#property copyright "Andre L. Ferreira"
#property version   "1.01"
#include <Arrays\ArrayDouble.mqh>
#include <Trade\Trade.mqh>
#include <Expert\Trailing\TrailingParabolicSAR.mqh>
#include "NaiveGapDefinitions.mqh"

MqlDateTime previousday;

int previous_positions; // number of openned positions
//CTrailingPSAR trailing;

//| Expert initialization function
int OnInit(){

    EventSetTimer(1);
   // trailing.Maximum(0.02);

    trade.SetExpertMagicNumber(EXPERT_MAGIC);
    trade.SetDeviationInPoints(deviation*ticksize);
    //--- what function is to be used for trading: true - OrderSendAsync(), false - OrderSend()
    trade.SetAsyncMode(true);

    // start day is the previous day
    TimeCurrent(previousday);
    previous_positions = PositionsTotal(); // number of openned positions

    hema=iMA(sname,PERIOD_M1, windowema, 0, MODE_EMA, PRICE_TYPICAL);

    if(hema == INVALID_HANDLE){
       printf("Error creating EMAindicator");
       return(INIT_FAILED);
    }
    // time in seconds from 1970 current time
    Print("Begining Gap Expert now: ", TimeCurrent());
    return(INIT_SUCCEEDED);
}

// double stopStdev(){
//     double stdev[1];
//     if (CopyBuffer(hstdev, 0, 0, 1, stdev) != 1){
//        Print("CopyBuffer from Stdev failed");
//        return 0;
//        }
//     return stdev[0]*2;
// }

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
    double closes[];
    double pivots[];
    double sl;
    bool result;

    CopyRates(sname, PERIOD_D1, 0, 6, rates);
    //CopyClose(sname, PERIOD_M1, 1, 21*9*60, closes); // 5 last days of 8 hours
    //classic_pivotPoints(rates, pivots);
     camarilla_pivotPoints(rates, pivots);
    //pivotsHistSMA(closes, ticksize, pivots); // works well when market is dancing up and down

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
        // array is ascending sorted so first element greaterw
        // them the  ask price defines the boundary of the supports
        // all supports for this ask price are before this position one element
        pos = searchGreat(pivots, ask) - 1; // position of first pivot bigger than ask price
        if(pos <  0){
             Print("Not enough pivots to place limit orders ");
             return; // this is an erroor something is wrong or or there is no pivots
        }
        if(pos+1 < nentry_pivots){ // less entries than desired
            Print("Will not place all limit orders ");
        }

        // everything before are supports
        sl = pivots[MathMax(0, pos-nentry_pivots)]; // and the first support is the SL
        sl = MathFloor(sl/ticksize)*ticksize;
        // for every support bellow ask price (except first) put a buy limit order
        // with the same target
        for(int i=MathMax(0, pos-nentry_pivots); i<=pos; i+=1) // 3-2=1 <= 3
            PlaceLimitOrder(pivots[i], sl, tp, sign);

        // Finally place the first buy order Now!
        //result = trade.Buy(quantity, sname, ask,sl, tp);
    }
    else{
        // this seams wrong just giving 3 orders 
        pos = searchGreat(pivots, bid); // position of first pivot bigger than bid price
        // so everything above including this are resistences
        if(pos <  0){
             Print("Not enough pivots to place limit orders ");
             return; // this is an erroor something is wrong or or there is no pivots
        }
        if( MathAbs(size-pos) < nentry_pivots) {
            Print("Will not place all limit orders ");
        }
        // everything including this above are resistences
        sl = pivots[MathMin(size-1, pos+nentry_pivots)]; // and the last resistence is the SL
        // for every resistence above the bid price (except last) put a sell limit order
        // with the same target
        sl = MathFloor(sl/ticksize)*ticksize;

        for(int i=pos; i<MathMin(size, pos+nentry_pivots); i+=1)
            PlaceLimitOrder(pivots[i], sl, tp, sign);

        // Finally place the first  sell Now!
        //result = trade.Sell(quantity, sname, bid, sl, tp);
    }

    //if(!result)
    //      Print("Buy()/Sell() method failed. Return code=",trade.ResultRetcode(),
    //            ". Code description: ",trade.ResultRetcodeDescription());
    //else
    //     Print("Buy()/Sell() method executed successfully. Return code=",trade.ResultRetcode(),
    //            " (",trade.ResultRetcodeDescription(),")");
}


double gap_tp;  // gap take profit
bool on_gap=false;

//| Timer function -- Every 5 minutes
void OnTimer() {
    MqlRates ratesnow[1], pday[1]; // previous day close and open today
    MqlDateTime todaynow;
    int copied, gapsign;
    double gap;

    TrailingStopLossEma();

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

        copied = CopyRates(sname, PERIOD_D1, 1, 1, pday);
        if( copied == -1){
            Print("Failed to get previous day data");
            return;
        }

        previousday = todaynow;  // just entered a position today
        gap = ratesnow[0].open - pday[0].close;
        // positive gap will close so it is a sell order
        // negative gap will close si it is a by order
        if (MathAbs(gap) < MaxGap && MathAbs(gap) >  MinGap ){ // Go in
            gapsign = gap/MathAbs(gap);

            // place take profit 15 points before gap close
            gap_tp = pday[0].close+gapsign*before_tp;
            PlaceOrders(-gapsign, gap_tp);
            on_gap = true;
        }

    }


    // in case did not reach the take profit close it by the day end
    // check to see if we should close any position
    ClosePositionbyTime();

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
