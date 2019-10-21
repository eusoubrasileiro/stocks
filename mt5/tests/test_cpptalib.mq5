#property copyright "Andre L. Ferreira 2018"
#property version   "1.00"
#include "Tests.mqh"

#import "ctalib.dll"
    int  taMA(int  startIdx, // start the calculation at index
	          int    endIdx, // end the calculation at index
	          const double &inReal[],
	          int   optInTimePeriod, // From 1 to 100000  - EMA window
	          int   optInMAType,
              double        &outReal[]);
    int  taSTDDEV(int  startIdx, // start the calculation at index
	          int    endIdx, // end the calculation at index
	          const double &inReal[],
	          int   optInTimePeriod, // From 1 to 100000  - EMA window
            double        &outReal[]);
#import

//ENUM_DEFINE(TA_MAType_SMA, Sma) = 0,
//ENUM_DEFINE(TA_MAType_EMA, Ema) = 1,
//ENUM_DEFINE(TA_MAType_WMA, Wma) = 2,
//ENUM_DEFINE(TA_MAType_DEMA, Dema) = 3,
//ENUM_DEFINE(TA_MAType_TEMA, Tema) = 4,
//ENUM_DEFINE(TA_MAType_TRIMA, Trima) = 5,
//ENUM_DEFINE(TA_MAType_KAMA, Kama) = 6,
//ENUM_DEFINE(TA_MAType_MAMA, Mama) = 7,
//ENUM_DEFINE(TA_MAType_T3, T3) = 8
void test_taMA(){
	double in[] = { 1, 1, 2, 3, 4, 5., 5 };
	int window = 3;
	double out[10];

	int size = taMA(0, 7, in, 3, 0, out);	// 0 simple moving average 1 EMA
	if (size == 5 && out[0] == 4./3 && out[size-1] == 14./3)
		Print("Passed - Test TA_Ma");
	else
		Print("Failed - Test TA_Ma");

	size = taMA(0, 7, in, 2, 0, out);
	if (size == 6 && out[0] == 1 && out[size - 1] == 5)
		Print("Passed - Test TA_Ma");
	else
		Print("Failed - Test TA_Ma");
}

void test_taSTDEV(){
  double in[] = {0.9759208 , 0.18005685, 0.20485216, 0.83701095, 0.97673102,
         0.92153903, 0.69225104, 0.04562335, 0.78740032, 0.137341 };
  double py_truth[] = {0.36946825, 0.30401539, 0.33581467,
       0.05745805, 0.12317467, 0.37087414, 0.32954747, 0.33018889};
	int window = 3;
	double out[8];

	int size = taSTDDEV(0, 10, in, 3, out);	
	if (almostEqual(out, py_truth, 8, 1e-4))
		Print("Passed - Test taSTDEV");
	else
		Print("Failed - Test taSTDEV");
}

void OnStart(){
    test_taMA();
    test_taSTDEV();
}
