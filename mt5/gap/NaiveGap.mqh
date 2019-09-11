#include <Arrays\ArrayDouble.mqh>

#import "cpparm.dll"
int Unique(double &arr[], int n);
// bins specify center of bins
// int  Histsma(double &data[], int n,  double &bins[], int nb, int nwindow);
#import

int classic_pivotPoints(MqlRates &rates[], double &pivots[]){
    // S3 and R3 are too low or too high normally never reached I found
    int size = ArraySize(rates);
    ArrayResize(pivots, size*4);
    double pivot;

    // for every day in mqlrate calculate the pivot points
    for(int i=0; i<size; i++){
        pivot = (rates[i].high + rates[i].low + rates[i].close)/3;
        pivots[i*4] = (pivot*2 - rates[i].low); // R1
        pivots[i*4+1] = (pivot + rates[i].high - rates[i].low); // R2
        pivots[i*4+2] = (pivot*2 - rates[i].high); // S1
        pivots[i*4+3] = (pivot - rates[i].high + rates[i].low); // S2
    }

    ArraySort(pivots);
    size = ArraySize(pivots);
    return size;
}

// R3 = PP + ((High – Low) x 1.000)
// R2 = PP + ((High – Low) x .618)
// R1 = PP + ((High – Low) x .382)
// PP = (H + L + C) / 3
// S1 = PP – ((High – Low) x .382)
// S2 = PP – ((High – Low) x .618)
// S3 = PP – ((High – Low) x 1.000)
int fibonacci_pivotPoints(MqlRates &rates[], double &pivots[]){
    // S3 and R3 are too low or too high normally never reached I found
    int size = ArraySize(rates);
    ArrayResize(pivots, size*5);
    double pivot, range;

    // for every day in mqlrate calculate the pivot points
    for(int i=0; i<size; i++){
        pivot = (rates[i].high + rates[i].low + rates[i].close)/3;
        range = (rates[i].high - rates[i].low);
        pivots[i*4] = (pivot - range*1); // S3
        pivots[i*4+1] = (pivot - range*0.618); // S2
        pivots[i*4+2] = (pivot + range*0.618); // R2
        pivots[i*4+3] = (pivot + range*1); // R3
    }

    ArraySort(pivots);
    size = ArraySize(pivots);

    return size;
}

// camarilla is the only one using the pivot point as an pivot value
int camarilla_pivotPoints(MqlRates &rates[], double &pivots[]){
    // S3 and R3 are too low or too high normally never reached I found
    int size = ArraySize(rates);
    ArrayResize(pivots, size*5);
    double pivot, range;

    // for every day in mqlrate calculate the pivot points
    for(int i=0; i<size; i++){
        pivot = ((rates[i].high + rates[i].low + rates[i].close)/3);
        range = ((rates[i].high - rates[i].low));
        //pivots[i*5] = rates[i].close - range*(1.1/12); // S1
        //pivots[i*5+1] = rates[i].close - range*(1.1/6); // S2
        pivots[i*5] = (rates[i].close - range*(1.1/4)); // S3
        pivots[i*5+1] = (rates[i].close - range*(1.1/2)); // S4
//        pivots[i*5+2] = rates[i].close + range*(1.1/12); // R1
//        pivots[i*5+3] = rates[i].close + range*(1.1/6); // R2
        pivots[i*5+2] = (pivot);
        pivots[i*5+3] = (rates[i].close + range*(1.1/4)); // R3
        pivots[i*5+4] = (rates[i].close + range*(1.1/2)); // R4
    }

    ArraySort(pivots);
    size = ArraySize(pivots);

    return size;
}

// Camarilla
// Define: Range = High - Low
// Pivot Point (P) = (High + Low + Close) / 3
// Support 1 (S1) = Close - Range * (1.1 / 12)
// Support 2 (S2) = Close - Range * (1.1 / 6)
// Support 3 (S3) = Close - Range * (1.1 / 4)
// Support 4 (S4) = Close - Range * (1.1 / 2)
// Support 5 (S5) = Close - (R5 - Close)
// Resistance 1 (R1) = Close + Range * (1.1 / 12)
// Resistance 2 (R2) = Close + Range * (1.1 / 6)
// Resistance 3 (R3) = Close + Range * (1.1 / 4)
// Resistance 4 (R4) = Close + Range * (1.1 / 2)
// Resistance 5 (R5) = (High/Low) * Close
