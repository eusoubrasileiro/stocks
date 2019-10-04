#property script_show_inputs

// in million R$
input const double MoneyBarSize = 0.5; 

input const string InputSymbol= "WINV19";

// this is ignored must used tickcount
// or use 0 to get the last tickcount
// input datetime Begin=D'2019.01.01 00:00'; 

// Download Money bars created from ticks time for specified InputSymbols
#include "..\datastruct\Bars.mqh"
#include "..\datastruct\Ticks.mqh"

//--- global variables
const uint tickcount=UINT_MAX>>8; //  number of ticks to copy / more than this explodes memory

void OnStart(){
   MqlTick mqlticks[];
   MoneyBar bars[];
   ArraySetAsSeries(mqlticks, true);
   int copied = 0;
   double moneybarsize = MoneyBarSize*1e6; // to make it million R$

    // number of 3 trials of download before giving up
   for(int try=0; try<3; try++) {
      // -1 if it has not complet it yet
      copied = CopyTicks(InputSymbol, mqlticks, COPY_TICKS_ALL, 0, tickcount);
      // CopyTicks
      Print("copied ", string(copied));
   } // ticks are allways returned no matter how few
   if(copied == 0)
   {
      Print("Failed to get history data for the InputSymbol ", InputSymbol);
      return;
   }
   Print("InputSymbol ", InputSymbol, " ticks downloaded: ", string(copied));
   PreviewSymbol(mqlticks);
   FixArrayTicks(mqlticks);
   int nbars = MoneyBars(mqlticks, SymbolInfoDouble(InputSymbol, SYMBOL_TRADE_TICK_VALUE),
          moneybarsize, bars);
   Print("InputSymbol ", InputSymbol, " MoneyBars formed: ", string(nbars));
   WriteData(InputSymbol, bars);

}

// write the InputSymbol data on file
void PreviewSymbol(MqlTick &rates[]){
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
   StringAdd(symbol, "_"+MoneyBarSize+"_mbar.bin");
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
