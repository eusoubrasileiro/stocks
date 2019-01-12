#property copyright "Andre L. F"
#property version   "1.01"
#include "BbandsUtil.mqh"
#include "BbandsTest.mqh"

//| Expert initialization function
int OnInit()
{   
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
    
    datetime timenow = TimeCurrent(); // time in seconds from 1970 current time
    Print("Begining Bbands Expert now: ", timenow);
    return(INIT_SUCCEEDED);
}

void PlaceOrderNow(int direction){
    MqlTradeRequest request = {0};
    MqlTradeResult result = {0};
    int ncontracts;
    ulong  position_ticket = 0;

    ncontracts = MathAbs(direction);  // number to buy or sell
    direction = direction/MathAbs(direction); // just sign  -1 or 1
    //--- parameters of request
    request.action     = TRADE_ACTION_DEAL;      // type of trade operation
    request.symbol    = sname;                               // symbol
    if(direction > 0){ //+ postive buy order
        request.price     = SymbolInfoDouble(request.symbol, SYMBOL_ASK); // price for opening
        request.type      = ORDER_TYPE_BUY;                        // order type
        // stop loss and take profit 3:1 rount to 5
        request.tp =request.price*(1+direction*expect_var*3);
        request.tp = MathFloor(request.tp/ticksize)*ticksize;
        request.sl = request.price*(1-direction*expect_var);
        request.sl = MathCeil(request.sl/ticksize)*ticksize;
        request.volume    = quantity*ncontracts; // volume executed in contracts
        request.deviation = deviation*ticksize;    //  allowed deviation from the price
    }
    else { //  -negative sell order       
        if(PositionsTotal() > 0){  // cannot sell what was not bought
            // sell only the same quantity bought to not enter in a short position
            position_ticket = PositionGetTicket(0);  // number of open positions can only be ONE (NETTING MODE)
            double volume = PositionGetDouble(POSITION_VOLUME);
            double decrease = quantity*ncontracts; // how many to sell            
            decrease = (decrease > volume)? volume: decrease; // cannot sell more than what was bought
            request.price = SymbolInfoDouble(request.symbol, SYMBOL_BID); // price for opening
            request.type = ORDER_TYPE_SELL;  // order type
            request.volume = decrease;
        }
        else
            return;
    }
    request.magic  = EXPERT_MAGIC;   // MagicNumber for this Expert
    if(!OrderSend(request,result))
        Print("OrderSend error ", GetLastError());
    //--- information about the operation
    Print("retcode ", result.retcode, "  deal ", result.deal);
    //--- output information about the closure by opposite position
    if(direction < 0)
        Print("Decreased position ",  position_ticket, " by ", request.volume);
}

void ClosePositionbyTime(){
    MqlTradeRequest request;
    MqlTradeResult  result;
    datetime dayend = dayEnd(TimeCurrent()); // 15 minutes before closing the stock market
    datetime timenow = TimeCurrent();
    // NET MODE only ONE buy or ONE sell at once
    int total = PositionsTotal(); // number of open positions
        if(total < 1) // nothing to do
            return;
        for(int i=0; i<total; i++){
        ulong  position_ticket = PositionGetTicket(0);  // ticket of the position
        ulong  magic = PositionGetInteger(POSITION_MAGIC);
        //--- if the MagicNumber matches MagicNumber of the position
        if(magic!=EXPERT_MAGIC)
            return;
        datetime opentime = PositionGetInteger(POSITION_TIME);   // time when the position was open
        if(timenow < opentime+expiretime && timenow < dayend)
            return; // continue open
        ENUM_POSITION_TYPE type = (ENUM_POSITION_TYPE) PositionGetInteger(POSITION_TYPE);  // type of the position
        double volume=PositionGetDouble(POSITION_VOLUME);
        //--- zeroing the request and result values
        ZeroMemory(request);
        ZeroMemory(result);
        //--- setting the operation parameters
        request.action   = TRADE_ACTION_DEAL;        // type of trade operation
        request.position = position_ticket;          // ticket of the position
        request.symbol   = sname;          // symbol
        request.volume   = volume;                   // volume of the position
        request.deviation = deviation*ticksize;                        // 7*0.01 tick size : 7 cents
        request.magic    = EXPERT_MAGIC;             // MagicNumber of the position
        //--- set the price and order type depending on the position type
        if(type==POSITION_TYPE_BUY)
        {
            request.price=SymbolInfoDouble(sname, SYMBOL_BID);
            request.type =ORDER_TYPE_SELL;
        }
        else // contigency should not be here but let it be
        {
            request.price=SymbolInfoDouble(sname, SYMBOL_ASK);
            request.type =ORDER_TYPE_BUY;
        }
        if(!OrderSend(request,result))
            Print("OrderSend error ", GetLastError());
        //--- information about the operation
        Print("closed by time - retcode ",result.retcode, " deal ", result.deal);
    }
}


void sendPrediction(prediction &pred){  // execute or not a prediction
    datetime dayend = dayEnd(TimeCurrent()); // 15 minutes before closing the stock market
    datetime daybegin = dayBegin(TimeCurrent()); // 2 hours after openning
    datetime timenow = TimeCurrent();   
    
    if(pred.direction < 0){ // no matter the time allways send sells
        PlaceOrderNow(pred.direction);
    }
    else{
        // only orders younger than x  minutes after the prediction
        if(pred.time <= timenow + exectolerance && 
            timenow < dayend && timenow > daybegin &&
            nlastDeals() <= dtndeals &&  ndealsDay() <= maxdealsday){
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
        sent_predictions[nsent] = pred;
        nsent++;
    #endif 
}

//| Timer function -- Every 1 minutes
void OnTimer(){
  // check to see if we should close any order
  ClosePositionbyTime();
  // we can work
  #ifndef BACKTESTING
        // not backtesting
        SavePriceData();
        // read predictions file even if with zeroed... with date and time
        int nread = readPredictions();
        if(nread==0){
            return;
        }
        prediction toexecute[]; // new predictions to be executed
        // check for new predictions
        int nnew = newPredictions(toexecute); // get new predictions
        if(nnew == 0) // nothing new
            return;
        for(int i=0; i<nnew; i++)
            sendPrediction(toexecute[i]);
  #else 
        // when testing
        // when testing doesn't need to save data
        prediction pnow;
        if(!TestGetPrediction(pnow, TimeCurrent())) // not time to place an order
            return;
        sendPrediction(pnow);
   #endif   
}

//+------------------------------------------------------------------+
//| Expert deinitialization function                                 |
//+------------------------------------------------------------------+
void OnDeinit(const int reason)
{
    EventKillTimer();
}
//+------------------------------------------------------------------+
//| Expert tick function                                             |
//+------------------------------------------------------------------+
void OnTick()
{
}

//+------------------------------------------------------------------+
//| Trade function                                                   |
//+------------------------------------------------------------------+
void OnTrade()
{
}
//+------------------------------------------------------------------+
//| TradeTransaction function                                        |
//+------------------------------------------------------------------+
void OnTradeTransaction(const MqlTradeTransaction& trans,
    const MqlTradeRequest& request,
    const MqlTradeResult& result)
{
}
//+------------------------------------------------------------------+
//| TesterInit function                                              |
//+------------------------------------------------------------------+
void OnTesterInit()
{
}
//+------------------------------------------------------------------+
void OnTesterDeinit()
{
}
