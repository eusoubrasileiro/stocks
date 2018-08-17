#include "Definitions.mqh"

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