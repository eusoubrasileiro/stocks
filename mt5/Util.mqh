#include "Expert.mqh"
#include "TrailingMA.mqh"
#include <Arrays\ArrayDouble.mqh>
#include <Trade\Trade.mqh>

#import "cpparm.dll"
int Unique(double &arr[],int n);
// bins specify center of bins
int  Histsma(double &data[],int n,double &bins[],int nb,int nwindow);
#import

#import "msvcrt.dll"
  int memcpy(double &dst[],  double &src[], int cnt);  
#import

// calculate begin of the day Zero Hour
// TODO OPTIMIZE it to make use of the 8 bytes
// UNIX timestamp in seconds since 1970 only 8 bytes
datetime GetDayZeroHour(datetime time){ // can be a unique day identifier
    MqlDateTime mqltime;
    TimeToStruct(time, mqltime);
    return time - (mqltime.hour*3600+mqltime.min*60+mqltime.sec);
}

// utils for sorted arrays
int searchGreat(double &arr[], double value){
    int size = ArraySize(arr);
    for(int i=0; i<size; i++)
        if(arr[i] > value)
            return i;
    return -1;
}

// Search for a value smaller than value in a sorted array
int searchLess(double &arr[], double value){
    int size = ArraySize(arr);
    for(int i=size-1; i>=0; i--) // start by the end
        if(arr[i] < value)
            return i;
    return -1;
}


int arange(double start, double stop, double step, double &arr[]){
    int size = MathFloor((stop-start)/step)+1;
    ArrayResize(arr, size);
    for(int i=0; i<size; i++)
        arr[i] = start + step*i;
    return size;
}

  // a naive histogram were empty bins are not taken in account
  // prices must be sorted
  // prices will be rounded to tickSize to define bins
  // empty bins will not be taken in account
int naivehistPriceTicksize(double &prices[], double &bins[], int &count_bins[]){
  int nprices = ArraySize(prices);
  // bins[] bins with unique price values
  // round prices to tickSize
  for(int i=0; i<nprices; i++)
    prices[i] = prices[i];

  ArrayCopy(bins, prices); // 'Unique' armadillo overwrites input array
  int nbins = Unique(bins, nprices); // get unique price values
  ArrayResize(bins, nbins); // bins based on unique values
  ArrayResize(count_bins, nbins);

  // naive histogram of counts pretty fast because prices is sorted
  // and rounded
  for(int i=0; i<nbins; i++){
      count_bins[i] = 0;
      for(int j=0; j<nprices; j++)
          if(prices[j] == bins[i])
              count_bins[i] += 1;
  }
  return nbins;
}

double percentile(double &data[], double perc){
    double sorted[];
    ArrayCopy(sorted, data, 0, 0, WHOLE_ARRAY);
    int asize = ArraySize(sorted);
    ArraySort(sorted);
    int n = MathMax(MathRound(perc * asize + 0.5), 2);
    return sorted[n-2];
}

bool IsEqualMqldt(MqlDateTime &mqldt_a, MqlDateTime &mqldt_b)
{
     if(mqldt_a.year==mqldt_b.year && mqldt_a.mon==mqldt_b.mon && mqldt_a.day==mqldt_b.day &&
            mqldt_a.hour==mqldt_b.hour && mqldt_a.min==mqldt_b.min && mqldt_a.sec==mqldt_b.sec)
           return true;
    return false;
}

bool IsEqualMqldt_M1(MqlDateTime &mqldt_a, MqlDateTime &mqldt_b)
{
     if(mqldt_a.year==mqldt_b.year && mqldt_a.mon==mqldt_b.mon && mqldt_a.day==mqldt_b.day &&
            mqldt_a.hour==mqldt_b.hour && mqldt_a.min==mqldt_b.min)
           return true;
    return false;
}

// // returns the fractdif coeficients
// // $$ \omega_{k} = -\omega_{k-1}\frac{d-k+1}{k} $$
// // output is allocated inside
// void FracDifCoefs(double d, int size, double &w[]){
//     ArrayResize(w, size);
//     w[0] = 1.;
//     for(int k=1; k<size; k++)
//         w[k]=-w[k-1]/k*(d-k+1);
//     ArrayReverse(w);
// }
//
// // apply fracdif filter on signal array
// // FracDifCoefs must be supplied
// // output is allocated inside
// int FracDifApply(double &signal[], int ssize, double &fracdifcoefs[], int fsize, double &output[]){
//     int corrsize, outsize;
//     corrsize = ssize + fsize - 1;
//     outsize = ssize - fsize + 1; // == corrsize-(fsize-1)*2
//     ArrayResize(output, corrsize);
//     CCorr::CorrR1D(signal, ssize, fracdifcoefs, fsize, output);
//     ArrayCopy(output, output, 0, 0, outsize);
//     ArrayResize(output, outsize);
//     return outsize;
// }
