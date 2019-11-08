// Download standard ticks time for specified symbol
// write on a binary file the MqlTick array
#property copyright "Andre L. Ferreira June 2018"
#property version   "1.00"
#property script_show_inputs
#include "..\datastruct\Bars.mqh"
#include "..\datastruct\Ticks.mqh"

input datetime start_date=D'2019.10.01 00:00'; // or use 0 to get the last tickcount
input datetime end_date=D'2019.10.25 00:00';

//--- global variables
input string input_symbol="PETR4";
const uint tickcount=UINT_MAX>>8; //  number of ticks to copy / more than this explodes memory

int file_io_hnd;

void OnStart(){
   MqlTick mqlticks[];
   ArraySetAsSeries(mqlticks, true);
   int copied = 0;
   
   ArrayResize(mqlticks, 100e6); // 100 MM

  // number of 3 trials of download before giving up
  for(int try=0; try<3; try++) {
    // -1 if it has not complet it yet
    copied = CopyTicksRange(input_symbol, mqlticks, COPY_TICKS_ALL, 
                (long) start_date*1000, (long) end_date*1000);
    // CopyTicks
    Print("copied ", string(copied));
  } // ticks are allways returned no matter how few
  if(copied == -1){
    Print("Failed to get history data for the symbol ", input_symbol);
    return;
  }
  Print("symbol ", input_symbol, " ticks downloaded: ", string(copied));
  PreviewSymbol(input_symbol, mqlticks);
  FixArrayTicks(mqlticks);
  WriteData(input_symbol, mqlticks);
  
  ArrayFree(mqlticks);

}

// write the symbol data on file
void PreviewSymbol(string symbol, MqlTick &rates[]){
    // verbose some data 100 first
    string format = " bid = %G, ask = %G, last = %G,"
                    " volume = %g, ms = %I64d, flags = %u,"
                    " volume real = %g";    
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
            (long) rates[i].time_msc, //not printing
            rates[i].flags,
            rates[i].volume_real);
        Print(out);
    }
}

void WriteData(string isymbol, MqlTick &arr[]){
//--- open the file
   ResetLastError();
   StringAdd(isymbol,"_mql5tick.bin");
   int handle=FileOpen(isymbol, FILE_READ|FILE_WRITE|FILE_BIN);
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
      Print("Failed to open the file, error ", GetLastError());
  }

  // then you read from python with
  // arr = np.fromfile('data.bin', dtype='<f4')  // that is due to the fact that float convetion is plataform specific
