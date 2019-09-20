#property copyright "Andre L. Ferreira"
#property version   "0.01"

#include "..\TrailingMA.mqh"
#include "..\Util.mqh"
#include "Bbands.mqh"
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


bool                     Expert_EveryTick       = false;
int                      Expert_MagicNumber     = 2525;
input int                      Expert_NBands          = 3; //number of bollinger bands
input int                      Expert_Window          = 21; // indicators buffer needed
input int                  Expert_Batch_Size          = 60; // "memory" of patterns for training sklearn model
input int                   Expert_NTraining          = 60; // minimum number of samples for training
input double                Expert_OrderSize          = 100e3;  // tick value * quantity bought in $$  
input double                Expert_StopLoss           = 60; // stop loss for each order
input double              Expert_TargetProfit         = 15; // target profit per order


CExpertXBands Expertx = new CExpertXBands;



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

    Expertx.Initialize(Expert_NBands, Expert_Window, Expert_Batch_Size,
                Expert_NTraining);

    return(INIT_SUCCEEDED);
}


void OnTimer() {
    MqlRates ratesnow[1], pday[1]; // previous day close and open today
    MqlDateTime todaynow;
    //--- updated quotes and indicators
    if(!Expertx.Refresh())
        return; // no work without correct data

    if(Expertx.isInsideDay()){


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
}

    //| TesterInit function
    void OnTesterInit(){
    }

    //+------------------------------------------------------------------+
    void OnTesterDeinit(){
    }
