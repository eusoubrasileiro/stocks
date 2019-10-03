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
string symbols[11] = {"WDO@", "WIN@", "BBDC4", "DOL$", "VALE3", "BBAS3", "PETR4",  "ABEV3", "B3SA3", "ITUB4", "WEGE3"};
const uint tickcount=UINT_MAX>>4; //  number of ticks to copy / more than this explodes memory
datetime date=D'2019.01.01 00:00'; // or use 0 to get the last tickcount
int file_io_hnd;

void OnStart(){
   MqlTick mqlticks[];
   ArraySetAsSeries(mqlticks, true);
   int nsymbols = ArraySize(symbols);
   uint copied = 0;

   for(int i=0; i<nsymbols; i++)
   { // number of 3 trials of download before giving up
       for(int try=0; try<3; try++) {
          // -1 if it has not complet it yet
          copied = CopyTicks(symbols[i], mqlticks, COPY_TICKS_ALL,  date, tickcount);
          // CopyTicks
          Print("copied ", string(copied));
       } // ticks are allways returned no matter how few

       Print("ticks downloaded: ", string(copied));
       WriteSymbol(symbols[i], mqlticks);
   }
}

// write the symbol data on file
void WriteSymbol(string symbol, MqlTick &rates[]){
    // verbose some data 100 first
    string format = "bid = %G, ask = %G, last = %G, volume = %d , flags = %d, volume real = %d";
    string out;

    for(int i=0; i<5; i++)
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
//--- open the file
   ResetLastError();
   StringAdd(symbol,"Tick.mt5bin");
   int handle=FileOpen(symbol, FILE_READ|FILE_WRITE|FILE_BIN);
   int size = ArraySize(arr);
   if(handle!=INVALID_HANDLE)
     {
     
      int wrote = 0;
      if(size > 5e5){ // write in chuncks
        int i, chunck = 5e5; // 500k ticks per time
        for(i=0; i < size; i+= wrote){
            wrote = FileWriteArray(handle, arr, i, chunck);
        }
        // write the rest
        if( i < size)
            FileWriteArray(handle, arr, i, size-i);        
      }
      else
        //--- write array data once
        wrote = FileWriteArray(handle, arr, 0, WHOLE_ARRAY);
      //--- close the file
      FileClose(handle);
     }
   else
      Print("Failed to open the file, error ",GetLastError());
  }

  // then you read from python with
  // arr = np.fromfile('data.bin', dtype='<f4')  // that is due to the fact that float convetion is plataform specific
