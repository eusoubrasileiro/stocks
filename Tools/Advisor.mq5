//+------------------------------------------------------------------+
//|                                                      Pattern.mq5 |
//|                                                       Andre L. F |
//|                                                                  |
//+------------------------------------------------------------------+
#property copyright "Andre L. F"
#property link      ""
#property version   "1.00"
#define EXPERT_MAGIC 777707777  // MagicNumber of the expert
// symbols neeeded for training and prediction on PETR4
string symbols[9] = {"BBDC4", "DOL$", "VALE3", "BBAS3", "PETR4",  "ABEV3", "B3SA3", "ITUB4", "WIN@"};

void WriteSymbol(string symbol, MqlRates &arr[]){
    ResetLastError();
    StringAdd(symbol,"RTM1.mt5bin");
    int handle=FileOpen(symbol, FILE_READ|FILE_WRITE|FILE_BIN);
    if(handle!=INVALID_HANDLE){
        //FileSeek(handle,0,SEEK_END);        //--- write array data in the end of the file
        FileWriteArray(handle, arr, 0, WHOLE_ARRAY);
        FileClose(handle);
    }
    else
    Print("Failed to open the file, error ",GetLastError());
}

void SaveDataNow(datetime timenow){
    //---- download the minimal data for training and prediction based on current time
    // nforecast=120
    // nvalidation=nforecast # to validate the model prior prediction
    // ntraining = 5*8*60 # 5*8 hours before for training - 1 week or 8 hours
    // nwindow = nvalidation+nforecast+ntraining = 5*8*60 (2400) + 120 + 120 = 2640
    // additionally it is needed +3*60 = 180 samples due EMA of crossed features
    // rounding up 3200 to consider minutes lost
    int nwindow=3200; // minimal needed data for training validating and predicting now
    MqlRates mqlrates[];
    ArraySetAsSeries(mqlrates, true);
    int nsymbols = ArraySize(symbols);
    int copied;

    for(int i=0; i<nsymbols; i++)
    {
        for(int try=0; try<3; try++) // number of 3 trials of download before giving up
        {
            // -1 if it has not complet it yet
            copied = CopyRates(symbols[i], PERIOD_M1, timenow-nwindow*60, 0, mqlrates);
            Print("copied ", string(copied));
            if(copied < nwindow) //  sleep time(1 seconds) for downloading the data
            Sleep(1000);
        }
        if(copied == -1)
        {
            Print("Failed to get history data for the symbol ", symbols[i]);
            continue;
        }
        Print("minutes downloaded: ", string(copied));
        WriteSymbol(symbols[i], mqlrates);
    }
}

// stores a prediction time and direction
struct prediction {
    datetime time; // time to apply prediction
    int direction; // 1 or -1 : {buy or sell}
};

bool GetPrediction(prediction &pred){
    // read 12 bytes prediction file {datetime:long, direction:int}
    // if the prediction date is equal the last ignore
    // return the prediction direction or 0 if it's the same as the last'
    string fname = "prediction.bin";

    for(int try=0; try<5; try++){ // try 5 times
        int handle=FileOpen(fname, FILE_READ|FILE_BIN);
        if(handle!=INVALID_HANDLE){
            pred.time = (datetime) FileReadLong(handle);
            pred.direction = (int) FileReadInteger(handle, 4);
            FileClose(handle);
            Print("prediction datetime: ", pred.time, " direction: ", pred.direction);
            return true;
        }
    }
    Print("Something is Wrong couldnt read Prediction file");
    return false;
}

