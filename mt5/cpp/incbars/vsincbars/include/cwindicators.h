#include "buffers.h"

// an indicator the each sample calculated 
// depends itself + window-1 samples before it
// or u need window samples to produce 1 output
// TypeSt for storage
// TypeIn for input
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
  int m_nprev; // size of already stored data previous
  int m_new; // size of recent call new data
  // where starts non EMPTY values
  int m_count; // count added samples (like size()) but continues beyound BUFFERSIZE 
  // to check when first m_window-1 samples are overwritten
  // where starts non EMPTY values
  int m_valid_idx;
  bool m_all_valid; // when all samples are valid. No EMPTY_VALUES on buffer anymore

  IWindowIndicator() {
      m_ncalculated = 0;
      m_nempty = 0;
      m_valid_idx = -1;
      m_all_valid = false;
      m_count = 0;      
      m_new = 0;
      m_nprev = 0;
      // max sized to avoid resizing during calls
      m_calculating.resize(BUFFERSIZE);
      m_calculated.resize(BUFFERSIZE);
  }

  void Init(int window) {
      m_window = window;
      m_prev_data.set_capacity(m_window - 1);
  }

public:

    int m_nempty; // number of empty samples added on last call
    int m_ncalculated; // number of samples being calculated or calculated on the last call

    // re-calculate indicator values based on array of new data
    // where new data has size count
    int Refresh(TypeIn newdata[], int count){

      m_ncalculated = 0;
      m_nempty = 0; 
      m_new = count;
      m_nprev = m_prev_data.size(); // number of previous data      

      if(count==0) // no data
        return 0;
      // needs m_window-1 previous samples to calculate 1 output sample
      // check enough samples using previous
      if(m_nprev < m_window-1){
        if(m_nprev + count < m_window){ // cannot calculate 1 output
          // not enough data now, but insert on previous data
          m_prev_data.addrange(newdata, count);
          // add dummy samples to mainting allignment with time and buffers
          m_nempty = count;
          AddEmpty(m_nempty);
          m_count += m_nempty;
          updatevalid();
          return 0;
        }
        else { // now can calculate 1 or more outputs
          // insert the missing EMPTY_VALUES
          m_nempty = m_window - 1 - m_nprev;
          AddEmpty(m_nempty);
          m_count += m_nempty;
          updatevalid();
        }
      }

      // if exists previous data include it
      // copy previous data 
      std::copy(m_prev_data.begin(), m_prev_data.end(), m_calculating.begin());
      // copy in sequence new data
      std::copy(newdata, newdata + m_new, m_calculating.begin() + m_nprev);
      // needs m_window-1 previous samples to calculate 1 output sample
      m_ncalculated = (m_nprev + m_new) - (m_window -1);

      Calculate(m_calculating.data(), (m_nprev + m_new), m_calculated.data());
      AddRange(m_calculated.begin(), m_calculated.begin() + m_ncalculated);

      // copy the now previous data for the subsequent call
      m_prev_data.addrange(newdata, m_new);

      m_count += m_ncalculated;
      updatevalid();

      return m_ncalculated;
    }

    // -1 if size=0 or index of first valid sample (not empty value)
    int valididx() { return m_valid_idx; }

protected:

    // perform indicator calculation on indata
    // will only be called with enough samples
    // number of new samples calculated is m_ncalculated
    // output on outdata
    virtual void Calculate(TypeIn indata[], int size, TypeSt outdata[]) = 0;

    virtual void AddEmpty(int count) = 0;

    virtual void AddRange(typename std::vector<TypeSt>::iterator start,
        typename std::vector<TypeSt>::iterator end) = 0;

 private:

    // calculate m_valid_idx and m_all_valid samples on buffer
    inline void updatevalid(){
        if (!m_all_valid) {
            if (m_count >= BUFFERSIZE + (m_window - 1)) {
                m_valid_idx = 0; m_all_valid = true;
            }
            else
                m_valid_idx = (m_count > m_window - 1) ? m_window - 1 : -1;
        }
    }
    
};

template<typename TypeSt, typename TypeIn>
class CWindowIndicator : public buffer<TypeSt>, public IWindowIndicator<TypeSt, TypeIn>
{
public:

    CWindowIndicator(void) { buffer<TypeSt>::set_capacity(BUFFERSIZE); };

    void AddRange(typename std::vector<TypeSt>::iterator start, 
        typename std::vector<TypeSt>::iterator end) override {
        buffer<TypeSt>::addrange(start, end);
    }

    // number of valid samples
    int nvalid() { 
        int valid_idx = IWindowIndicator<TypeSt, TypeIn>::m_valid_idx;
        return (valid_idx < 0) ? 0 : buffer<TypeSt>::size() - valid_idx;
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

    void Calculate(double indata[], int size, double outdata[]) override;
};




class CTaMAIndicator : public CWindowIndicatorDouble {

protected:
    int m_tama_type;

public:
    CTaMAIndicator(void) {};

    void Init(int window, int tama_type);

    void Calculate(double indata[], int size, double outdata[]) override;
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

    CTaBBANDS() {
        m_out_upper.resize(BUFFERSIZE);
        m_out_middle.resize(BUFFERSIZE);
        m_out_down.resize(BUFFERSIZE);
    };

    void Init(int window, double devs, int ma_type);

    void AddEmpty(int count) override;

    void Calculate(double indata[], int size, bands outdata[]) override;
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

    void Calculate(double indata[], int size, double outdata[]);

};


// a window indicator of size
// each sample only depends on the current and previous
// values on the bollinger bands 

// Based on a bollinger band defined by upper-band and lower-band
// calculate signals:
// buy   1 : crossing down-outside it's buy
// sell -1 : crossing up-outside it's sell
// hold  0 : nothing usefull happend
class CBandSignal : public CWindowIndicator<int, double>
{
protected:
    CTaBBANDS bands;

public:
    CBandSignal() {};

    void Init(int window, double devs, int ma_type);

    void Calculate(double indata[], int size, int outdata[]);

    int Refresh(double newdata[], int count);

    void AddEmpty(int count) override {
        buffer<int>::addempty(count);
    }
};

//
// Windowed Augmented Dickey-Fuller test or SADF (supremum) ADF
// SADF
// 

#include <..\urt\include\URT.hpp>

class CSADFIndicator : public CWindowIndicatorDouble
{
protected:
    std::shared_ptr<urt::ADF<double>> m_adfuller;
    urt::Vector<double> m_urtdata;
    std::string m_lagmethod;
    int m_minw, m_maxw;

public:

    CSADFIndicator();

    void Init(int minwindow, int maxwindow, std::string lagmethod = "AIC");

    void Calculate(double indata[], int size, double outdata[]);

};

