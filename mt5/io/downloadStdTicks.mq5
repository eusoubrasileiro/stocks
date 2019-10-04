// Download standard ticks time for specified symbols
// write on a binary file the mqlrates array
#property copyright "Andre L. Ferreira June 2018"
#property version   "1.00"
#property script_show_inputs
input datetime date=D'2019.01.01 00:00'; // or use 0 to get the last tickcount

#include "..\datastruct\Bars.mqh"
#include "..\datastruct\Ticks.mqh"

//--- global variables
string symbols[4] = {"WINV19", "VALE3", "PETR4", "WEGE3"};
const uint tickcount=UINT_MAX>>8; //  number of ticks to copy / more than this explodes memory

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
       if(copied == -1)
       {
          Print("Failed to get history data for the symbol ", symbols[i]);
          continue;
       }
       Print("symbol ", symbols[i], " ticks downloaded: ", string(copied));
       PreviewSymbol(symbols[i], mqlticks);
       FixArrayTicks(mqlticks);
       WriteData(symbols[i], mqlticks);
   }
}

// write the symbol data on file
void PreviewSymbol(string symbol, MqlTick &rates[]){
    // verbose some data 100 first
    string format = "bid = %G, ask = %G, last = %G, volume = %d , ms = %d, flags = %u, volume real = %d";
    string out;
    int rsize = ArraySize(rates);
    if(rsize == 0)
        return;
    for(int i=rsize-3; i<rsize; i++) // print last 3 ticks
    {
    // you can better use the unix time number directly (unix time) and read it from python
        out= i+"  :  "+TimeToString(rates[i].time);
        out=out+" "+StringFormat(format,
            rates[i].bid,
            rates[i].ask,
            rates[i].last,
            rates[i].volume,
            rates[i].time_msc, //not printing
            rates[i].flags,
            rates[i].volume_real);
        Print(out);
    }
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
