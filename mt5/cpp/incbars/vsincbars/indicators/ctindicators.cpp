#include "cwindicator.h"
#include "ctalib.h"


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

    int Calculate(double indata[], int size, double outdata[])
    {
      return taMA(0, size,
           indata, m_window, m_tama_type, outdata);
    }

};


class CTaSTDDEV : public CWindowIndicator
{

public:

   CTaSTDDEV(int window){ CWindowIndicator::Init(window); };

   int Calculate(double indata[], int size, double outdata[])
   {
        return taSTDDEV(0,  size,
                indata, m_window, outdata);
   }

};


// triple buffer indicator
// cleanear code with multiple inheritance
class CTaBBANDS : IWindowIndicator
{
    protected:
        int m_tama_type;
        double m_devs; // number of deviatons from the mean
        std::vector<double> m_out_upper;
        std::vector<double> m_out_middle;
        std::vector<double> m_out_down;

    public:
      //CTaBBands code
      CCBuffer<double> m_upper;
      CCBuffer<double> m_middle;
      CCBuffer<double> m_down;

      CTaBBANDS(int window, double devs, int ma_type){
          m_devs = devs;
          m_tama_type = ma_type;
          IWindowIndicator::Init(window);
      };

      void AddEmpty(int count){
        m_upper.AddEmpty(count);
        m_middle.AddEmpty(count);
        m_down.AddEmpty(count);
      }

      int Calculate(double indata[], int size, double outdata[])
      {
          m_out_upper.resize(size);
          m_out_middle.resize(size);
          m_out_down.resize(size);

          return taBBANDS(0, // start the calculation at index
              size, // end the calculation at index
              indata,
              m_window, // From 1 to 100000  - MA window
              m_devs,
              m_tama_type, // MA type
              m_out_upper.data(),
              m_out_middle.data(),
              m_out_down.data());
      }

      void AddRange(std::vector<double>::iterator start, std::vector<double>::iterator end){
          // totally ignores signature since I have to add in three buffers
          m_upper.AddRange(m_out_upper.begin(), m_out_upper.begin()+m_calculated);
          m_middle.AddRange(m_out_middle.begin(), m_out_middle.begin()+m_calculated);
          m_down.AddRange(m_out_down.begin(), m_out_down.begin()+m_calculated);
      };

      void SetSize(const int size)
      {
        m_upper.SetSize(size);
        m_middle.SetSize(size); 
        m_down.SetSize(size);
      }

      int Size(){
        return m_middle.Size();
      }

};
