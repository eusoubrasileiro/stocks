#include "buffers.h"

// an indicator the each sample calculated depends itself + window-1 samples before it
// or u need window samples to produce 1 output
template<typename Type>
class IWindowIndicator
{

protected:

  int m_window;
  // last data
  // used to make previous calculations size window
  // needed to calculate next batch of new samples
  // true or false for existing previous values
  CBuffer<Type> m_prev_data;
  std::vector<Type> m_calculating; // now m_calculating

  void Init(int window){
      //m_window1 = window-1;
      m_window = window;
      m_prev_data.Resize(m_window-1);
      m_calculated = 0;
      m_nempty = 0;
  }

public:
    int m_nempty; // number of empty samples added on last call
    int m_calculated; // number of samples calculated on the last call


    // re-calculate indicator values based on array of new data
    // where new data starts at start and has size count
    bool Refresh(Type newdata[], int start, int count){

      m_calculated = 0;
      m_nempty = 0; 

      int nprev = m_prev_data.Count(); // number of previous data      

      if(count==0) // no data
        return false;
      // needs m_window samples to calculate 1 output sample
      // check enough samples using previous
      if(nprev < m_window-1){
        if(nprev + count < m_window){ // cannot calculate 1 output
          // not enough data now, but insert on previous data
          m_prev_data.AddRange(newdata, start, count);
          // add dummy samples to mainting allignment with time and buffers
          m_nempty = count;
          AddEmpty(m_nempty);
          return false;
        }
        else { // now can calculate 1 or more outputs
          // insert the missing EMPTY_VALUES
          m_nempty = m_window - 1 - nprev;
          AddEmpty(m_nempty);
        }
      }

      // if exists previous data include its size for array of calculation      
      m_calculating.resize(nprev + count);
      // copy previous data if any
      // dst, src, dst_idx_srt, src_idx_srt, count_to_copy
      ArrayCopy<Type>(m_calculating.data(), m_prev_data.m_data.data(), 0, 0, nprev);
      // copy in sequence new data
      ArrayCopy<Type>(m_calculating.data(), newdata, nprev, start, count);

      m_calculated = Calculate(m_calculating.data(), nprev+count, m_calculating.data());

      AddRange(m_calculating.begin(), m_calculating.begin()+m_calculated);

      // copy the now previous data for the subsequent call
      m_prev_data.AddRange(newdata, start, count);

      return true;
    }

    // perform indicator calculation on indata
    // will only be called with enough samples
    // return number of new samples calculated
    // output can and should be same array
    virtual int Calculate(Type indata[], int size, Type outdata[]) = 0;

    virtual void AddEmpty(int count) = 0;

    virtual void Add(Type value) = 0;

    virtual void AddRange(typename std::vector<Type>::iterator start, typename std::vector<Type>::iterator end) = 0;
};



class CWindowIndicator : public CCBuffer<double>, public IWindowIndicator<double>
{
    
public:

    void AddEmpty(int count) override {
        CCBuffer<double>::AddEmpty(count);
    }

    void Add(double value) override {
        CCBuffer<double>::Add(value);
    }

    void AddRange(std::vector<double>::iterator start, std::vector<double>::iterator end) override  {
        CCBuffer<double>::AddRange(start, end);
    }

};



//
// Indicators based on Ctalib
//


class CTaSTDDEV : public CWindowIndicator{
public:
    CTaSTDDEV(int window);

    int Calculate(double indata[], int size, double outdata[]) override;
};




class CTaMAIndicator : public CWindowIndicator {

protected:
    int m_tama_type;

public:
    CTaMAIndicator(int window, int tama_type);

    int Calculate(double indata[], int size, double outdata[]) override;
};



class CTaBBANDS : public IWindowIndicator<double> 
{
protected:
    int m_tama_type;
    double m_devs; // number of deviatons from the mean
    std::vector<double> m_out_upper;
    std::vector<double> m_out_middle;
    std::vector<double> m_out_down;

public:

    CCBuffer<double> m_upper;
    CCBuffer<double> m_middle;
    CCBuffer<double> m_down;

    CTaBBANDS(int window, double devs, int ma_type, int size);

    void AddEmpty(int count) override;

    int Calculate(double indata[], int size, double outdata[]) override;

    void AddRange(std::vector<double>::iterator start, std::vector<double>::iterator end) override;

    void Add(double value) override {}; // does nothing since this is tripple buffer

    void SetSize(const int size);

    int Size();
};


//
// FracDiff
// 

class CFracDiffIndicator : public CWindowIndicator
{
protected:
    double m_dfraction; // fractional difference

public:

    CFracDiffIndicator(int window, double dfraction, int size);

    int Calculate(double indata[], int size, double outdata[]);

};

// Based on a bollinger band defined by upper-band and lower-band
// calculate signals:
// buy   1 : crossing down-outside it's buy
// sell -1 : crossing up-outside it's sell
// hold  0 : nothing usefull happend
// storing int/short as a double -- but who cares?! for now...
class CBandSignal : public CWindowIndicator
{
protected:

    std::shared_ptr<CTaBBANDS> pbands;

public:

    CBandSignal(int window, double devs, int ma_type, int bfsize);

    bool Refresh(double newdata[], int start, int count);

    int Calculate(double indata[], int size, double outdata[]) override;
};