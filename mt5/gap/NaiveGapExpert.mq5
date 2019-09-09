#property copyright "Andre L. Ferreira"
#property version   "1.01"
#include <Arrays\ArrayDouble.mqh>
#include <Trade\Trade.mqh>
#include "NaiveGapDefinitions.mqh"

MqlDateTime previousday;

int previous_positions; // number of openned positions

//| Expert initialization function
int OnInit(){

    EventSetTimer(1);
   // trailing.Maximum(0.02);
    positionExpireTime = (int) (expertPositionExpireHours*3600); // hours to seconds
    tickSize = SymbolInfoDouble(Symbol(), SYMBOL_TRADE_TICK_SIZE);
    tickValue = SymbolInfoDouble(Symbol(), SYMBOL_TRADE_TICK_VALUE);
    minVolume = SymbolInfoDouble(Symbol(), SYMBOL_VOLUME_MIN); // minimal volume for a deal
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

void PlaceLimitOrder(double entry, double sl, double tp, int sign, int amount){ // sign > 0 buy sell otherwise
    bool result = false;
    entry = roundTickSize(entry); // ask price
    tp = roundTickSize(tp);
    sl = roundTickSize(sl);

    double bid, ask;
    bid = SymbolInfoDouble(Symbol(), SYMBOL_BID);
    ask = SymbolInfoDouble(Symbol(), SYMBOL_ASK);
    
    if(sign > 0){        
        // use ask price to calculate max ammount
        // maxAmount = int(orderSize/ask);
        // convert to 100's
        result = trade.BuyLimit(roundVolume(int(orderSize/ask)*amount), 
                    entry, Symbol(), sl, tp);
    }
    else{
        // use bid price to calculate order size
        result = trade.SellLimit(roundVolume(int(orderSize/ask)*amount), 
                    entry, Symbol(), sl, tp);
    }

    if(!result)
        Print("Buy()/Sell() Limit method failed. Return code=",trade.ResultRetcode(),
        ". Code description: ",trade.ResultRetcodeDescription());
    else
        Print("Buy()/Sell() Limit method executed successfully. Return code=",trade.ResultRetcode(),
        " (",trade.ResultRetcodeDescription(),")");
}

void PlaceOrders(int sign, double tp){
    int npivots, nuse_pivots; // number of pivots calculated
    // and efective usefull number used  of pivots bellow or above the ask/bid price
    MqlRates rates[];
    double pivots[];
    double sl, rwr_ratio;
    int iorders; // counter of orders being placed
    int i, start_usepivots, end_usepivots, nunique_pivots;

    switch(typePivots){
        case 1: // classic
        CopyRates(Symbol(), PERIOD_D1, 1, expertnDaysPivots, rates);
        npivots = classic_pivotPoints(rates, pivots);
        break;
        case 2: // camarilla
        CopyRates(Symbol(), PERIOD_D1, 1, expertnDaysPivots, rates);
        npivots = camarilla_pivotPoints(rates, pivots);
        break;
        default:
        case 3: // fibo
        CopyRates(Symbol(), PERIOD_D1, 1, expertnDaysPivots, rates);
        npivots = fibonacci_pivotPoints(rates, pivots);
        break;
    }

    double bid, ask;
    bid = SymbolInfoDouble(Symbol(),SYMBOL_BID);
    ask = SymbolInfoDouble(Symbol(),SYMBOL_ASK);
    tp = roundTickSize(tp);

    // there are repeated pivots due multiple days
    int count_unique_pivots[]; // frequency count like a histogram but for unique values
    double unique_pivots[]; // unique pivots price values
    nunique_pivots = naivehistPriceTicksize(pivots, unique_pivots, count_unique_pivots);
    //+ postive buy order
    if(sign > 0){ // gonna go up
        //Place Buy Limit Orders
        // search all supports bellow the asking price
        int ifirst_support = searchLess(unique_pivots, ask); // position of first support
        // so all bellow are also supports for this ask price - array is ascending sorted
        if(ifirst_support <  1){ // one isnt enough at least 2 need due stop-loss
             Print("Not enough pivots to place limit orders with a stop");
             return; // this is an erroor something is wrong or or there is no pivots
        }
        // first useful pivot index (of stop-loss)
        // expertnOdersperDay can be bigger than number of pivots so to make sure
        // nothing goes wrong
        start_usepivots = MathMax(0, ifirst_support-expertnOdersperDay);
        end_usepivots =  ifirst_support; // INCLUDED
        nuse_pivots = 1+end_usepivots-start_usepivots; // number of efective pivots
        // can be used to place orders
        if( nuse_pivots < expertnOdersperDay){ // less entries than desired
            Print("Will not place all limit orders ");
        }
        sl = unique_pivots[start_usepivots]; // and the first support is the SL
        sl = roundTickSize(sl);
        // calculate average price if all orders are placed? and stop-gain ratio
        //double avgprice = 0;
        //for(int i=MathMax(0, ifirst_support-expertnOdersperDay); i<=ifirst_support; i++)
        //    avgprice += pivots[i];
        //avgprice /= ifirst_support+1
        // for every support bellow ask price (except first) put a buy limit order
        // with the same target and sl
        // the size of the order will be defined by the histogram of counts
        // i starts with the orders closer to the tp otherwise they will they have preference
        for(iorders=expertnOdersperDay, i=end_usepivots;
            i>= start_usepivots+1  && iorders > 0;
            i--){ // the first is the stop-loss
            rwr_ratio = (tp-unique_pivots[i])/(unique_pivots[i]-sl);
            if( rwr_ratio < expertRewardRiskRatio) // cant be smaller than RewardRiskRatio
              continue;
            if(count_unique_pivots[i] >= iorders){ // there will be only one order
              PlaceLimitOrder(unique_pivots[i], sl, tp, sign, iorders);
              break;
            }
            else{
              PlaceLimitOrder(unique_pivots[i], sl, tp, sign, count_unique_pivots[i]);
              iorders -= count_unique_pivots[i];
            }
        }
        // Finally place the first  sell Now!
        //result = trade.Sell(orderSize, Symbol(), bid, sl, tp);
    }
    else{ // gonna go down
        int ifirst_resistance = searchGreat(unique_pivots, bid); // position of first pivot bigger than bid price
        // so everything above including this are resistences

        if(ifirst_resistance <  1){ // one isnt enough at least 2 need due stop-loss
             Print("Not enough pivots to place limit orders ");
             return; // this is an erroor something is wrong or or there are no pivots
        }
        // nothing goes wrong
        start_usepivots = ifirst_resistance;
        // last useful pivot index (of stop-loss)
        // expertnOdersperDay can be bigger than number of pivots avoid shit
        end_usepivots =  MathMin(nunique_pivots-1, ifirst_resistance+expertnOdersperDay); // INCLUDED
        nuse_pivots = 1+end_usepivots-start_usepivots; // number of useful pivots
        if( nuse_pivots < expertnOdersperDay) {
            Print("Will not place all limit orders ");
        }
        // everything including this above are resistences
        sl = unique_pivots[end_usepivots]; // and the last resistence is the SL
        sl = roundTickSize(sl);
        // for every resistence above the bid price (except last) put a sell limit order
        // with the same target and sl
        // the size of the order will be defined by the histogram of counts
        // i starts with the orders closer to the tp otherwise they have preference
        for(iorders=expertnOdersperDay, i=start_usepivots;
            i<= end_usepivots-1 && iorders > 0;  // the last is the stop loss
            i++){
            rwr_ratio = (unique_pivots[i]-tp)/(sl-unique_pivots[i]);
            if( rwr_ratio < expertRewardRiskRatio) // cant be smaller
                continue;
            if(count_unique_pivots[i] >= iorders){ // there will be only one order
              PlaceLimitOrder(unique_pivots[i], sl, tp, sign, iorders);
              break;
            }
            else{
              PlaceLimitOrder(unique_pivots[i], sl, tp, sign, count_unique_pivots[i]);
              iorders -= count_unique_pivots[i];
            }
        }
        // Finally place the first  sell Now!
        //result = trade.Sell(orderSize, Symbol(), bid, sl, tp);
    }

    // just for debugging
    Print("npivots : "+StringFormat("%d", npivots) + "  nunique_pivots : ", StringFormat("%d", nunique_pivots));
    for(i=0; i<nunique_pivots; i++)
        Print(i +" : "+StringFormat("%G - count %d", unique_pivots[i], count_unique_pivots[i]));

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
