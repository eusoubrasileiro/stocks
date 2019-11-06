//+------------------------------------------------------------------+
//|                                                      ProjectName |
//|                                      Copyright 2012, CompanyName |
//|                                       http://www.companyname.net |
//+------------------------------------------------------------------+
#property copyright "Andre L. Ferreira 2018"
#property version   "1.00"
#include "..\Buffers.mqh"
#include "Tests.mqh"

void test_CBuffer(){
  CBuffer<double> sbuffer();
  sbuffer.Resize(4);
  for(int i=0; i<12; i++) // minimum buffer size is 16
       sbuffer.Add(0);
  for(int i=1; i<7; i++) // exceeds the buffer
   sbuffer.Add(i);

  if (sbuffer.GetData(0) == 6 &&  sbuffer.GetData(1) == 5 &&
       sbuffer.GetData(2) == 4 && sbuffer.GetData(3) == 3)
      Print("Passed - Test CBuffer");
  else
      Print("Failed - Test CBuffer");
}

void test_CCBuffer_Add(){
  CCBuffer<double> sbuffer(16); // buffer size 16

  for(int i=0; i<12; i++)
       sbuffer.Add(0);
  for(int i=1; i<7; i++) // exceeds the buffer
   sbuffer.Add(i);

  if(sbuffer[0] == 5 &&  sbuffer[1] == 6 &&
       sbuffer[14] == 3 && sbuffer[15] == 4)
      Print("Passed - Test CCBuffer Add");
  else
      Print("Failed - Test CCBuffer Add");
}

void test_CCBuffer_Indexes_Data(){
  CCBuffer<double> sbuffer(5); // buffer size 5

  for(int i=0; i<3; i++)
      sbuffer.Add(0);
  for(int i=1; i<5; i++) // exceeds the buffer
      sbuffer.Add(i);

   int start1, start2, end1, end2;
   double dest[4];
   int parts, count;

   // get 4 elements
   // beginning at the first 0
   parts = sbuffer.indexesData(0, 4, start1, end1, start2, end2);  // get 4 elements

   count = end1-start1;
   ArrayCopy(dest, sbuffer.m_data, 0, start1, count);
   ArrayCopy(dest, sbuffer.m_data, count, start2, end2-start2);
   double assert1[4] = {0, 1, 2, 3};
   if(parts == 2 && almostEqual(assert1, dest, 4, 1e-8))
      Print("Passed - Test CCBuffer indexesData 1");
   else
      Print("Failed - Test CCBuffer indexesData 1");

   // get 1 element
   // beginning at the 4th 
   parts = sbuffer.indexesData(4, 1, start1, end1, start2, end2);  

   count = end1-start1;
   ArrayCopy(dest, sbuffer.m_data, 0, start1, count);
   ArrayCopy(dest, sbuffer.m_data, count, start2, end2-start2);
   double assert2[1] = {4};
   if(parts == 1 && almostEqual(assert2, dest, 1, 1e-8))
       Print("Passed - Test CCBuffer indexesData 2");
   else
       Print("Failed - Test CCBuffer indexesData 2");

   // get 2 elements
   // beginning at 0 
   parts = sbuffer.indexesData(0, 3, start1, end1, start2, end2);  

   count = end1-start1;
   ArrayCopy(dest, sbuffer.m_data, 0, start1, count);
   ArrayCopy(dest, sbuffer.m_data, count, start2, end2-start2);
   double assert3[3] = {0, 1, 2};
   if(parts == 1 && almostEqual(assert3, dest, 1, 1e-8))
       Print("Passed - Test CCBuffer indexesData 3");
   else
       Print("Failed - Test CCBuffer indexesData 3");       
   
}

void OnStart()
{
    test_CBuffer();
    test_CCBuffer_Add();
    test_CCBuffer_Indexes_Data();
}
