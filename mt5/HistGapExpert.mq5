#property copyright "Andre L. Ferreira"
#property version   "1.01"
#include <Arrays\ArrayDouble.mqh>
#include <Trade\Trade.mqh>
//#include <Expert\Trailing\TrailingParabolicSAR.mqh>
#include "NaiveGapDefinitions.mqh"

MqlDateTime previousday;

int previous_positions; // number of openned positions

//| Expert initialization function
int OnInit(){

    EventSetTimer(1);
   // trailing.Maximum(0.02);
    positionExpireTime = (int) (expertPositionExpireHours*3600); // hours to seconds
    tickSize = SymbolInfoDouble(Symbol(), SYMBOL_TRADE_TICK_SIZE);
    trade.SetExpertMagicNumber(EXPERT_MAGIC);
    trade.SetDeviationInPoints(orderDeviation*tickSize);
    //--- what function is to be used for trading: true - OrderSendAsync(), false - OrderSend()
    trade.SetAsyncMode(true);

    // start day is the previous day
    TimeCurrent(previousday);
    previous_positions = PositionsTotal(); // number of openned positions

    hema=iMA(Symbol(), PERIOD_M1, trailingEmaWindow, 0, MODE_EMA, PRICE_TYPICAL);

    if(hema == INVALID_HANDLE){
       printf("Error creating EMAindicator");
       return(INIT_FAILED);
    }
    Print("Begining Gap Expert now: ", TimeCurrent());
    return(INIT_SUCCEEDED);
}

void PlaceLimitOrder(double entry, double sl, double tp, int sign){ // sign > 0 buy sell otherwise
    bool result = false;
    entry = roundTickSize(entry); // ask price
    tp = roundTickSize(tp);
    sl = roundTickSize(sl);

    if(sign > 0)
        result = trade.BuyLimit(orderSize, entry, Symbol(), sl, tp);
    else
        result = trade.SellLimit(orderSize, entry, Symbol(), sl, tp);

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

    CopyClose(Symbol(), PERIOD_M1, expertUseCurrentDay, expertnDaysPivots*9*60, closes); // days of 9 hours
    pivotsHistSMA(closes, tickSize, pivots); // works well when market is dancing up and down

    size = ArraySize(pivots);
    // // stop loss based on standard orderDeviation of last 5 days on H4 time-frame
    // stop = stopStdev();
    // if(stop == 0)
    //   stop = 1000;
    double bid, ask;
    bid = SymbolInfoDouble(Symbol(),SYMBOL_BID);
    ask = SymbolInfoDouble(Symbol(),SYMBOL_ASK);
    tp = roundTickSize(tp);

    //+ postive buy order
    if(sign > 0){

        //Place Buy Limit Orders
        // search all supports bellow the asking price
        // array is ascending sorted so first element greater or equal ask price defines the ceil (boundary) of the last support
        // all rest bellow are also supports for this ask price are before this position one element
        pos = searchGreatEqual(pivots, ask) - 1; // position of first pivot bigger than ask price
        if(pos <  0){
             Print("Not enough pivots to place limit orders ");
             return; // this is an erroor something is wrong or or there is no pivots
        } // +1 due zero based index
        if(pos+1 < expertEntries){ // less entries than desired
            Print("Will not place all limit orders ");
        }

        // everything before are supports
        sl = pivots[MathMax(0, pos-expertEntries)]; // and the first support is the SL
        sl = roundTickSize(sl);
        // for every support bellow ask price (except first) put a buy limit order
        // with the same target
        for(int i=MathMax(0, pos-expertEntries); i<=pos; i++) // 3-2=1 <= 3
            PlaceLimitOrder(pivots[i], sl, tp, sign);

        // Finally place the first buy order Now! Not good
        //result = trade.Buy(orderSize, Symbol(), ask,sl, tp);
    }
    else{
        // this seams wrong giving 3 orders
        pos = searchGreat(pivots, bid); // position of first pivot bigger than bid price
        // so everything above including this are resistences
        if(pos <  0){
             Print("Not enough pivots to place limit orders ");
             return; // this is an erroor something is wrong or or there are no pivots
        }
        if( MathAbs(size-pos) < expertEntries) {
            Print("Will not place all limit orders ");
        }
        // everything including this above are resistences
        sl = pivots[MathMin(size-1, pos+expertEntries)]; // and the last resistence is the SL
        // for every resistence above the bid price (except last) put a sell limit order
        // with the same target
        sl = roundTickSize(sl);

        for(int i=pos; i<MathMin(size, pos+expertEntries); i++)
            PlaceLimitOrder(pivots[i], sl, tp, sign);

        // Finally place the first  sell Now!
        //result = trade.Sell(orderSize, Symbol(), bid, sl, tp);
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

    copied = CopyRates(Symbol(), PERIOD_M5, 0, 1, ratesnow);
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

        copied = CopyRates(Symbol(), PERIOD_D1, 1, 1, pday);
        if( copied == -1){
            Print("Failed to get previous day data");
            return;
        }

        previousday = todaynow;  // just entered a position today
        gap = ratesnow[0].open - pday[0].close;
        // positive gap will close so it is a sell order
        // negative gap will close si it is a by order
        if (MathAbs(gap) < maximumGapSize && MathAbs(gap) >  minimumGapSize ){ // Go in
            gapsign = gap/MathAbs(gap);

            // place take profit X discount points before gap close
            gap_tp = pday[0].close+gapsign*discountGap;
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
