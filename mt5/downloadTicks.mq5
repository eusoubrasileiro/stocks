// Download 1 minute time frame data for specified symbols
// write on a binary file the mqlrates array
#property copyright "Andre L. Ferreira June 2018"
#property version   "1.00"
#property script_show_inputs
//--- input parameters
//input string InpFileName="data.bin";
//nput string InpDirectoryName="";

//--- global variables
int    count=0;
string symbols[9] = {"WIN@", "BBDC4", "DOL$", "VALE3", "BBAS3", "PETR4",  "ABEV3", "B3SA3", "ITUB4"};
const uint maxms=4294967296; //maximum value 2<<32 - 1 ~ 71.01% of a year
// msday = 7*60*60*1000 ms in  day
// msyear = 6048000000 ms in a year
datetime date=D'2018.01.01 00:00';

void OnStart(){
   MqlTick mqlticks[];
   ArraySetAsSeries(mqlticks, true);
   int nsymbols = ArraySize(symbols);
   uint copied = 0;

   for(int i=0; i<nsymbols; i++)
   { // number of 3 trials of download before giving up
       for(int try=0; try<3; try++) {
          // -1 if it has not complet it yet
          copied = CopyTicks(symbols[i], mqlticks, COPY_TICKS_ALL,  date, maxms);
          // CopyTicks
          Print("copied ", string(maxms));
          if(copied < maxms) //  sleep time(15 seconds) for downloading the data
              Sleep(10000);
       } // ticks are allways returned no matter how few
       
       Print("minutes downloaded: ", string(copied));
       WriteSymbol(symbols[i], mqlticks);
   }
}

// write the symbol data on file
void WriteSymbol(string symbol, MqlTick &rates[]){
    // verbose some data 100 first
    string format = "bid = %G, ask = %G, last = %G, volume = %d , flags = %d, volume real = %d";
    string out;

    for(int i=0; i<100; i++)
    {
    // you can better use the unix time number directly (unix time) and read it from python
        out=i+":"+TimeToString(rates[i].time);
        out=out+" "+StringFormat(format,
            rates[i].bid,
            rates[i].ask,
            rates[i].last,
            rates[i].volume,
//rates[i].time_msc, not printing
            rates[i].flags,
            rates[i].volume_real);
        Print(out);
    }

    WriteData(symbol, rates);
}

void WriteData(string symbol, MqlTick &arr[]){
    int n = ArraySize(arr);
//--- open the file
   ResetLastError();
   StringAdd(symbol,"Tick.mt5bin");
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
