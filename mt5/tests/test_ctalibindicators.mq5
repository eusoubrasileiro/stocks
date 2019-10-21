#include "Tests.mqh"
#include "..\indicators\CTalibIndicators.mqh"

void test_CTaMAIndicator(){
	double in[] = { 1, 1, 2, 3, 4, 5., 5, 3};
	int size = 8;
	int window = 2;
	double out[10];	
	CTaMAIndicator cta_MA = new CTaMAIndicator;
	cta_MA.setParams(window, 0); // simple MA
	
	// direct call for entire array
	int outsize = taMA(0, size, in, window, 0, out);	
	
	// partial calls until total array calculated
	// each call with 2 samples
	double tmp[2];	
	for(int i=0; i<size; i+=2){
	    ArrayCopy(tmp, in, 0, i, 2);
	    cta_MA.Refresh(tmp);	    
	}
	

}


void OnStart(){
    test_CTaMAIndicator();
}
