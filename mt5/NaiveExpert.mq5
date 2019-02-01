//+------------------------------------------------------------------+
//|                                                      ProjectName |
//|                                      Copyright 2012, CompanyName |
//|                                       http://www.companyname.net |
//+------------------------------------------------------------------+
#property copyright "Andre L. F"
#property version   "1.01"
#include "BbandsUtil.mqh"
#include "BbandsTest.mqh"
#include "BbandsControl.mqh"

//| Expert initialization function
int OnInit(){
   EventSetTimer(60);
//--- create timer
#ifdef BACKTESTING
// when testing doesnt need to wait 1 minute
// when testing doesn't need to save data
// read all predictions at once
   TestReadPredictions();
#else
// real time operations
   SavePriceData();
#endif
// to set stop loss based on previous Stdev indicator
   hstdev=iStdDev(sname, PERIOD_M1, windowstdev, 0, MODE_SMA, PRICE_TYPICAL);
   hemah=iMA(sname, PERIOD_M1, windowema, 0, MODE_EMA, PRICE_TYPICAL);
   hemal=iMA(sname, PERIOD_M1, windowema, 0, MODE_EMA, PRICE_TYPICAL);

   if(hstdev==INVALID_HANDLE|| hema == INVALID_HANDLE){
      printf("Error creating Stdev or EMAindicator");
      return(INIT_FAILED);
   }

   datetime timenow=TimeCurrent(); // time in seconds from 1970 current time
   Print("Begining Bbands Expert now: ",timenow);
   return(INIT_SUCCEEDED);
}
//+------------------------------------------------------------------+
//|                                                                  |
//+------------------------------------------------------------------+
double stopsStdev(){
// get the stop loss based on the moving average ratio is 1:1
   double   stdev[1];
   if (CopyBuffer(hstdev, 0, 0, 1, stdev) != 1){
      Print("CopyBuffer from Stdev failed");
   }
   else {
      laststdev =  stdev[0];
   }
   return laststdev*3; // 3*stdev is 89% data
}

//+------------------------------------------------------------------+
//|                                                                  |
//+------------------------------------------------------------------+
void PlaceOrderNow(int direction){
   MqlTradeRequest request={0};
   MqlTradeResult result={0};
   int ncontracts;
   ulong  position_ticket=0;

   ncontracts=MathAbs(direction);  // number to buy or sell
                                   // get stops loss and gain
   double stop=stopsStdev();

// direction // just sign  -1 or 1
//--- parameters of request
   request.action=TRADE_ACTION_DEAL;      // type of trade operation
   request.symbol=sname;                               // symbol
   if(direction>0)
     { //+ postive buy order
      request.price=SymbolInfoDouble(request.symbol,SYMBOL_ASK); // price for opening
      request.type=ORDER_TYPE_BUY;                        // order type
      // stop loss and take profit 1.25:1 rount to 5
      request.tp =request.price+direction*stop*1.25;
      request.tp = MathFloor(request.tp/ticksize)*ticksize;
      request.sl = request.price-direction*stop;
      request.sl = MathCeil(request.sl/ticksize)*ticksize;
      request.volume=quantity*ncontracts; // volume executed in contracts
      request.deviation=deviation*ticksize;    //  allowed deviation from the price
     }
   else
     { //  -negative sell order
      if(PositionsTotal()>0)
        {  // cannot sell what was not bought
         // sell only the same quantity bought to not enter in a short position
         position_ticket=PositionGetTicket(0);  // number of open positions can only be ONE (NETTING MODE)
         if(PositionGetInteger(POSITION_MAGIC) != EXPERT_MAGIC)  // can only close position open by the expert
            return;
         double volume=PositionGetDouble(POSITION_VOLUME);
         double decrease=quantity*ncontracts; // how many to sell
         decrease=(decrease>volume)? volume: decrease; // cannot sell more than what was bought
         request.price=SymbolInfoDouble(request.symbol,SYMBOL_BID); // price for opening
         request.type=ORDER_TYPE_SELL;  // order type
         request.volume=decrease;
        }
      else
         return;
     }
   request.magic=EXPERT_MAGIC;   // MagicNumber for this Expert
   if(!OrderSend(request,result))
      Print("OrderSend error ",GetLastError());
//--- information about the operation
   Print("retcode ",result.retcode,"  deal ",result.deal);
//--- output information about the closure by opposite position
   if(direction<0)
      Print("Decreased position ",position_ticket," by ",request.volume);
}

