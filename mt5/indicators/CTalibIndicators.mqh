#include "CBufferIndicator.mqh"

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


class CTaMAIndicator : public CWindowIndicator
{
protected:
  int m_tama_type;

public:
    //ENUM_DEFINE(TA_MAType_SMA, Sma) = 0,
    //ENUM_DEFINE(TA_MAType_EMA, Ema) = 1,
    //ENUM_DEFINE(TA_MAType_WMA, Wma) = 2,
    //ENUM_DEFINE(TA_MAType_DEMA, Dema) = 3,
    //ENUM_DEFINE(TA_MAType_TEMA, Tema) = 4,
    //ENUM_DEFINE(TA_MAType_TRIMA, Trima) = 5,
    //ENUM_DEFINE(TA_MAType_KAMA, Kama) = 6,
    //ENUM_DEFINE(TA_MAType_MAMA, Mama) = 7,
    //ENUM_DEFINE(TA_MAType_T3, T3) = 8
    
    void setParams(int window, int tama_type){m_tama_type = tama_type; setWindow(window);}
    
    int Calculate(double &indata[], int size, double &outdata[])
    {
      return taMA(0, size, 
           indata, m_window, m_tama_type, outdata);
    }
    
};


class CTaSTDDEV : public CWindowIndicator
{

public:
    
   void setParams(int window){setWindow(window);}

   int Calculate(double &indata[], int size, double &outdata[])
   {
        return taSTDDEV(0,  size, 
                indata, m_window, outdata);
   }

};
