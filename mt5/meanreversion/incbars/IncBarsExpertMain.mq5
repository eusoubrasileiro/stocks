#property copyright "Andre L. Ferreira"
#property version   "1.01"

#include "..\..\TrailingMA.mqh"
#include "..\..\Util.mqh"
#include "IncBars.mqh"
#include <Expert\Money\MoneyNone.mqh>

//Inputs
//number of bollinger bands
input int                      Expert_NBands          = 3;
// reference window for bbands
input int                      Expert_Window          = 21;
// "memory" of patterns for training sklearn model
input int                      Expert_Batch_Size      = 5;
// minimum number of samples for training
input int                      Expert_NTraining       = 100;
// orderSize in $$$
input double                   Expert_OrderSize       = 25e3;
// stop loss for each order $$$
input double                   Expert_StopLoss        = 100;
// target profit per order $$$
input double                   Expert_TargetProfit    = 15;
input int                      Expert_MaxPositions    = 3; // max open positions Expert_OrderSize

const bool                     Expert_EveryTick       = true;
const int                      Expert_MagicNumber     = 2525;


// current day not completed ohlc will not be equal
const int Expert_OrdersPerDay = 100; // Number of orders placed per day
const int Expert_TrailingEma = 5; //  EMA Trailing Stop Window in M1

CExpertIncBars cExpert = new CExpertIncBars;

//| Expert initialization function
int OnInit(){

    // Use the timer to get missing ticks every 1 second
    // will be more than enough this is not HFT! remember that!
    EventSetTimer(30); // in seconds

    //--- Initializing expert
    if(!cExpert.Init(Symbol(), PERIOD_M1, Expert_EveryTick, Expert_MagicNumber))
      return(-1);

//
//   no trailing for while
//
//    CTrailingMA *trailing = new CTrailingMA(MODE_EMA, PRICE_CLOSE, Expert_TrailingEma);
//    if(trailing==NULL)
//       return(-2);
//
//    if(!cExpert.InitTrailing(trailing))
//      return(-3);

//    if(!trailing.ValidationSettings())
//      return(-4);

    CMoneyNone *money=new CMoneyNone;
    if(!cExpert.InitMoney(money))
       return(-6);

    if(!cExpert.InitIndicators())
       return(-5);       
      
    cExpert.Initialize(Expert_NBands, Expert_Window, Expert_Batch_Size,
                       Expert_NTraining, Expert_OrderSize,
                       Expert_StopLoss, Expert_TargetProfit, Expert_MaxPositions);

    return(INIT_SUCCEEDED);
}


void OnTimer() {
    cExpert.CheckTicks();
}

// Useless stuff
//| Expert deinitialization function                                 |
void OnDeinit(const int reason){
    EventKillTimer();
    cExpert.Deinit();
}

//| Expert tick function
//void OnTick(){
//cExpert.CheckTicks();
//}

//| Trade function                                                   |
void OnTrade(){
    cExpert.OnTrade();
}

//| TradeTransaction function                                       |

//void OnTradeTransaction(const MqlTradeTransaction &trans,
//    const MqlTradeRequest &request,
//    const MqlTradeResult &result){
//
//}
