#include "Tests.mqh"
#include "..\indicators\CTalibIndicators.mqh"

void test_CTaMAIndicator(){
	double in[] = {1, 1, 2, 3, 4, 5, 5, 3, 1};
	double ta_truth[] = { 1. , 1.5, 2.5, 3.5, 4.5, 5. , 4. , 2.};
	int size = 8;
	int window = 2;
	double out[10];
	CTaMAIndicator cta_MA(window, 0);	 // simple MA
	cta_MA.Resize(10);

	// partial calls until total array calculated
	double a[] = {1, 1};
	double b[] = {2, 3};
	double c[] = {4, 5, 5};
	double d[] = {3, 1};
	cta_MA.Refresh(a);
	cta_MA.Refresh(b);
	cta_MA.Refresh(c);
	cta_MA.Refresh(d);

	if(!almostEqual(ta_truth, cta_MA.m_data, 8, 1e-6))
	    Print("Failed - Test CTaMAIndicator");
	else
	    Print("Passed - Test CTaMAIndicator");
}

void test_CTaBBANDSIndicator(){
	double in[] = {1, 1, 2, 3, 4, 5, 5, 3, 1};
	double ta_truth_upper[] = { 1.  , 2.75, 3.75, 4.75, 5.75, 5.  , 6.5 , 4.5 };
	double ta_truth_middle[] = { 1. , 1.5, 2.5, 3.5, 4.5, 5. , 4. , 2.};
	double ta_truth_down[] = { 1.  ,  0.25,  1.25,  2.25,  3.25,  5.  ,  1.5 , -0.5};
	
	int size = 8;
	int window = 2;
	double out[10];
	CTaBBANDS cta_BBANDS(window, 2.5, 0); // simple MA + 2.5 deviations
    cta_BBANDS.Resize(10);
    
	// partial calls until total array calculated
	double a[] = {1, 1};
	double b[] = {2, 3};
	double c[] = {4, 5, 5};
	double d[] = {3, 1};
	cta_BBANDS.Refresh(a);
	cta_BBANDS.Refresh(b);
	cta_BBANDS.Refresh(c);
	cta_BBANDS.Refresh(d);	

	if(!almostEqual(ta_truth_upper, cta_BBANDS.m_upper.m_data, 8, 1e-6) ||
	   !almostEqual(ta_truth_middle, cta_BBANDS.m_middle.m_data, 8, 1e-6) ||
	   !almostEqual(ta_truth_down, cta_BBANDS.m_down.m_data, 8, 1e-6) )
	    Print("Failed - Test CTaBBANDSIndicator");
	else
	    Print("Passed - Test CTaBBANDSIndicator");
}


void OnStart(){
    test_CTaMAIndicator();
    test_CTaBBANDSIndicator();
    CTaSTDDEV cta_STDDEV(10);
}
