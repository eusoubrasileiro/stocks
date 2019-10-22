//+------------------------------------------------------------------+
//|                                                      ProjectName |
//|                                      Copyright 2012, CompanyName |
//|                                       http://www.companyname.net |
//+------------------------------------------------------------------+
#property copyright "Andre L. Ferreira 2018"
#property version   "1.00"
#include "..\Buffers.mqh"

//+------------------------------------------------------------------+
//|                                                                  |
//+------------------------------------------------------------------+
void OnStart()
  {   
   CBuffer<double> sbuffer();
   sbuffer.Resize(4);
   for(int i=0; i<12; i++) // minimum buffer size is 16
        sbuffer.Add(0);
   for(int i=1; i<7; i++) // exceeds the buffer
    sbuffer.Add(i);
 
   if ( sbuffer.GetData(0) == 6 &&  sbuffer.GetData(1) == 5 &&
        sbuffer.GetData(2) == 4 && sbuffer.GetData(3) == 3 )
        Print("Passed - Test CBuffer");    
   else
        Print("Failed - Test CBuffer");  
  }
//+------------------------------------------------------------------+
