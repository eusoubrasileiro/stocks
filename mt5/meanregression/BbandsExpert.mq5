#property copyright "Andre L. Ferreira"
#property version   "2.01"

#include "..\TrailingMA.mqh"
#include "..\Util.mqh"
#include <Expert\Money\MoneyNone.mqh>

//Inputs
// that's applied on take profit discountGap*ticksize'
input double orderSize = 100e3; // each order Size in $$ for stocks or future contracts
// expert operations end (no further sells or buys) close all positions
input double expertDayEndHour = 15.5; // Operational window maximum day hour
// current day not completed ohlc will not be equal
input int expertnOdersperDay = 100; // Number of orders placed per day
input double expertPositionExpireHours = 1.5; // Time to expire a position (close it)
input int  trailingEmaWindow = 5; //  EMA Trailing Stop Window in M1
input double stoploss = 60; // stop loss for each order
input double targetprofit = 15; // target profit per order

bool                     Expert_EveryTick       = false;
int                      Expert_MagicNumber     = 2525;
int                      Expert_NBands          = 3; //number of bollinger bands
int                      Expert_Window          = 21; // indicators buffer needed
// number of samples needed/used for training 5*days?
const int                Expert_BufferSize      = 60*7*5; // indicators buffer needed


class CExpertXBands : public CExpertX
{
    int m_nbands; // number of bands
    int m_window; //
    int m_signals[][Expert_BufferSize]; // store buy|sell|hold signals for each bband
    // 1|2|0 = buy|sell|hold
    CIndicators *m_bands; // collection of bbands indicators

  public:
                     CExpertXBands(void){
                        m_bands = new CIndicators;
                        // use all those series
                        m_used_series = USE_SERIES_OPEN|USE_SERIES_CLOSE|USE_SERIES_HIGH|USE_SERIES_LOW;
                     };
                    ~CExpertXBands(void){};

  //CIndicators m_indicators;

  void CreateBBands(int nbands, int window){
    // Call only after init indicators
    m_nbands = nbands;
    m_window = window;
    m_buffersize = buffersize;
    ArrayResize(m_signals, m_nbands);

    // call only after refresh / resize all internal series buffers to BufferSize
    m_open.BufferSize(Expert_BufferSize);
    m_close.BufferSize(Expert_BufferSize);
    m_high.BufferSize(Expert_BufferSize);
    m_low.BufferSize(Expert_BufferSize);

    double inc=0.5;
    for(int i=0; i<nbands; i++){ // create multiple increasing bollinger bands
        CiBands band = new CiBands;
        band.Create(m_symbol.Name(), PERIOD_M1, window*inc, 0, 2.5, PRICE_TYPICAL);
        band.BufferResize(buffersize);
        m_indicators.Add(band); // needed to be refreshed by CExpert
        m_bands.Add(band);
        inc += 0.5;
    }
  }

  void calculateRawSignals(){
    for(int i=0; i<m_nbands; i++)
       for(int j=0;j<Expert_BufferSize;j++){
        //    Based on a bollinger band defined by upper-band and lower-band
        //    return signal:
        //        buy 1 : crossing down-outside it's buy
        //        sell 2 : crossing up-outside it's sell
        //        hold : nothing usefull happend
        // remember order is reversed yonger firs older later
        if( m_high.At(j) >= m_bands.At(i).Upper(j))
            m_signals[i][j] = 2; // sell
        else 
        if( m_low.At(j) <= m_bands.At(i).Lower(j))
            m_signals[i][j] = 1; // buy
        else
            m_signals[i][j] = 0; // nothing
       }
    }
  }

  protected:



};

// Create and add MA indicator to the collection of indicator internal call
bool CTrailingMA::InitIndicators(CIndicators *indicators)
{
//--- check
 if(indicators==NULL)
    return(false);
//--- create MA indicator
 if(m_MA==NULL)
    if((m_MA=new CiMA)==NULL)
      {
       printf(__FUNCTION__+": error creating object CTrailingMA indicator");
       return(false);
      }
//--- add MA indicator to the internal CExertBase collection
 if(!indicators.Add(m_MA))
   {
    printf(__FUNCTION__+": error adding object CTrailingMA indicator");
    delete m_MA;
    return(false);
   }
//--- initialize MA indicator (no shift!!)
 if(!m_MA.Create(m_symbol.Name(), m_period, m_ma_period, 0, m_ma_method, m_ma_applied))
   {
    printf(__FUNCTION__+": error initializing object");
    return(false);
   }
 m_MA.BufferResize(2); //  just need 2 samples, now and before

 return(true);
}