int handle = 0;
bool GetPredictionTest(prediction &pred){
    // read 12 bytes prediction file {datetime:long, direction:int}
    // if the prediction date is equal the last ignore
    // return the prediction direction or 0 if it's the same as the last'
    string fname = "prediction.bin";

    if(handle!=INVALID_HANDLE)
        int handle=FileOpen(fname, FILE_READ|FILE_BIN);

    if(handle!=INVALID_HANDLE){
        Print("Something is Wrong couldnt read Prediction file");
        return false;
    }

    pred.time = (datetime) FileReadLong(handle);
    pred.direction = (int) FileReadInteger(handle, 4);

    Print("prediction datetime: ", pred.time, " direction: ", pred.direction);
    return true;
}


unsigned int nstocks(double enterprice){
    // """Needed number of stocks based on:
    // * Minimal acceptable profit $MinP$ (in R$)
    // * Cost per order $CostOrder$   (in R$)
    // * Taxes: $IR$ imposto de renda  (in 0-1 fraction)
    // * Enter Price $EnterPrice$
    // * Expected gain on the operation $ExGain$ (reasonable)  (in 0-1 fraction)
    // $$ N_{stocks} \ge \frac{MinP+2 \ CostOrder}{(1-IR)\ EnterPrice \ ExGain} $$
    // Be reminded that Number of Stocks MUST BE in 100s.
    // This guarantees a `MinP` per order"""
    // # round stocks to 100's
    double exgain=0.01;
    double minp=150;
    double costorder=15;
    double ir=0.2;
    int ceil;
    ceil = int(int((minp+costorder*2)/((1-ir)*enterprice*exgain))/100);
    return ceil*100;
}

datetime dayEnd(datetime timenow){
    // set end of THIS day for operations, 15 minutes before closing the stock market
    // http://www.bmfbovespa.com.br/pt_br/servicos/negociacao/puma-trading-system-bm-fbovespa/para-participantes-e-traders/horario-de-negociacao/acoes/
    datetime endofday;
    MqlDateTime mqltime;
    TimeToStruct(timenow, mqltime);
    mqltime.hour = 16;
    mqltime.min = 45;
    mqltime.sec = 0;
    endofday = StructToTime(mqltime);
    return endofday;
}

datetime dayBegin(datetime timenow){
    // set end of THIS day for operations, 15 minutes before closing the stock market
    // http://www.bmfbovespa.com.br/pt_br/servicos/negociacao/puma-trading-system-bm-fbovespa/para-participantes-e-traders/horario-de-negociacao/acoes/
    datetime daybegin;
    MqlDateTime mqltime;
    TimeToStruct(timenow, mqltime);
    // no orders on the first 2 hours, starts at 10 but cannot place orders until 12
    mqltime.hour = 12;
    mqltime.min = 0;
    mqltime.sec = 0;
    daybegin = StructToTime(mqltime);
    return daybegin;
}

//| Expert initialization function
int OnInit()
{
    EventSetTimer(60);     //--- create timer
    datetime timenow = TimeCurrent(); // time in seconds from 1970 current time
    SaveDataNow(timenow);
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
            if(opentime+expiretime > timenow || timenow > endday)
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

// last prediction
prediction plast={0};
//| Timer function -- Every 1 minutes
void OnTimer(){
    prediction pnow;
    bool status;
    datetime dayend, daybegin;
    datetime timenow = TimeCurrent(); // time in seconds from 1970 current time

    dayend = dayEnd(timenow); // 15 minutes before closing the stock market
    daybegin = dayBegin(timenow); // 2 hours after openning
    // check to see if we should close any order
    ClosePositionsbyTime(timenow, dayend, 3600*2); // 2 hours expire time
    // do not place orders 1:30 before the end of the day
    if(timenow > dayend - (90*60))
        return;
    // do not place orders in the begin of the day
    if(timenow < daybegin)
        return;
    // we can work
    SaveDataNow(timenow);
    Sleep(5000); // sleep enough time for the python worker make
    //a new prediction file
    // read prediction file... with date and time
    if(!GetPrediction(pnow)) // something wrong there was no prediction
        return;
    // same prediction or there was no prediction
    if(pnow.direction == plast.direction && pnow.time == plast.time)
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
