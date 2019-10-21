#include "ta_libc.h"
#include "exports.h"
#include <malloc.h>
#include <iostream>

using namespace std;



// Talib dependency is worth?
// The list is short and
// maybe writing my own c++ code is easier and faster ...
// AD                  Chaikin A/D Line
// ADOSC               Chaikin A/D Oscillator
// ADX                 Average Directional Movement Index
// ADXR                Average Directional Movement Index Rating
// APO                 Absolute Price Oscillator
// AROON               Aroon
// AROONOSC            Aroon Oscillator
// ATR                 Average True Range
// AVGPRICE            Average Price
// BBANDS              Bollinger Bands
// BETA                Beta
// BOP                 Balance Of Power
// CCI                 Commodity Channel Index
// CMO                 Chande Momentum Oscillator
// CORREL              Pearson's Correlation Coefficient (r)
// DEMA                Double Exponential Moving Average
// DX                  Directional Movement Index
// EMA                 Exponential Moving Average
// HT_DCPERIOD         Hilbert Transform - Dominant Cycle Period
// HT_DCPHASE          Hilbert Transform - Dominant Cycle Phase
// HT_PHASOR           Hilbert Transform - Phasor Components
// HT_SINE             Hilbert Transform - SineWave
// HT_TRENDLINE        Hilbert Transform - Instantaneous Trendline
// HT_TRENDMODE        Hilbert Transform - Trend vs Cycle Mode
// KAMA                Kaufman Adaptive Moving Average
// LINEARREG           Linear Regression
// LINEARREG_ANGLE     Linear Regression Angle
// LINEARREG_INTERCEPT Linear Regression Intercept
// LINEARREG_SLOPE     Linear Regression Slope
// MA                  All Moving Average
// MACD                Moving Average Convergence/Divergence
// MACDEXT             MACD with controllable MA type
// MACDFIX             Moving Average Convergence/Divergence Fix 12/26
// MAMA                MESA Adaptive Moving Average
// MAX                 Highest value over a specified period
// MAXINDEX            Index of highest value over a specified period
// MEDPRICE            Median Price
// MFI                 Money Flow Index
// MIDPOINT            MidPoint over period
// MIDPRICE            Midpoint Price over period
// MIN                 Lowest value over a specified period
// MININDEX            Index of lowest value over a specified period
// MINMAX              Lowest and highest values over a specified period
// MINMAXINDEX         Indexes of lowest and highest values over a specified period
// MINUS_DI            Minus Directional Indicator
// MINUS_DM            Minus Directional Movement
// MOM                 Momentum
// NATR                Normalized Average True Range
// OBV                 On Balance Volume
// PLUS_DI             Plus Directional Indicator
// PLUS_DM             Plus Directional Movement
// PPO                 Percentage Price Oscillator
// ROC                 Rate of change : ((price/prevPrice)-1)*100
// ROCP                Rate of change Percentage: (price-prevPrice)/prevPrice
// ROCR                Rate of change ratio: (price/prevPrice)
// ROCR100             Rate of change ratio 100 scale: (price/prevPrice)*100
// RSI                 Relative Strength Index
// SAR                 Parabolic SAR
// SAREXT              Parabolic SAR - Extended
// SMA                 Simple Moving Average
// STDDEV              Standard Deviation
// STOCH               Stochastic
// STOCHF              Stochastic Fast
// STOCHRSI            Stochastic Relative Strength Index
// SUM                 Summation
// T3                  Triple Exponential Moving Average (T3)
// TEMA                Triple Exponential Moving Average
// TRANGE              True Range
// TRIMA               Triangular Moving Average
// TRIX                1-day Rate-Of-Change (ROC) of a Triple Smooth EMA
// TSF                 Time Series Forecast
// TYPPRICE            Typical Price
// ULTOSC              Ultimate Oscillator
// VAR                 Variance
// WCLPRICE            Weighted Close Price
// WILLR               Williams' %R
// WMA                 Weighted Moving Average


//ENUM_DEFINE(TA_MAType_SMA, Sma) = 0,
//ENUM_DEFINE(TA_MAType_EMA, Ema) = 1,
//ENUM_DEFINE(TA_MAType_WMA, Wma) = 2,
//ENUM_DEFINE(TA_MAType_DEMA, Dema) = 3,
//ENUM_DEFINE(TA_MAType_TEMA, Tema) = 4,
//ENUM_DEFINE(TA_MAType_TRIMA, Trima) = 5,
//ENUM_DEFINE(TA_MAType_KAMA, Kama) = 6,
//ENUM_DEFINE(TA_MAType_MAMA, Mama) = 7,
//ENUM_DEFINE(TA_MAType_T3, T3) = 8

// you can use this way also (buffer used as input and output)
// TA_MA(0, BUFFER_SIZE - 1,
// &buffer[0],
//	30, TA_MAType_SMA,
// 	&outBeg, &outNbElement, &buffer[0]);
int DLL_EXPORT taMA(int  startIdx, // start the calculation at index
					int    endIdx, // end the calculation at index
					const double inReal[],
					int           optInTimePeriod, // From 1 to 100000  - EMA window
					int     optInMAType,
					double        outReal[])
{
  int outBegIdx; // index based on the input array inReal where we start having valid MA values
  int outNBElement; // number of elements on the output discounting for initial window values needed
	TA_MA(startIdx, endIdx, inReal,
		optInTimePeriod, (TA_MAType) optInMAType,
		&outBegIdx, &outNBElement, outReal);
	return outNBElement-1;
}


// you can use this way also (buffer used as input and output)
// TA_MA(0, BUFFER_SIZE - 1,
// &buffer[0],
//	30, TA_MAType_SMA,
// 	&outBeg, &outNbElement, &buffer[0]);
int DLL_EXPORT taSTDDEV(int  startIdx, // start the calculation at index
					int    endIdx, // end the calculation at index
					const double inReal[],
					int           optInTimePeriod, // From 1 to 100000  - EMA window
					double        outReal[])
{
  int outBegIdx; // index based on the input array inReal where we start having valid MA values
  int outNBElement; // number of elements on the output discounting for initial window values needed
	TA_STDDEV(startIdx, endIdx, inReal,
		optInTimePeriod, 1., // number to multiply stddev to
		&outBegIdx, &outNBElement, outReal);
	return outNBElement-1;
}


// https://stackoverflow.com/questions/15213082/c-histogram-bin-sorting
//
// unsigned int bin;
// for (unsigned int sampleNum = 0; sampleNum < SAMPLE_COUNT; ++sampleNum)
// {
//       const int sample = data0[sampleNum];
//       bin = BIN_COUNT;
//       for (unsigned int binNum = 0; binNum < BIN_COUNT; ++binNum)  {
//             const int rightEdge = binranges[binNum];
//             if (sample <= rightEdge) {
//                bin = binNum;
//                break;
//            }
//       }
//       bins[bin]++;
//  }


BOOL APIENTRY DllMain(HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
)
{
    TA_RetCode retCode;

    switch (ul_reason_for_call)
    {
        case DLL_PROCESS_ATTACH:
            retCode = TA_Initialize();
            // attach to process
            // return FALSE to fail DLL load
            // if( retCode != TA_SUCCESS )
            //  printf( "Cannot initialize TA-Lib (%d)!\n", retCode );
            break;

        case DLL_PROCESS_DETACH:
            // detach from process
            TA_Shutdown();
            break;

        case DLL_THREAD_ATTACH:
            // attach to thread
            break;

        case DLL_THREAD_DETACH:
            // detach from thread
            break;
    }
    return TRUE; // succesful
}
