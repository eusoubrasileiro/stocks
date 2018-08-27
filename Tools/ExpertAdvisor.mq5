#property copyright "Andre L. F"
#property version   "1.00"
#include "PatternNN.mqh"
#include "Testing.mqh"

//| Expert initialization function
int OnInit()
{
    datetime timenow = TimeCurrent(); // time in seconds from 1970 current time
    //--- create timer
    if(!TESTINGW_FILE){ // real operation
        EventSetTimer(60);
        SaveDataNow(timenow);
    }
    else{ // when testing doesnt need to wait 1 minute
        EventSetTimer(60);
        // when testing doesn't need to save data
        // read all predictions at once
        TestReadPredictions();
    }

    Print("Begining now: ", timenow);

    return(INIT_SUCCEEDED);
}

bool PlaceOrderNow(int direction){
    MqlTradeResult result = {0};
    MqlTradeRequest request = {0};

    //--- parameters of request
    request.action    = TRADE_ACTION_DEAL;                     // type of trade operation
    request.symbol    = "PETR4";                               // symbol
    if(direction == 1){ // buy order
        request.price     = SymbolInfoDouble(request.symbol,SYMBOL_ASK); // price for opening
        request.type      = ORDER_TYPE_BUY;                        // order type
    }
    else{ //  -1 sell order
        request.price     = SymbolInfoDouble(request.symbol,SYMBOL_BID); // price for opening
        request.type      = ORDER_TYPE_SELL;                        // order type
    }
    // stop loss and take profit 3:1
    request.tp =request.price*(1+direction*expect_var*3);
    request.sl = request.price*(1-direction*expect_var);
    request.volume    = nstocks(request.price);                // volume in 100s lot
    request.deviation = 7;                                  // 0.07 cents : allowed deviation from the price
    request.magic     = EXPERT_MAGIC;                          // MagicNumber of the order
    if(!OrderSend(request,result))     //--- send the request
        PrintFormat("OrderSend error %d",GetLastError());     // if unable to send the request, output the error code
        return false;
    //--- information about the operation
    PrintFormat("retcode=%u  deal=%I64u  order=%I64u",result.retcode,result.deal,result.order);
    return true;
}

void ClosePositionsbyTime(datetime timenow, datetime endday, int expiretime){
    MqlTradeRequest request;
    MqlTradeResult  result;
    int total=PositionsTotal(); // number of open positions
    for(int i=total-1; i>=0; i--) //--- iterate over all open positions, avoid the top
    {
        //--- parameters of the order
        ulong  position_ticket = PositionGetTicket(i);                                    // ticket of the position
        ulong  magic = PositionGetInteger(POSITION_MAGIC);                                // MagicNumber of the position
        double volume=PositionGetDouble(POSITION_VOLUME);
        datetime opentime = PositionGetInteger(POSITION_TIME);                            // time when the position was open
        ENUM_POSITION_TYPE type = (ENUM_POSITION_TYPE) PositionGetInteger(POSITION_TYPE);  // type of the position
        if(magic==EXPERT_MAGIC) //--- if the MagicNumber matches
        {
            // time to close
            if(timenow > opentime+expiretime || timenow > endday)
            {
                //--- zeroing the request and result values
                ZeroMemory(request);
                ZeroMemory(result);
                //--- setting the operation parameters
                request.action   = TRADE_ACTION_DEAL;        // type of trade operation
                request.position = position_ticket;          // ticket of the position
                request.symbol   = "PETR4";          // symbol
                request.volume   = volume;                   // volume of the position
                request.deviation = 7;                        // 7*0.01 tick size : 7 cents
                request.magic    =EXPERT_MAGIC;             // MagicNumber of the position
                //--- set the price and order type depending on the position type
                if(type==POSITION_TYPE_BUY)
                {
                    request.price=SymbolInfoDouble("PETR4",SYMBOL_BID);
                    request.type =ORDER_TYPE_SELL;
                }
                else
                {
                    request.price=SymbolInfoDouble("PETR4",SYMBOL_ASK);
                    request.type =ORDER_TYPE_BUY;
                }
                //--- output information about the closure by opposite position
                PrintFormat("Close #%I64d %s %s by #%I64d",position_ticket, EnumToString(type),request.position_by);
                //--- send the request
                if(!OrderSend(request,result))
                    PrintFormat("OrderSend error %d",GetLastError()); // if unable to send the request, output the error code
                //--- information about the operation
                PrintFormat("retcode=%u  deal=%I64u  order=%I64u",result.retcode,result.deal,result.order);
            }
        }
    }
}


//| Timer function -- Every 1 minutes
void OnTimer(){
    prediction pnow;
    datetime dayend, daybegin;
    datetime timenow = TimeCurrent(); // time in seconds from 1970 current time

    dayend = dayEnd(timenow); // 15 minutes before closing the stock market
    daybegin = dayBegin(timenow); // 2 hours after openning
    // check to see if we should close any order
    ClosePositionsbyTime(timenow, dayend, 3600*2); // 2 hours expire time

    // we can work
    if(!TESTINGW_FILE){ // not testing
        SaveDataNow(timenow);
        Sleep(5000); // sleep enough time for the python worker make
        //a new prediction file
        // read prediction file... with date and time
        if(!GetPrediction(pnow)) // something wrong there was no prediction
            return;
    }
    else{ // when testing
        // when testing doesn't need to save data
        //Sleep(10); //sleep few 10 ms
        if(!TestGetPrediction(pnow, timenow)) // not time to place an order
            return;
    }
    // same prediction or there was no prediction
    if(pnow.direction == plast.direction && pnow.time == plast.time)
        return;
    // moved to here so we can check if something is wrong
    // do not place orders 1:30 before the end of the day
    if(timenow > dayend - (90*60))
        return;
    // do not place orders in the begin of the day
    if(timenow < daybegin)
        return;
    // no orders older than 2 minutes, the second condition almost never used
    if(pnow.time > timenow + 3*60 || pnow.time < timenow - 3*60 )
        return;

    if(nlastOrders() > 3) // cannot place more than 4 orders per
        return;
        
    // number of open positions dont open more than
    if(PositionsTotal() > 14) // dont open more than 14 positions
        return;

    // place
    PlaceOrderNow(pnow.direction);

    // update last prediction
    plast = pnow;
}

//+------------------------------------------------------------------+
//| Expert deinitialization function                                 |
//+------------------------------------------------------------------+
void OnDeinit(const int reason)
{
    //--- destroy timer
    EventKillTimer();
}
//+------------------------------------------------------------------+
//| Expert tick function                                             |
//+------------------------------------------------------------------+
void OnTick()
{
    //---
}

//+------------------------------------------------------------------+
//| Trade function                                                   |
//+------------------------------------------------------------------+
void OnTrade()
{
    //---

}
//+------------------------------------------------------------------+
//| TradeTransaction function                                        |
//+------------------------------------------------------------------+
void OnTradeTransaction(const MqlTradeTransaction& trans,
    const MqlTradeRequest& request,
    const MqlTradeResult& result)
{
    //---

}
//+------------------------------------------------------------------+
//| TesterInit function                                              |
//+------------------------------------------------------------------+
void OnTesterInit()
{
    //---

}
//+------------------------------------------------------------------+
void OnTesterDeinit()
{
    //---

}
