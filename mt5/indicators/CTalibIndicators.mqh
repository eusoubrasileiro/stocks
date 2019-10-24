#include "CWindowIndicator.mqh"

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

    int taBBANDS(int  startIdx, // start the calculation at index
    					int    endIdx, // end the calculation at index
    					const double &inReal[],
    					int      optInTimePeriod, // From 1 to 100000  - MA window
    					double   optInNbDev,
    					int      optInMAType, // MA type
    					double &outRealUpperBand[],
    					double &outRealMiddleBand[],
    					double &outRealLowerBand[]);
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

    CTaMAIndicator(int window, int tama_type){
        m_tama_type = tama_type;
        CWindowIndicator::Init(window);
    };

    int Calculate(double &indata[], int size, double &outdata[])
    {
      return taMA(0, size,
           indata, m_window, m_tama_type, outdata);
    }

};


class CTaSTDDEV : public CWindowIndicator
{

public:

   CTaSTDDEV(int window){ CWindowIndicator::Init(window); };

   int Calculate(double &indata[], int size, double &outdata[])
   {
        return taSTDDEV(0,  size,
                indata, m_window, outdata);
   }

};


// triple buffer indicator
// cannot write clean code without multiple inheritance
class CTaBBANDS
{

protected:
   // borroewd code from CWindowIndicator due no multiple inheritance
    int m_window;
    double m_previous_data[]; // last data
    // used to make previous calculations size window
    // needed to calculate next batch of new samples
    bool m_previous;
    double m_calculating[]; // now m_calculating
    //CTaBBands code
    int m_tama_type;
    double m_devs; // number of deviatons from the mean
    double m_out_upper[];
    double m_out_middle[];
    double m_out_down[];

public:
   // borroewd code from CWindowIndicator due no multiple inheritance
  int m_calculated; // number of samples calculated in the last call
  //CTaBBands code
  CBuffer<double> *m_upper;
  CBuffer<double> *m_middle;
  CBuffer<double> *m_down;

  // borroewd code from CWindowIndicator due no multiple inheritance
  void Init(int window){
      m_window = window;
      ArrayResize(m_previous_data, m_window-1);
      m_previous = false; // no previous data yet
      m_calculated = 0;
  }

  CTaBBANDS(int window, double devs, int ma_type){
      m_upper = new CBuffer<double>();
      m_down = new CBuffer<double>();
      m_middle = new CBuffer<double>();
      m_devs = devs;
      m_tama_type = ma_type;
      // borroewd code from CWindowIndicator due no multiple inheritance
      Init(window);
  };

  ~CTaBBANDS(){
    delete GetPointer(m_middle);
    delete GetPointer(m_upper);
    delete GetPointer(m_down);
    ArrayFree(m_out_upper);
    ArrayFree(m_out_middle);
    ArrayFree(m_out_down);
    // borroewd code from CWindowIndicator due no multiple inheritance
    ArrayFree(m_previous_data);
    ArrayFree(m_calculating);
  }

  // re-calculate indicator values
  bool Refresh(double &newdata[], int start=0, int count=0){
    int nnew_data = (count > 0)? count : ArraySize(newdata);    

    if( nnew_data == 0 || // not enough data  or no data
        (!m_previous && nnew_data < m_window))
      return false;

    // if exists previous data include its size for array of calculation
    int ncalc = (m_previous)? m_window-1: 0;
    ArrayResize(m_calculating, ncalc+nnew_data);
    // copy previous data if any
    // dst, src, dst_idx_srt, src_idx_srt, count_to_copy
    ArrayCopy(m_calculating, m_previous_data, 0, 0, ncalc);
    // copy in sequence new data
    ArrayCopy(m_calculating, newdata, ncalc, start, nnew_data);
    // output arrays
    ArrayResize(m_out_upper, nnew_data+ncalc);
    ArrayResize(m_out_middle, nnew_data+ncalc);
    ArrayResize(m_out_down, nnew_data+ncalc);

    m_calculated = taBBANDS(0, // start the calculation at index
             ncalc+nnew_data, // end the calculation at index
             m_calculating,
             m_window, // From 1 to 100000  - MA window
             m_devs,
             m_tama_type, // MA type
             m_out_upper,
             m_out_middle,
             m_out_down);

    m_upper.AddRange(m_out_upper, m_calculated);
    m_middle.AddRange(m_out_middle, m_calculated);
    m_down.AddRange(m_out_down, m_calculated);

    // copy the now previous data for the subsequent call
    ArrayCopy(m_previous_data, newdata, 0, start+nnew_data-m_window+1, m_window-1);
    m_previous = true;

    return true;
  }

  bool Resize(const int size)
  {
    return m_upper.Resize(size) &&
           m_middle.Resize(size) &&
           m_down.Resize(size);
  }

  int Size(){
    return m_middle.Size();
  }

};
