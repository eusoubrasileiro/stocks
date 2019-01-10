#property copyright "Andre L. F"
#property version   "1.01"
#include "BbandsUtil.mqh"
#include "BbandsTest.mqh"

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
    Print("Begining Bbands Expert now: ", timenow);

    return(INIT_SUCCEEDED);
}

// wait the execution of an order until it turns in a deal
void waitDeal(MqlTradeResult &result){
  if(result.recode == TRADE_RETCODE_DONE)
    return;
  if(result.recode == TRADE_RETCODE_PLACED || result.recode == TRADE_RETCODE_DONE_PARTIAL){ // only working with TRADE_ACTION_DEAL
    // result deal is not filled properly due the trading server
    // not having executed it yet so we will keep looking
    ulong order_ticket = result.order;
    long code = 0;
    while(True){ // wait until order is fully executed in a deal
        OrderSelect(order_ticket);
        code = OrderGetInteger(order_ticket, ENUM_ORDER_STATE)
        if(code == ORDER_STATE_FILLED) // fully executed
          break;
    }
  }
}

bool PlaceOrderNow(int direction, MqlTradeResult &result){
    MqlTradeRequest request = {0};
    int nbuys;
    int ncontracts;

    ncontracts = MathAbs(direction);  // number to buy or sell
    direction = direction/MathAbs(direction); // just sign  -1 or 1
    //--- parameters of request
    request.action    = TRADE_ACTION_DEAL;                     // type of trade operation
    request.symbol    = sname;                               // symbol
    if(direction > 0){ //+ postivie buy order
        request.price     = SymbolInfoDouble(request.symbol,SYMBOL_ASK); // price for opening
        request.type      = ORDER_TYPE_BUY;                        // order type
    }
    else{ //  -negative sell order
        nbuys = PositionsTotal(); // number of open positions
        if(nbuys < 1 ) // cannot sell what was not bought
            return false;
        else{ // sell only the same quantity bought to not enter in a short position
            ncontracts = MathMin(nbuys, ncontracts);
        }
        request.price     = SymbolInfoDouble(request.symbol,SYMBOL_BID); // price for opening
        request.type      = ORDER_TYPE_SELL;                        // order type
    }
    // stop loss and take profit 3:1 rount to 5
    request.tp =request.price*(1+direction*expect_var*3);
    request.tp = MathFloor(request.tp/ticksize)*ticksize;
    request.sl = request.price*(1-direction*expect_var);
    request.sl = MathCeil(request.sl/ticksize)*ticksize;
    request.volume    = nv*ncontracts; // volume executed in contracts
    request.deviation = deviation*ticksize;                                  //  allowed deviation from the price
    request.magic     = EXPERT_MAGIC;                          // MagicNumber of the order
    if(!OrderSend(request, result))     //--- send the request
        PrintFormat("OrderSend error %d",GetLastError());     // if unable to send the request, output the error code
        return false;
    //--- information about the operation
    PrintFormat("retcode=%d  deal=%d  order=%d",result.retcode,result.deal,result.order);
    waitDeal(result);
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
                request.symbol   = sname;          // symbol
                request.volume   = volume;                   // volume of the position
                request.deviation = deviation*ticksize;                        // 7*0.01 tick size : 7 cents
                request.magic    =EXPERT_MAGIC;             // MagicNumber of the position
                //--- set the price and order type depending on the position type
                if(type==POSITION_TYPE_BUY)
                {
                    request.price=SymbolInfoDouble(sname, SYMBOL_BID);
                    request.type =ORDER_TYPE_SELL;
                }
                else
                {
                    request.price=SymbolInfoDouble(sname, SYMBOL_ASK);
                    request.type =ORDER_TYPE_BUY;
                }
                //--- output information about the closure by opposite position
                PrintFormat("Close position %s %s by %d", position_ticket, EnumToString(type), request.position_by);
                //--- send the request
                if(!OrderSend(request,result))
                    PrintFormat("OrderSend error %d", GetLastError()); // if unable to send the request, output the error code
                //--- information about the operation
                PrintFormat("retcode=%d  deal=%d  order=%d",result.retcode,result.deal,result.order);

                waitDeal(result);
            }
        }
    }
}


void sendPrediction(prediction &pred, datetime timenow, datetime daybegin, datetime dayend){
  // nexec;
  // execute or not a prediction
  if(pred.direction < 0){ // no matter the time allways send sells
    PlaceOrderNow(pred.direction);
    executed_predictions[nexec] = pred;
    nexec++;
  }
  // no orders older than 2 minutes, the second condition almost never used
  if(pred.time > timenow + 2*60 || pred.time < timenow - 2*60 )
      return;
  // moved to here so we can check if something is wrong
  // do not place orders in the end of the day
  // do not place orders in the begin of the day
  if(timenow > dayend && timenow < daybegin)
    return;
  // cannot place more than xxx orders per dt
  // dont open more than yyy positions per day
  if(nlastOrders() >= dtnorders || nordersDay() >= maxorders)
      return;    // number of open positions dont open more than that per day

  PlaceOrderNow(pred.direction);
  executed_predictions[nexec] = pred;
  nexec++;
}

//| Timer function -- Every 1 minutes
void OnTimer(){
  prediction toexecute[]; // new predictions to be executed
  datetime dayend, daybegin;
  datetime timenow = TimeCurrent(); // time in seconds from 1970 current time
  dayend = dayEnd(timenow); // 15 minutes before closing the stock market
  daybegin = dayBegin(timenow); // 2 hours after openning
  // check to see if we should close any order
  ClosePositionsbyTime(timenow, dayend, expire_time);
  // we can work
  if(!TESTINGW_FILE){ // not testing
      SaveDataNow(timenow);
      // read predictions file even if with zeroed... with date and time
      int nread = readPredictions();
      if(nread==0){
        return;
      }
      // check for new predictions
      int nnew = newPredictions(toexecute); // get new predictions
      if(nnew == 0) // nothing new
        return;
      for(int i=0; i< nnew; i++)
        sendPrediction(toexecute[i], timenow, daybegin, dayend);

  }
  else{ // when testing
      // when testing doesn't need to save data
      //Sleep(10); //sleep few 10 ms
      prediction pnow;
      if(!TestGetPrediction(pnow, timenow)) // not time to place an order
          return;
      sendPrediction(pnow, timenow, daybegin, dayend);
  }
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
