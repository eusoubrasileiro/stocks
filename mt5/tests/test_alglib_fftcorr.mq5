#include <Math/Alglib/fasttransforms.mqh>

//+------------------------------------------------------------------+
//| 1-dimensional real cross-correlation.                            |
//| For given Pattern/Signal returns corr(Pattern,Signal)            |
//| (non-circular).                                                  |
//| Correlation is calculated using reduction to convolution.        |
//| Algorithm with max(N,N)*log(max(N,N)) complexity is used (see    |
//| ConvC1D() for more info about performance).                      |
//| IMPORTANT:                                                       |
//|     for  historical reasons subroutine accepts its parameters in |
//|     reversed order: CorrR1D(Signal, Pattern) = Pattern x Signal  |
//|     (using  traditional definition of cross-correlation, denoting|
//|     cross-correlation as "x").                                   |
//| INPUT PARAMETERS                                                 |
//|     Signal  -   array[0..N-1] - real function to be transformed, |
//|                 signal containing pattern                        |
//|     N       -   problem size                                     |
//|     Pattern -   array[0..M-1] - real function to be transformed, |
//|                 pattern to search withing signal                 |
//|     M       -   problem size                                     |
//| OUTPUT PARAMETERS                                                |
//|     R       -   cross-correlation, array[0..N+M-2]:              |
//|                 * positive lags are stored in R[0..N-1],         |
//|                   R[i] = sum(pattern[j]*signal[i+j]              |
//|                 * negative lags are stored in R[N..N+M-2],       |
//|                   R[N+M-1-i] = sum(pattern[j]*signal[-i+j]       |
//| NOTE:                                                            |
//|     It is assumed that pattern domain is [0..M-1]. If Pattern is |
//| non-zero on [-K..M-1],  you can still use this subroutine, just  |
//| shift result by K.                                               |
//+------------------------------------------------------------------+
//static void CCorr::CorrR1D(double &signal[],const int n,double &pattern[],
//                           const int m,double &r[])

void test_fftcorr(){
    double in[] = {1, 1, 2, 1, 1};
    double pattern[] = {1, 2 , 1};
    double res[10];
    
    CCorr::CorrR1D(in, 5, pattern, 3, res);
    for(int i; i<10; i++)
        PrintFormat("%f", res[i]);
}



void OnStart(){
    test_fftcorr();
}