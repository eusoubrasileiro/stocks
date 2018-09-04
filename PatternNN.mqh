#include "Definitions.mqh"

void WriteSymbol(string symbol, MqlRates &arr[]){
    ResetLastError();
    StringAdd(symbol,"RTM1.mt5bin");
    int handle=FileOpen(symbol, FILE_READ|FILE_WRITE|FILE_BIN|FILE_COMMON);
    if(handle!=INVALID_HANDLE){
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
    // rounding up 3200
    // asks much more than needed to workaround unsolvable minute data lost
    int nwindow=5000; // minimal needed data for training validating and predicting now
    MqlRates mqlrates[];
    int nsymbols = ArraySize(symbols);
    int copied, error;
    int totalcopied=0;

    for(int i=0; i<nsymbols; i++)
    {
        for(int try=0; try<3; try++) // number of 3 trials of download before giving up
        {
            // -1 if it has not complet it yet
            copied = CopyRates(symbols[i], PERIOD_M1, 0,  nwindow, mqlrates);
            if(copied < nwindow){ //  sleep time(1 seconds) for downloading the data
                Sleep(7000);
                // 4401 Request history not found, no data yet
                error =  GetLastError();
            }
        }
        if(copied == -1)
        {
            Print("Failed to get history data for the symbol ", symbols[i]);
            continue;
        }
        WriteSymbol(symbols[i], mqlrates);
        totalcopied += copied;
    }
    Print("percent data downloaded: ",  string((float) totalcopied/(nwindow*nsymbols)));
}


bool GetPrediction(prediction &pred){
    // read 12 bytes prediction file {datetime:long, direction:int}
    // if the prediction date is equal the last ignore
    // return the prediction direction or 0 if it's the same as the last'
    string fname = "prediction.bin";
    //FILE_COMMON location of the file in a shared folder for all client terminals \Terminal\Common\Files
    ///home/andre/.wine/drive_c/users/andre/Application Data/MetaQuotes/Terminal/Common/Files
    for(int try=0; try<5; try++){ // try 5 times
        int handle=FileOpen(fname, FILE_READ|FILE_BIN|FILE_COMMON);
        if(handle!=INVALID_HANDLE){
//            pred.time = (datetime) FileReadLong(handle);
//            pred.direction = (int) FileReadInteger(handle, 4);
            FileReadStruct(handle, pred);
            FileClose(handle);
            Print("prediction datetime: ", pred.time, " direction: ", pred.direction);
            return true;
        }
        else
            Sleep(2000); // cannot read so wait a bit
    }
    Print("Something is Wrong couldnt read Prediction file");
    return false;
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
    // double minp=; minprofit
    double costorder=15;
    double ir=0.2;
    int ceil;
    ceil = int(int((minprofit+costorder*2)/((1-ir)*enterprice*exgain))/100);
    return ceil*100;
}

datetime dayEnd(datetime timenow){
    // set end of THIS day for operations, 15 minutes before closing the stock market
    // http://www.bmfbovespa.com.br/pt_br/servicos/negociacao/puma-trading-system-bm-fbovespa/para-participantes-e-traders/horario-de-negociacao/acoes/
    datetime endofday;
    MqlDateTime mqltime;
    TimeToStruct(timenow, mqltime);
    // calculate begin of the day
    endofday = timenow - (mqltime.hour*3600+mqltime.min*60+mqltime.sec);
    return endofday+16*3600+45*60;// 16:45 min
}

datetime dayBegin(datetime timenow){
    // set end of THIS day for operations, 15 minutes before closing the stock market
    // http://www.bmfbovespa.com.br/pt_br/servicos/negociacao/puma-trading-system-bm-fbovespa/para-participantes-e-traders/horario-de-negociacao/acoes/
    datetime daybegin;
    MqlDateTime mqltime;
    TimeToStruct(timenow, mqltime);
    // no orders on the first 2 hours, starts at 10 but cannot place orders until 12
    // calculate begin of the day
    daybegin = timenow - (mqltime.hour*3600+mqltime.min*60+mqltime.sec);
    return daybegin+12*3600;
}

//  number of orders oppend on the last n minutes defined on Definitons.mqh
int nlastOrders(){
    int lastnopen=0; // number of orders openned on the last n minutes
    //--- request trade history
    datetime now = TimeCurrent();
    // on the last 15 minutes count the openned orders
    HistorySelect(now-60*perdt,now);
    ulong ticket;
    long entry;
    uint  total=HistoryDealsTotal(); // total deals on the last n minutes
    //--- for all deals
   for(uint i=0;i<total;i++){
      //--- try to get deals ticket
      ticket=HistoryDealGetTicket(i);
      if(ticket>0){ // get the deal entry property
         entry =HistoryDealGetInteger(ticket,DEAL_ENTRY);
         if(entry==DEAL_ENTRY_IN) // a buy or a sell (entry not closing/exiting)
            lastnopen++;
      }
    }
    return lastnopen;
}

int nordersDay(){
    int open=0; // number of orders openned
    //--- request trade history
    datetime now = TimeCurrent();
    // on the last 15 minutes count the openned orders
    HistorySelect(dayBegin(now), now);
    ulong ticket;
    long entry;
    uint  total=HistoryDealsTotal(); // total deals
    //--- for all deals
   for(uint i=0;i<total;i++){
      //--- try to get deals ticket
      ticket=HistoryDealGetTicket(i);
      if(ticket>0){ // get the deal entry property
         entry =HistoryDealGetInteger(ticket,DEAL_ENTRY);
         if(entry==DEAL_ENTRY_IN) // a buy or a sell (entry not closing/exiting)
            open++;
      }
    }
    return open;
}
