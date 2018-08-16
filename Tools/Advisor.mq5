//+------------------------------------------------------------------+
//|                                                      Pattern.mq5 |
//|                                                       Andre L. F |
//|                                                                  |
//+------------------------------------------------------------------+
#property copyright "Andre L. F"
#property link      ""
#property version   "1.00"
//--- input parameters
input int      Input1;
input datetime Input2;

// symbols neeeded for training and prediction on PETR4
string symbols[9] = {"BBDC4", "DOL$", "VALE3", "BBAS3", "PETR4",  "ABEV3", "B3SA3", "ITUB4", "WIN@"};
unsigned int iminutes; // index of of minutes

// 6 horas e 55 minutos = 360 + 55 = 415 minutos
// http://www.bmfbovespa.com.br/pt_br/servicos/negociacao/puma-trading-system-bm-fbovespa/para-participantes-e-traders/horario-de-negociacao/acoes/

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
}

prediction GetPrediction(){
    // read 12 bytes prediction file {datetime:long, direction:int}
    // if the prediction date is equal the last ignore
    // return the prediction direction or 0 if it's the same as the last'
    string fname = "prediction.bin";
    prediction ;

    for(int try=0; try<5; try++){ // try 5 times
        int handle=FileOpen(fname, FILE_READ|FILE_BIN);
        if(handle!=INVALID_HANDLE){
            pred.time = (datetime) FileReadLong(handle);
            pred.direction = (int) FileReadInteger(handle, 4);
            FileClose(handle);
            Print('prediction datetime: ', pred.time, ' direction: ', pred.direction);
            return pred;
        }
    }
    Print('Something is Wrong couldnt read Prediction file');
    return -1;
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

//| Expert initialization function
int OnInit()
{
    //--- create timer
    EventSetTimer(60);
    datetime timenow = TimeCurrent(); // time in seconds from 1970 current time
    SaveDataNow(timenow);
    // minutes start counting now
    iminutes = 0;
    return(INIT_SUCCEEDED);
}

unsigned int expert_magic = 777707777;

unsigned long PlaceOrderNow(int direction){
//  bool  OrderSend(
// MqlTradeRequest&  request,      // query structure
// MqlTradeResult&   result        // structure of the answer
// );
    MqlTradeResult result = {0};
    MqlTradeRequest request = {0};
    unsigned long ticket;

    status = OrderSend(request, result);
    //--- parameters of request
   request.action    = TRADE_ACTION_DEAL;                     // type of trade operation
   request.symbol    = Symbol();                              // symbol
   request.volume    = 0.1;                                   // volume of 0.1 lot
   request.type      = ORDER_TYPE_BUY;                        // order type
   request.price     = SymbolInfoDouble(Symbol(),SYMBOL_ASK); // price for opening
   request.deviation = 5;                                     // allowed deviation from the price
   request.magic     = expert_magic;                          // MagicNumber of the order
   //--- send the request
   if(!OrderSend(request,result))
        PrintFormat("OrderSend error %d",GetLastError());     // if unable to send the request, output the error code
        //--- information about the operation
    PrintFormat("retcode=%u  deal=%I64u  order=%I64u",result.retcode,result.deal,result.order);
    ticket =  result.deal;

    return ticket;
}

//| Timer function -- Every 1 minutes
void OnTimer(){
    unsigned int deal_ticket;
    /// datetime lastprediction, datetime timenow
    prediction plast, pnow;
    // save the new data
    datetime timenow = TimeCurrent(); // time in seconds from 1970 current time
    SaveDataNow(timenow);
    Sleep(3000); // sleep enough time for the python worker make
    //a new prediction file
    // read prediction file... with date and time
    pnow = GetPrediction();
     // same prediction or there was no prediction
    if(pnow == plast)
        return;

    PlaceOrderNow(direction);
    // place orders/
    // verify if an order should be closed by expiring time


    //iminutes++;
    // update last prediction
    plast = pnow
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
