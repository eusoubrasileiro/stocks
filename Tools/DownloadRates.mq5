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
string symbols[8] = {"BBDC4", "DOL$", "VALE3", "BBAS3", "PETR4",  "ABEV3", "BVMF3", "ITUB4"};
const int minutes=2147483647; //maximum value 2<<31 - 1

void OnStart(){ 
   MqlRates mqlrates[]; 
   ArraySetAsSeries(mqlrates, true); 
   int nsymbols = ArraySize(symbols);
   int copied;
   
   for(int i=0; i<nsymbols; i++)
   {
       for(int try=0; try<3; try++) // number of 3 trials of download before giving up
       {
          // -1 if it has not complet it yet
          copied = CopyRates(symbols[i], PERIOD_M1, 0, minutes, mqlrates);
          Print("copied ", string(copied));
          if(copied < minutes) //  sleep time(15 seconds) for downloading the data
              Sleep(10000);               
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
  
// write the symbol data on file 
void WriteSymbol(string symbol, MqlRates &rates[]){
    // verbose some data 100 first
    string format="open = %G, high = %G, low = %G, close = %G, tickvolume = %d, realvolume = %G"; 
    string out;     
    
    for(int i=0; i<100; i++) 
    { 
    // you can better use the unix time number directly (unix time) and read it from python
        out=i+":"+TimeToString(rates[i].time);  
        out=out+" "+StringFormat(format, 
            rates[i].open, 
            rates[i].high, 
            rates[i].low, 
            rates[i].close, 
            rates[i].tick_volume,
            rates[i].real_volume); 
        Print(out); 
    }  

    WriteData(symbol, rates); 
} 

void WriteData(string symbol, MqlRates &arr[]){
    int n = ArraySize(arr); 
//--- open the file 
   ResetLastError(); 
   StringAdd(symbol,".mt5bin");
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
