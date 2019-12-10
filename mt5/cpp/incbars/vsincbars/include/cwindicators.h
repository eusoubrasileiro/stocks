#include "buffers.h"

// an indicator the each sample calculated depends itself + window-1 samples before it
// or u need window samples to produce 1 output
template<typename TypeSt, typename TypeIn>
class IWindowIndicator
{

protected:

  int m_window;
  // last data
  // used to make previous calculations size window
  // needed to calculate next batch of new samples
  // true or false for existing previous values
  buffer<TypeIn> m_prev_data;
  std::vector<TypeIn> m_calculating; // now m_calculating
  std::vector<TypeSt> m_calculated; // just calculated
  // where starts non EMPTY values
  int m_count; // count added samples (like size()) but continues beyound BUFFERSIZE 
  // to check when first m_window-1 samples are overwritten
  // where starts non EMPTY values
  int m_valid_start;
  bool m_all_valid; // when all samples are valid. No EMPTY_VALUES on buffer anymore

  void Init(int window) {
      m_window = window;
      m_prev_data.set_capacity(m_window - 1);
      m_ncalculated = 0;
      m_nempty = 0;
      m_valid_start = -1;
      m_all_valid = false;
      m_count = 0;
  }

public:
    int m_nempty; // number of empty samples added on last call
    int m_ncalculated; // number of samples calculated on the last call

    // re-calculate indicator values based on array of new data
    // where new data has size count
    int Refresh(TypeIn newdata[], int count){

      m_ncalculated = 0;
      m_nempty = 0; 

      int nprev = m_prev_data.size(); // number of previous data      

      if(count==0) // no data
        return 0;
      // needs m_window-1 previous samples to calculate 1 output sample
      // check enough samples using previous
      if(nprev < m_window-1){
        if(nprev + count < m_window){ // cannot calculate 1 output
          // not enough data now, but insert on previous data
          m_prev_data.addrange(newdata, count);
          // add dummy samples to mainting allignment with time and buffers
          m_nempty = count;
          AddEmpty(m_nempty);
          m_count += m_nempty;
          updateValidIdx();
          return 0;
        }
        else { // now can calculate 1 or more outputs
          // insert the missing EMPTY_VALUES
          m_nempty = m_window - 1 - nprev;
          AddEmpty(m_nempty);
          m_count += m_nempty;
          updateValidIdx();
        }
      }

      // if exists previous data include its size for array of calculation      
      m_calculating.resize(nprev + count);
      // copy previous data if any
      std::copy(m_prev_data.begin(), m_prev_data.end(), m_calculating.begin());
      // copy in sequence new data
      std::copy(newdata, newdata + count, m_calculating.begin() + nprev);

      m_calculated.resize(nprev+count); // output
      m_ncalculated = Calculate(m_calculating.data(), nprev+count, m_calculated.begin());

      AddRange(m_calculated.begin(), m_calculated.begin()+ m_ncalculated);
      m_count += m_ncalculated;
      updateValidIdx();

      // copy the now previous data for the subsequent call
      m_prev_data.addrange(newdata, count);

      return m_ncalculated;
    }

    int validIdx() { return m_valid_start; }

protected:

    // perform indicator calculation on indata
    // will only be called with enough samples
    // return number of new samples calculated
    // output can and should be same array
    virtual int Calculate(TypeIn indata[], int size, TypeSt outdata[]) = 0;

    virtual void AddEmpty(int count) = 0;

    virtual void AddRange(typename std::vector<TypeSt>::iterator start,
        typename std::vector<TypeSt>::iterator end) = 0;

 private:

    // calculate m_valid_start and m_all_valid samples on buffer
    inline void updateValidIdx() override {
        if (!m_all_valid) {
            if (m_count >= BUFFERSIZE + (m_window - 1)) {
                m_valid_start = 0; m_all_valid = true;
            }
            else
                m_valid_start = (m_count > m_window - 1) ? m_window - 1 : -1;
        }
    }
    
};

template<typename TypeSt, typename TypeIn>
class CWindowIndicator : public buffer<TypeSt>, public IWindowIndicator<TypeSt, TypeIn>
{
public:

    CWindowIndicator(void) { set_capacity(BUFFERSIZE); };

    void AddRange(typename std::vector<TypeSt>::iterator start, 
        typename std::vector<TypeSt>::iterator end) override {
        buffer<TypeSt>::addrange(start, end);
    }

};


class CWindowIndicatorDouble : public  CWindowIndicator<double, double> 
{
    void AddEmpty(int count) override {
        buffer<double>::addempty(count);
    }
};




//
// Indicators based on Ctalib
//


class CTaSTDDEV : public CWindowIndicatorDouble {
public:

    CTaSTDDEV(void) {};

    void Init(int window);

    int Calculate(double indata[], int size, double outdata[]) override;
};




class CTaMAIndicator : public CWindowIndicatorDouble {

protected:
    int m_tama_type;

public:
    CTaMAIndicator(void) {};

    void Init(int window, int tama_type);

    int Calculate(double indata[], int size, double outdata[]) override;
};

struct bands {
    double up, down;
};

class CTaBBANDS : public CWindowIndicator<bands, double>
{
protected:
    int m_tama_type;
    double m_devs; // number of deviatons from the mean
    // latest/newest calculated values
    std::vector<double> m_out_upper;
    std::vector<double> m_out_middle;
    std::vector<double> m_out_down;

public:

    CTaBBANDS() {};

    void Init(int window, double devs, int ma_type);

    void AddEmpty(int count) override;

    int Calculate(double indata[], int size, bands outdata[]) override;
};


//
// FracDiff
// 

class CFracDiffIndicator : public CWindowIndicatorDouble
{
protected:
    double m_dfraction; // fractional difference

public:

    CFracDiffIndicator() { m_dfraction = 1;  };

    void Init(int window, double dfraction);

    int Calculate(double indata[], int size, double outdata[]);

};


// a window indicator of size
// each sample only depends on the current and previous
// values on the bollinger bands 

// Based on a bollinger band defined by upper-band and lower-band
// calculate signals:
// buy   1 : crossing down-outside it's buy
// sell -1 : crossing up-outside it's sell
// hold  0 : nothing usefull happend
// storing int/short as a double -- but who cares?! for now...
class CBandSignal : public CWindowIndicator<int, double>
{
protected:

    CTaBBANDS bands;

public:
    bool m_first_call;

    CBandSignal() {
        m_first_call = true;
    };

    void Init(int window, double devs, int ma_type);

    int Calculate(double indata[], int size, int outdata[]);
};