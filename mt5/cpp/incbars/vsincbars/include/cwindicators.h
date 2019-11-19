#include "buffers.h"

// an indicator the each sample calculated depends itself + window-1 samples before it
// or u need window samples to produce 1 output
class IWindowIndicator
{

protected:

  int m_window;
  // last data
  // used to make previous calculations size window
  // needed to calculate next batch of new samples
  // true or false for existing previous values
  CBuffer<double> m_prev_data;
  std::vector<double> m_calculating; // now m_calculating

  void Init(int window){
      //m_window1 = window-1;
      m_window = window;
      m_prev_data.Resize(m_window-1);
      m_calculated = 0;
  }

public:
    int m_calculated; // number of samples calculated in the last call


    // re-calculate indicator values based on array of new data
    // where new data starts at start and has size count
    bool Refresh(double newdata[], int start=0, int count=0){

      int nprev = m_prev_data.Size(); // number of previous data

      if(count==0) // no data
        return false;
      // needs m_window samples to calculate 1 output sample
      // check enough samples using previous
      if(nprev < m_window-1){
        if(nprev + count < m_window){ // cannot calculate 1 output
          // not enough data now, but insert on previous data
          m_prev_data.AddRange(newdata, start, count);
          // add dummy samples to mainting allignment with time and buffers
          AddEmpty(count);
          return false;
        }
        else { // now can calculate 1 or more outputs
          // insert the missing EMPTY_VALUES
          AddEmpty(m_window-1-nprev);
        }
      }

      // if exists previous data include its size for array of calculation      
      m_calculating.resize(nprev + count);
      // copy previous data if any
      // dst, src, dst_idx_srt, src_idx_srt, count_to_copy
      ArrayCopy<double>(m_calculating.data(), m_prev_data.m_data.data(), 0, 0, nprev);
      // copy in sequence new data
      ArrayCopy<double>(m_calculating.data(), newdata, nprev, start, count);

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
    virtual int Calculate(double indata[], int size, double outdata[]) = 0;

    virtual void AddEmpty(int count) = 0;

    virtual void Add(double value) = 0;

    virtual void AddRange(std::vector<double>::iterator start, std::vector<double>::iterator end) = 0;
};



class CWindowIndicator : public CCBuffer<double>, public IWindowIndicator
{
    
public:

    void AddEmpty(int count) {
        CCBuffer<double>::AddEmpty(count);
    }

    void Add(double value) {
        CCBuffer<double>::Add(value);
    }

    void AddRange(std::vector<double>::iterator start, std::vector<double>::iterator end) {
        CCBuffer<double>::AddRange(start, end);
    }

};



//
// Indicators based on Ctalib
//


class CTaSTDDEV : public CWindowIndicator{
public:
    CTaSTDDEV(int window);

    int Calculate(double indata[], int size, double outdata[]);
};




class CTaMAIndicator : public CWindowIndicator {

protected:
    int m_tama_type;

public:
    CTaMAIndicator(int window, int tama_type);

    int Calculate(double indata[], int size, double outdata[]);
};



class CTaBBANDS : public IWindowIndicator 
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

    CTaBBANDS(int window, double devs, int ma_type);

    void AddEmpty(int count);

    int Calculate(double indata[], int size, double outdata[]);

    void AddRange(std::vector<double>::iterator start, std::vector<double>::iterator end);

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
    CFracDiffIndicator(int window, double dfraction);

    int Calculate(double indata[], int size, double outdata[]);

};