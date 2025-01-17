// Download 1 minute standard time bars data for specified symbols
// write on a binary file the mqlrates array
#property copyright "Andre L. Ferreira June 2018"
#property version   "1.00"
#property script_show_inputs

//--- global variables
int    count=0;
string symbols[10] = {"WIN@", "BBDC4", "DOL$", "VALE3", "BBAS3", "PETR4",  "ABEV3", "B3SA3", "ITUB4", "WEGE3"};
const int minutes=INT_MAX;

void OnStart(){
   MqlRates mqlrates[];
   ArraySetAsSeries(mqlrates, true);
   int nsymbols = ArraySize(symbols);
   int copied;

   for(int i=0; i<nsymbols; i++){
       for(int try=0; try<3; try++){ // number of 3 trials of download before giving up
          // -1 if it has not complet it yet
          copied = CopyRates(symbols[i], PERIOD_M1, 0, minutes, mqlrates);
       }
       if(copied == -1){
          Print("Failed to get history data for the symbol ", symbols[i]);
          continue;
       }
       Print("symbol ", symbols[i], " minutes downloaded: ", string(copied));
       PreviewSymbol(symbols[i], mqlrates);
       WriteData(symbols[i], mqlrates);
   }
}

// write the symbol data on file
void PreviewSymbol(string symbol, MqlRates &rates[]){
    // verbose some data 100 first
    string format="open = %G, high = %G, low = %G, close = %G, tickvolume = %d, realvolume = %G";
    string out;
    int rsize = ArraySize(rates);
    if(rsize == 0)
        return;
    for(int i=rsize-3; i<rsize; i++){ // print last 3 rates
       // you can better use the unix time number directly (unix time) and read it from python
        out=i+"  :  "+TimeToString(rates[i].time);
        out=out+" "+StringFormat(format,
            rates[i].open,
            rates[i].high,
            rates[i].low,
            rates[i].close,
            rates[i].tick_volume,
            rates[i].real_volume);
        Print(out);
    }
}

void WriteData(string symbol, MqlRates &arr[]){
    int n = ArraySize(arr);
//--- open the file
   ResetLastError();
   StringAdd(symbol,"M1.mt5bin");
   int handle=FileOpen(symbol, FILE_READ|FILE_WRITE|FILE_BIN);
   if(handle!=INVALID_HANDLE)
     {
      //--- write array data to the end of the file
      //FileSeek(handle,0,SEEK_END);
      FileWriteArray(handle, arr, 0, n);

      //--- close the file
      FileClose(handle);
     }
   else
      Print("Failed to open the file, error ",GetLastError());
  }

  // then you read from python with
  // arr = np.fromfile('data.bin', dtype='<f4')  // that is due to the fact that float convetion is plataform specific
