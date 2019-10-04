#property script_show_inputs
input const double MoneyBarSize = 100e6; // 1M R$

// Download Money bars created from ticks time for specified symbols
#include "..\datastruct\Bars.mqh"

//--- global variables
string symbols[11] = {"WDO@", "WIN@", "BBDC4", "DOL$", "VALE3", "BBAS3", "PETR4",  "ABEV3", "B3SA3", "ITUB4", "WEGE3"};
const uint tickcount=UINT_MAX>>8; //  number of ticks to copy / more than this explodes memory
datetime date=D'2019.01.01 00:00'; // or use 0 to get the last tickcount
int file_io_hnd;


void OnStart(){
   MqlTick mqlticks[];
   MoneyBar bars[];
   ArraySetAsSeries(mqlticks, true);
   int nsymbols = ArraySize(symbols);
   int copied = 0;

   for(int i=0; i<nsymbols; i++)
   { // number of 3 trials of download before giving up
       for(int try=0; try<3; try++) {
          // -1 if it has not complet it yet
          copied = CopyTicks(symbols[i], mqlticks, COPY_TICKS_ALL,  date, tickcount);
          // CopyTicks
          Print("copied ", string(copied));
       } // ticks are allways returned no matter how few
       if(copied == 0)
       {
          Print("Failed to get history data for the symbol ", symbols[i]);
          continue;
       }
       Print("symbol ", symbols[i], " ticks downloaded: ", string(copied));
       PreviewSymbol(symbols[i], mqlticks);
       int nbars = MoneyBars(mqlticks, SymbolInfoDouble(symbols[i], SYMBOL_TRADE_TICK_VALUE),
              MoneyBarSize, bars);
       WriteData(symbols[i], bars);
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

void WriteData(string symbol, MoneyBar &arr[]){
//--- open the file
   ResetLastError();
   StringAdd(symbol,"mbar.bin");
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
