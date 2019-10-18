#property copyright "Andre L. Ferreira"
#property version   "1.01"

#include "..\..\TrailingMA.mqh"
#include "..\..\Util.mqh"
#include "CExpertBands.mqh"
#include <Expert\Money\MoneyNone.mqh>

//Inputs
//number of bollinger bands
input int                      Expert_NBands          = 6;
// reference window for bbands
input int                      Expert_Window          = 21;
// "memory" of patterns for training sklearn model
input int                  Expert_Batch_Size          = 15;
// minimum number of samples for training
input int                   Expert_NTraining          = 60;
// orderSize in $$$
input double                Expert_OrderSize          = 25e3;
// stop loss for each order $$$
input double                Expert_Train_StopLoss     = 100;
// target profit per order $$$
input double              Expert_Train_TargetProfit   = 15;
// stop loss for each order $$$
input double              Expert_Run_TargetProfit     = 15;
// target profit per order $$$
input double              Expert_Run_StopLoss        = 15;
// recursive == kalman filter option
input bool                Expert_Recursive           = true;


const bool                     Expert_EveryTick       = false;
const int                      Expert_MagicNumber     = 2525;
// expert operations end (no further sells or buys) close all positions
const double Expert_DayEndHour = 15.5; // Operational window maximum day hour
// current day not completed ohlc will not be equal
const int Expert_OrdersPerDay = 100; // Number of orders placed per day
const double Expert_PositionExpireHours = 1.5; // Time to expire a position (close it)
const int Expert_TrailingEma = 5; //  EMA Trailing Stop Window in M1

CExpertBands cExpert = new CExpertBands;

//| Expert initialization function
int OnInit(){

    EventSetTimer(60); // in seconds - 1 Minute

    //--- Initializing expert
    if(!cExpert.Init(Symbol(), PERIOD_M1, Expert_EveryTick, Expert_MagicNumber)
          || !cExpert.setPublic()) // initialize "gambiarra" CTrade public variable
      return(-1);

    cExpert.setDayTradeParams(Expert_PositionExpireHours, Expert_DayEndHour);

    CTrailingMA *trailing = new CTrailingMA(MODE_EMA, PRICE_CLOSE, Expert_TrailingEma);
    if(trailing==NULL)
       return(-2);

    if(!cExpert.InitTrailing(trailing))
      return(-3);

    if(!trailing.ValidationSettings())
       return(-4);

    CMoneyNone *money=new CMoneyNone;
    if(!cExpert.InitMoney(money))
       return(-6);

    if(!cExpert.InitIndicators())
       return(-5);

    cExpert.Initialize(Expert_NBands, Expert_Window, Expert_Batch_Size,
                        Expert_NTraining, Expert_OrderSize,
                          Expert_Train_StopLoss, Expert_Train_TargetProfit,
                          Expert_Run_StopLoss, Expert_Run_TargetProfit,
                          Expert_Recursive);

    return(INIT_SUCCEEDED);
}


void OnTimer() {
    cExpert.OnTimer();
}

// Useless stuff
//| Expert deinitialization function                                 |
void OnDeinit(const int reason){
    EventKillTimer();
}

//| Expert tick function     
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