//+------------------------------------------------------------------+
//|                                                                  |
//+------------------------------------------------------------------+
void sendPrediction(prediction &pred){  // execute or not a prediction
   datetime dayend=dayEnd(TimeCurrent()); // 15 minutes before closing the stock market
   datetime daybegin=dayBegin(TimeCurrent()); // 2 hours after openning
   datetime timenow = TimeCurrent();

   if(pred.direction<0)
     { // no matter the time allways send sells
      PlaceOrderNow(pred.direction);
     }
   else
     {
      // only orders younger than x  minutes after the prediction
      if(pred.time<=timenow+exectolerance &&
         timenow<dayend && timenow>daybegin &&
         nlastDeals()<=dtndeals && ndealsDay()<=maxdealsday)
        {
         // deals are only ENTRY_IN deals that means entering a position
         // do not place orders in the end of the day
         // do not place orders in the begin of the day
         // cannot make more than`dtndeals` deals per dt
         // dont open more than `maxdealsday` positions per day
         PlaceOrderNow(pred.direction);
        }
     }
#ifndef BACKTESTING
// real operation record operations processed
   sent_predictions[nsent]=pred;
   nsent++;
#endif
}

void changeStop(double change) {
   // modify stop loss
   MqlTradeRequest request;
   MqlTradeResult  result;
   //   double stops=0;
   request.action = TRADE_ACTION_SLTP;
   request.symbol = Symbol();
   request.sl = PositionGetDouble(POSITION_SL)  +  change;
   request.sl = MathFloor(request.sl/ticksize)*ticksize;
   request.tp = PositionGetDouble(POSITION_TP) + change;
   request.tp = MathFloor(request.tp/ticksize)*ticksize;
    if(!OrderSend(request, result))
        Print("OrderSend error ",GetLastError());
}

bool trailingStopLossEma(){
   double ema[1]; // actual ema value
   bool trailled = false;

   if (CopyBuffer(hema, 0,  0,  1, ema) != 1){
      Print("CopyBuffer from Stdev failed");
   }
   if( PositionsTotal() > 0){        // net mode
         if( PositionGetInteger(POSITION_TYPE) == POSITION_TYPE_BUY &&
            PositionGetInteger(POSITION_MAGIC) == EXPERT_MAGIC){
            if(ema[0] > lastema && ema[0] > lastemachange) {
             // it is going up steady  compared with last ema value  and with last changed stop
               changeStop(ema[0]-lastema);
               trailled = true;
               lastemachange = ema[0];
            }
         }
   }
   lastema = ema[0];
   return trailled;
}


//| Timer function -- Every 1 minutes
void OnTimer() {
    trailingStopLossEma();
    // check to see if we should close any order
    ClosePositionbyTime();
    // we can work
#ifndef BACKTESTING
    // not backtesting
    SavePriceData();
    // read predictions file even if with zeroed... with date and time
   int nread=readPredictions();
   if(nread==0)
     {
      return;
     }
   prediction toexecute[]; // new predictions to be executed
                           // check for new predictions
   int nnew=newPredictions(toexecute); // get new predictions
   if(nnew==0) // nothing new
      return;
   for(int i=0; i<nnew; i++)
      sendPrediction(toexecute[i]);
#else
// when testing
// when testing doesn't need to save data
   prediction pnow;
   if(!TestGetPrediction(pnow,TimeCurrent())) // not time to place an order
      return;
   sendPrediction(pnow);
#endif
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
    if(PositionsTotal()==0 || PositionGetInteger(POSITION_MAGIC) != EXPERT_MAGIC)
        lastemachange = 0;
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
