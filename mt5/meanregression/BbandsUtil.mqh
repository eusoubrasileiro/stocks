#include "BbandsDefinitions.mqh"

void WriteSymbolFile(string symbol, MqlRates &arr[]){
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

void SavePriceData(){
    //---- download the minimal data for training and prediction based on current time
    // asks much more than needed to workaround unsolvable minute data lost
    MqlRates mqlrates[]; 
    datetime timenow = TimeCurrent();
    int nsymbols = ArraySize(symbols);
    int copied=-1;
    int totalcopied=0;

    for(int i=0; i<nsymbols; i++){
        for(int try=0; try<3; try++){ // number of 3 trials before giving up
            // -1 if it has not complet it yet
            copied = CopyRates(symbols[i], PERIOD_M1, 0,  ntrainingbars, mqlrates);
            if(MathAbs(copied) < ntrainingbars)  //  sleep time (2 seconds) for downloading the data
                Sleep(2000);
        }
        if(copied == -1){
            Print("Failed to get history data for the symbol ", symbols[i]);
            continue;
        }
        WriteSymbolFile(symbols[i], mqlrates);
        totalcopied += copied;
    }
    Print("percent price data saved: ",  string((float) totalcopied/(ntrainingbars*nsymbols)));
}


long readPredictions(void){
    // read 12 bytes predictions file {datetime:long, direction:int}
    string fname = "predictions.bin";
    //FILE_COMMON location of the file in a shared folder for all client terminals \Terminal\Common\Files
    ///home/andre/.wine/drive_c/users/andre/Application Data/MetaQuotes/Terminal/Common/Files
    long nread = 0;
    while(true){ // try inf
      // int handle=FileOpen(fname, FILE_READ|FILE_BIN|FILE_COMMON);
      nread =  FileLoad(fname, read_predictions, FILE_COMMON);
      if(nread == -1){
        Sleep(1000); // wait a bit
        continue;
      }
      else {
        Print("read ", nread, " predictions");
        if(nread > 0)
          Print("last: prediction datetime: ", read_predictions[nread-1].time, " direction: ", read_predictions[nread-1].direction);        
        break;
      }
    }
    return nread;
}


bool isInPredictions(prediction &value, prediction &array[]){
  // check wether the value is in the array
  uint n = ArraySize(array);
  for(uint j=0; j<n; j++)
    if(value.direction == array[j].direction && value.time == array[j].time)
      return true;
  return false;
}

int newPredictions(prediction &newpredictions[]){
  // compare read_predictions with executed_predictions
  // return number of new orders 'toexecute_predictions'
  uint nread = ArraySize(read_predictions);
  ArrayResize(newpredictions, nread); // maximum is the number read
  int nnew = 0;
  for(uint i=0; i<nread; i++){
    // check if it has already been executed
    if(!isInPredictions(read_predictions[i], sent_predictions)){
      newpredictions[nnew] = read_predictions[i];
      nnew++; // new prediction
    }
  }
  return int(nnew);
}

datetime dayEnd(datetime timenow){
    // set end of THIS day for operations, 15 minutes before closing the stock market
    // http://www.b3.com.br/en_us/solutions/platforms/puma-trading-system/for-members-and-traders/trading-hours/derivatives/indices/
    datetime day0hour;
    MqlDateTime mqltime;
    TimeToStruct(timenow, mqltime);
    // calculate begin of the day
    day0hour = timenow - (mqltime.hour*3600+mqltime.min*60+mqltime.sec);
    return day0hour+endhour*3600+endminute*60;
}

datetime dayBegin(datetime timenow){
    datetime day0hour;
    MqlDateTime mqltime;
    TimeToStruct(timenow, mqltime);
    // no orders on the first 30 minutes
    day0hour = timenow - (mqltime.hour*3600+mqltime.min*60+mqltime.sec);
    return day0hour+starthour*3600+startminute*60;
}

//  number of orders oppend on the last n minutes defined on Definitons.mqh
int nlastDeals(){
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
         entry =HistoryDealGetInteger(ticket, DEAL_ENTRY);
         if(entry==DEAL_ENTRY_IN) // a buy or a sell (entry not closing/exiting)
            lastnopen++;
      }
    }
    return lastnopen;
}

int ndealsDay(){
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