//| Expert initialization function
int OnInit(){

    EventSetTimer(60); // in seconds - 1 Minute

    //--- Initializing expert
    if(!Expertx.Init(Symbol(), PERIOD_M1, Expert_EveryTick, Expert_MagicNumber)
          || !Expertx.setPublic()) // initialize "gambiarra" CTrade public variable
      return(-1);

    Expertx.setDayTradeParams(expertPositionExpireHours, expertDayEndHour);

    CTrailingMA *trailing = new CTrailingMA(MODE_EMA, PRICE_CLOSE, trailingEmaWindow);
    if(trailing==NULL)
       return(-2);

    if(!Expertx.InitTrailing(trailing))
      return(-3);

    if(!trailing.ValidationSettings())
       return(-4);

    CMoneyNone *money=new CMoneyNone;
    if(!Expertx.InitMoney(money))
       return(-6);

    if(!Expertx.InitIndicators())
       return(-5);



    // ExtExpert.trader.SetDeviationInPoints(orderDeviation*tickSize);
    //--- what function is to be used for trading: true - OrderSendAsync(), false - OrderSend()
    // ExtExpert.trader.SetAsyncMode(true);
    // start day is the previous day
    TimeCurrent(previousday);
    previous_positions = PositionsTotal(); // number of openned positions
    Print("Begining Gap Expert now: ", TimeCurrent());

    return(INIT_SUCCEEDED);
}

void PlaceLimitOrder(double entry, double sl, double tp, int sign, int amount){ // sign > 0 buy sell otherwise
    bool result = false;
    entry = Expertx.symbol.NormalizePrice(entry); // ask price
    tp = Expertx.symbol.NormalizePrice(tp);
    sl = Expertx.symbol.NormalizePrice(sl); // CTrade.BuyLimit doesnt normalize Price to tickSize/tickValue

    double bid, ask;
    bid = SymbolInfoDouble(Symbol(), SYMBOL_BID);
    ask = SymbolInfoDouble(Symbol(), SYMBOL_ASK);

    if(sign > 0){
        // use ask price to calculate max ammount
        // maxAmount = int(orderSize/ask);
        // convert to 100's
        result = Expertx.trader.BuyLimit(Expertx.roundVolume(int(orderSize/ask)*amount),
                    entry, Symbol(), sl, tp);
    }
    else{
        // use bid price to calculate order size
        result = Expertx.trader.SellLimit(Expertx.roundVolume(int(orderSize/ask)*amount),
                    entry, Symbol(), sl, tp);
    }

    if(!result)
        Print("Buy()/Sell() Limit method failed. Return code=",Expertx.trader.ResultRetcode(),
        ". Code description: ",Expertx.trader.ResultRetcodeDescription());
    else
        Print("Buy()/Sell() Limit method executed successfully. Return code=",Expertx.trader.ResultRetcode(),
        " (",Expertx.trader.ResultRetcodeDescription(),")");
}

void PlaceOrders(int sign, double tp){
    int npivots, nuse_pivots; // number of pivots calculated
    // and efective usefull number used  of pivots bellow or above the ask/bid price
    MqlRates rates[];
    double pivots[];
    double sl, rwr_ratio;
    int iorders; // counter of orders being placed
    int i, start_usepivots, end_usepivots, nunique_pivots;
    CopyRates(Symbol(), PERIOD_D1, 1, expertnDaysPivots, rates);

    switch(typePivots){
        case 1: // classic
        npivots = classic_pivotPoints(rates, pivots);
        break;
        case 2: // camarilla
        npivots = camarilla_pivotPoints(rates, pivots);
        break;
        default:
        case 3: // fibo
        npivots = fibonacci_pivotPoints(rates, pivots);
        break;
    }

    double bid, ask;
    bid = SymbolInfoDouble(Symbol(),SYMBOL_BID);
    ask = SymbolInfoDouble(Symbol(),SYMBOL_ASK);

    for(i=0; i<npivots; i++) // better bins on unique
        pivots[i] = Expertx.symbol.NormalizePrice(pivots[i]);

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


void OnTimer() {
    MqlRates ratesnow[1], pday[1]; // previous day close and open today
    MqlDateTime todaynow;
    int copied, gapsign;
    double gap;

    //--- updated quotes and indicators
    if(!Expertx.Refresh())
        return; // no work without correct data

    TimeCurrent(todaynow);

    copied = CopyRates(Symbol(), PERIOD_M5, 0, 1, ratesnow);
    if( copied == -1){
        Print("Failed to get today data");
        return;
    }

    // check if gap take profit was reached if so close all pending orders
    if(gap_tp >= ratesnow[0].low && gap_tp <= ratesnow[0].high && on_gap){
        Expertx.DeleteOrders();
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
    if(Expertx.SelectPosition()){ // if any open position
        Expertx.CloseOpenPositionsbyTime();
        Expertx.CheckTrailingStop();
    }
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
                Expertx.DeleteOrders();
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
