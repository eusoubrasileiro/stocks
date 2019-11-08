// Tests
// 1 - mqlticks creation
// 2 - money bars creation
// 3 - talib indicators for seconds/ms time-frame
// Should be run on strategy tester with
// - custom symbol PETR4-Test
//   - created from data\
// - entire history
// - every tick based on real tick


#include "..\TrailingMA.mqh"
#include "..\meanreversion\rbarbbands\CExpertRBarBBands.mqh"
#include "..\meanreversion\rbarbbands\RBarBBands.mqh"
#include "..\datastruct\Ticks.mqh"
#include <Expert\Money\MoneyNone.mqh>

//Inputs
//number of bollinger bands
const int                      Expert_NBands          = 6;
// reference window for bbands
const int                      Expert_Window          = 21;
// "memory" of patterns for training sklearn model
const int                  Expert_Batch_Size          = 15;
// minimum number of samples for training
const int                   Expert_NTraining          = 600;
// orderSize in $$$
const double                Expert_OrderSize          = 25e3;
// stop loss for each order $$$
const double                Expert_Train_StopLoss     = 100;
// target profit per order $$$
const double              Expert_Train_TargetProfit   = 15;
// stop loss for each order $$$
const double              Expert_Run_TargetProfit     = 15;
// target profit per order $$$
const double              Expert_Run_StopLoss        = 15;
// recursive == kalman filter option
const bool                Expert_Recursive           = false;

const bool                     Expert_EveryTick       = false;
const int                      Expert_MagicNumber     = 2525;
// expert operations end (no further sells or buys) close all positions
const double Expert_DayEndHour = 15.5; // Operational window maximum day hour
// current day not completed ohlc will not be equal
const int Expert_OrdersPerDay = 100; // Number of orders placed per day
const double Expert_PositionExpireHours = 1.5; // Time to expire a position (close it)
const int Expert_TrailingEma = 5; //  EMA Trailing Stop Window in M1

class CTestExpertRBarBands : public CExpertRBarBands
{
    void verifyEntry(){// make tests here
      MqlTick tick = m_ticks[m_ticks.Count()-1];



    }; 
};

CTestExpertRBarBands cExpert = new CTestExpertRBarBands;

//| Expert initialization function
int OnInit(){

    // Use the timer to get missing ticks every 1 second
    // will be more than enough this is not HFT! remember that!
    EventSetTimer(30); // in seconds

    //--- Initializing expert
    if(!cExpert.Init(Symbol(), PERIOD_M1, Expert_EveryTick, Expert_MagicNumber))
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
    cExpert.CheckTicks();
}

// Useless stuff
//| Expert deinitialization function                                 |
void OnDeinit(const int reason){
    EventKillTimer();

    // Tests here
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
