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
  buffer<Type> m_prev_data;
  std::vector<Type> m_calculating; // now m_calculating

  void Init(int window){
      //m_window1 = window-1;
      m_window = window;
      m_prev_data.set_capacity(m_window-1);
      m_calculated = 0;
      m_nempty = 0;
  }

public:
    int m_nempty; // number of empty samples added on last call
    int m_calculated; // number of samples calculated on the last call


    // re-calculate indicator values based on array of new data
    // where new data starts at start and has size count
    bool Refresh(Type newdata[], int count){

      m_calculated = 0;
      m_nempty = 0; 

      int nprev = m_prev_data.size(); // number of previous data      

      if(count==0) // no data
        return false;
      // needs m_window samples to calculate 1 output sample
      // check enough samples using previous
      if(nprev < m_window-1){
        if(nprev + count < m_window){ // cannot calculate 1 output
          // not enough data now, but insert on previous data
          m_prev_data.addrange(newdata, count);
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
      std::copy(m_prev_data.begin(), m_prev_data.end(), m_calculating.begin());
      // copy in sequence new data
      std::copy(newdata, newdata + count, m_calculating.begin() + nprev);

      m_calculated = Calculate(m_calculating.data(), nprev+count, m_calculating.data());

      AddRange(m_calculating.begin(), m_calculating.begin()+m_calculated);

      // copy the now previous data for the subsequent call
      m_prev_data.addrange(newdata, count);

      return true;
    }

protected:

    // perform indicator calculation on indata
    // will only be called with enough samples
    // return number of new samples calculated
    // output can and should be same array
    virtual int Calculate(Type indata[], int size, Type outdata[]) = 0;

    virtual void AddEmpty(int count) = 0;

    virtual void Add(Type value) = 0;

    virtual void AddRange(typename std::vector<Type>::iterator start, typename std::vector<Type>::iterator end) = 0;
};



class CWindowIndicator : public buffer<double>, public IWindowIndicator<double>
{
    
public:

    CWindowIndicator(void) { set_capacity(BUFFERSIZE); };

    void AddEmpty(int count) override {
        buffer<double>::addempty(count);
    }

    void Add(double value) override {
        buffer<double>::add(value);
    }

    void AddRange(std::vector<double>::iterator start, std::vector<double>::iterator end) override  {
        buffer<double>::addrange(start, end);
    }

};



//
// Indicators based on Ctalib
//


class CTaSTDDEV : public CWindowIndicator{
public:

    CTaSTDDEV(void) {};

    void Init(int window);

    int Calculate(double indata[], int size, double outdata[]) override;
};




class CTaMAIndicator : public CWindowIndicator {

protected:
    int m_tama_type;

public:
    CTaMAIndicator(void) {};

    void Init(int window, int tama_type);

    int Calculate(double indata[], int size, double outdata[]) override;
};



class CTaBBANDS : public IWindowIndicator<double> 
{
protected:
    int m_tama_type;
    double m_devs; // number of deviatons from the mean

public:
    // latest/newest calculated values
    std::vector<double> m_out_upper;
    std::vector<double> m_out_middle;
    std::vector<double> m_out_down;
    buffer<double> m_upper;
    buffer<double> m_middle;
    buffer<double> m_down;

    CTaBBANDS() {};

    void Init(int window, double devs, int ma_type);

    void AddEmpty(int count) override;

    int Calculate(double indata[], int size, double outdata[]) override;

    void AddRange(std::vector<double>::iterator start, std::vector<double>::iterator end) override;

    void Add(double value) override {}; // does nothing since this is tripple buffer

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

    CFracDiffIndicator() { m_dfraction = 1;  };

    void Init(int window, double dfraction);

    int Calculate(double indata[], int size, double outdata[]);

};


// not a window indicator : each sample only depends on the current
// values on the bollinger bands

// Based on a bollinger band defined by upper-band and lower-band
// calculate signals:
// buy   1 : crossing down-outside it's buy
// sell -1 : crossing up-outside it's sell
// hold  0 : nothing usefull happend
// storing int/short as a double -- but who cares?! for now...
class CBandSignal : public buffer<double>
{
protected:

    CTaBBANDS bands;

public:
    int m_calculated;

    CBandSignal() { m_calculated = 0; };

    void Init(int window, double devs, int ma_type);

    bool Refresh(double newdata[], int count);
};