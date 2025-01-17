#define NOMINMAX
#include <functional>
#include <array>
#include "buffers.h"
#include "ctalib.h"
#include "pytorchcpp.h"
#include "moneybars.h"

template<class T>
using vec_iterator = typename std::vector<T>::iterator;

// an indicator the each sample calculated
// depends itself + window-1 samples before it
// or u need window samples to produce 1 output
// simplest example: z
// - diff 1st order : window = 2 samples, out[t] = x[t]-x[t-1]
// - m_prev_data.resize(2-1) only need 1 previous sample
// TypeSt for storage - can have multiple buffers to store
// TypeIn for input 
template<typename TypeSt, typename TypeIn, int NumberBuffers>
class CWindowIndicator
{

protected:

  std::array<buffer<TypeSt>, NumberBuffers>  m_buffers;
  size_t m_buffersize;
  size_t m_window; // minimum number of input samples to produce 1 output sample
  size_t m_prev_needed;   // m_prev_need + 1 = 1  to produce 1 output sample
  // last data
  // used to make previous calculations size window
  // needed to calculate next batch of new samples
  // true or false for existing previous values
  buffer<TypeIn> m_prev_data;
  std::vector<TypeIn> m_calculating; // now calculating
  // just calculated, can have multiple outputs due multiple buffers  
  std::array<std::vector<TypeSt>, NumberBuffers> m_calculated; // outputs have storage type
  size_t m_nprev; // size of already stored data previous
  size_t m_new; // size of recent call new data
  // where starts non EMPTY values
  size_t m_total_count; // count added samples (like size()) but continues beyound BUFFERSIZE
    // Event on Refresh for Son indicator
  std::function<void(int)> m_onRefresh; // pass number of empty samples created or 0 for none
  size_t m_nempty; // number of dummy/empty samples added on last call
  size_t m_ndummy; // total dummy/empty on buffer
  size_t m_ncalculated; // number of samples being calculated or calculated on the last call

public :

  CWindowIndicator(int buffersize){
      m_window = 1; // output is m_window -1 + 1
      m_ncalculated = m_nempty = m_ndummy = m_total_count = m_new = m_nprev = m_prev_needed = 0;
      m_buffersize = buffersize;
      // max sized to avoid resizing during calls
      m_calculating.resize(m_buffersize);
      for (int i = 0; i < NumberBuffers; i++) {
          m_calculated[i].resize(m_buffersize);
          m_buffers[i].set_capacity(m_buffersize);
      }
  }

  CWindowIndicator() : CWindowIndicator(BUFFERSIZE)
  {};

  void Init(int window) {
      m_window = window;
      m_prev_needed = m_window - 1;
      m_prev_data.set_capacity(m_prev_needed);
  }

    // overload better than break everything
    template<class iterator_type>
    int Refresh(iterator_type start,
                iterator_type end) {

        auto count = std::distance(start, end);
        m_ncalculated = 0;
        m_nempty = 0;
        m_new = count;
        m_nprev = m_prev_data.size(); // number of previous data

        if (count==0) // no data
            return 0;
        // needs m_window-1 previous samples + count > 0 to calculate 1 output sample
        // check enough samples using previous
        if (m_nprev < m_prev_needed) { // m_prev_need + 1 = 1 output
            if (m_nprev + count < m_window) { // cannot calculate 1 output
              // not enough data now, but insert on previous data
                m_prev_data.addrange<iterator_type>(start, end);
                // add dummy samples to mainting allignment with time and buffers
                m_nempty = count;
                m_total_count += m_nempty;
                AddEmpty(m_nempty);
                if (m_onRefresh)
                    m_onRefresh(m_nempty);
                return 0;
            }
            else { // now can calculate 1 or more outputs
              // insert the missing EMPTY_VALUES
                m_nempty = m_prev_needed - m_nprev;
                m_total_count += m_nempty;
                AddEmpty(m_nempty);
            }
        }

        // copy previous data
        std::copy(m_prev_data.begin(), m_prev_data.end(), m_calculating.begin());
        // copy in sequence new data
        std::copy(start, end, m_calculating.begin() + m_nprev);
        // needs m_window-1 previous samples to calculate 1 output sample
        /////// 
        m_ncalculated = (m_nprev + m_new) - m_prev_needed;

        Calculate(m_calculating.data(), (m_nprev + m_new), m_calculated);

        AddLatest();

        // copy the now previous data for the subsequent call
        m_prev_data.addrange<iterator_type>(end-m_new, end);

        m_total_count += m_ncalculated;

        // if exists son indicator you can
        // call its Refresh now
        if (m_onRefresh)
            m_onRefresh(m_nempty);

        return m_ncalculated;
    }

    // number of samples being calculated or calculated on the last Refresh() call
    size_t nCalculated() { return m_ncalculated;  }

    // samples needed to calculated 1 output sample
    size_t Window() { return m_window;  }

    // using vBegin()
    // you dont need to keep track of valid indexes etc.
    // begin removing empty ==  first invalid samples
    typename buffer<TypeSt>::const_iterator  vBegin(int buffer_index) {
        // this wont be called that many times
        // correction wether circular buffer is full or not
        size_t offset = (m_total_count > m_buffersize) ? 0 : m_ndummy;
        // it will be end() if dont have any valid samples
        return m_buffers[buffer_index].begin() + offset;
    }

    typename buffer<TypeSt>::const_iterator  Begin(int buffer_index) {
        // this wont be called that many times
        // correction wether circular buffer is full or not
        return m_buffers[buffer_index].begin();
    }

    typename buffer<TypeSt>::const_iterator End(int buffer_index) {
        return m_buffers[buffer_index].end();
    }

    // return the buffer by index, default to first buffer
    typename buffer<TypeSt> & operator [](int buffer_index) {
        return m_buffers[buffer_index];
    }

    // return the buffer by index - no check, default to first buffer
    typename buffer<TypeSt>& Buffer(int buffer_index=0) {
        return m_buffers[buffer_index];
    }

    // last calculated samples begin
    typename buffer<TypeSt>::const_iterator  vLastBegin(int buffer_index) {
        return m_buffers[buffer_index].end() - m_ncalculated;
    }

    // last calculated samples begin
    typename buffer<TypeSt>::const_iterator  LastBegin(int buffer_index) {
        return m_buffers[buffer_index].end() - m_new;
    }


    // number of valid samples
    size_t vCount() {
        return std::distance(vBegin(0), End(0));
    }

    size_t Count() { // number of elements on buffer(s)/same
        return m_buffers[0].size();
    }

    // add a son indicator 'refresh' function
    void addOnRefresh(std::function<void(int)> on_refresh) {
        m_onRefresh = on_refresh;
    }

    // size
    size_t BufferSize() {
        return m_buffersize;
    }

private:
    // latest calculated data added on all buffers
     void AddLatest() {
         for (int i = 0; i < NumberBuffers; i++) {
             m_buffers[i].addrange(m_calculated[i].data(), m_ncalculated);
         }
     }

protected:

    // perform indicator calculation on indata
    // will only be called with enough samples
    // number of new samples calculated is m_ncalculated
    // output on outdata
    virtual void Calculate(TypeIn *indata, int size, std::array<std::vector<TypeSt>, NumberBuffers> &outdata) = 0;

    // cannot happen as virtual - also (TypeIn*indata, int size) is optimal for most 
    // indicators calculations
    //template<class iterator_type>
    //virtual void Calculate(iterator_type start, iterator_type end, 
    //    std::array<std::vector<TypeSt>, NumberBuffers>& outdata) = 0;

    void AddEmpty(int count) {
        m_ndummy += count;
        for (int i = 0; i < NumberBuffers; i++) // empty value / dummy on all buffers
            m_buffers[i].addempty(count);
    }
};



class CWindowIndicatorDouble : public  CWindowIndicator<double, double, 1>
{
public:

    CWindowIndicatorDouble(int buffersize) : CWindowIndicator<double, double, 1>(buffersize)
    {};

    CWindowIndicatorDouble(void) : CWindowIndicatorDouble(BUFFERSIZE)
    {};

};


//
// Indicators based on Ctalib
//


class CTaSTDDEV : public CWindowIndicatorDouble {

public:

    CTaSTDDEV(void) : CWindowIndicatorDouble() {};

    void Init(int window);

    void Calculate(double *indata, int size, std::array<std::vector<double>, 1> &outdata) override;
};




class CTaMA : public CWindowIndicatorDouble {

protected:
    int m_tama_type;

public:
    CTaMA(void) {};

    void Init(int window, int tama_type);

    void Calculate(double* indata, int size, std::array<std::vector<double>, 1> &outdata) override;
};


//   double up, down; array - up[0], down[1]
class CTaBBANDS : public CWindowIndicator<double, double, 2>
{
protected:
    int m_tama_type;
    double m_devs; // number of deviatons from the mean
    // middle band calculated values needed for ctalib
    std::vector<double> m_out_middle;

public:

    CTaBBANDS() {
        m_out_middle.resize(m_buffersize);
    };

    void Init(int window, double devs, int ma_type);

    void Calculate(double* indata, int size, std::array<std::vector<double>, 2> &outdata) override;

    // first buffer Upper Band
    inline double Up(size_t index) { return m_buffers[0].at(index); }
    // second buffer Lower Band
    inline double Down(size_t index) { return m_buffers[1].at(index); }
};


//
// FracDiff
//

class CFracDiff : public CWindowIndicator<float, float, 1>
{
protected:
    float m_dfraction; // fractional difference

public:

    CFracDiff() { m_dfraction = 1;  };

    void Init(int window, float dfraction);

    void Calculate(float* indata, int size, std::array<std::vector<float>, 1> &outdata) override;

};


//
// Return on MoneyBars price[i+1]/price[i] - 1.
// weighted return considering h, l and open inside each bar
//


class CMbReturn : public CWindowIndicator<double, MoneyBar, 1>
{

public:

    CMbReturn() : CWindowIndicator() {};

    void Init();

    void Calculate(MoneyBar* inbars, int size, std::array<std::vector<double>, 1>& outdata) override;

};

//
// Windowed Augmented Dickey-Fuller test or SADF (supremum) ADF
// SADF very optimized for GPU using Libtorch C++ Pytorch
//

// must be double because of MT5
// each SADF(t) point have two values associated
class CSADF : public CWindowIndicator<float, float, 2>
{
protected:
    int m_minw, m_maxw; // minimum and maximum backward window
    int m_order; // max lag order of AR model
    bool m_usedrift; // wether to include drift on AR model
    float m_gpumemgb; // how much GPU memory each batch of SADF(t) should have
    bool m_verbose; // wether show verbose messages when calculating
    // for calculation using pytorchpp.dll

public:

    CSADF(int buffersize);

    void Init(int maxwindow, int minwindow, int order, bool usedrift=false, float gpumemgb=2.0, bool verbose=false);

    void Calculate(float* indata, int size, std::array<std::vector<float>, 2> &outdata) override;

    // SADF(t) value - first buffer
    inline float SADFt(size_t index) { return m_buffers[0].at(index); }
    // max values percentile dispersion of ADF on backward expanding window 
    inline float MaxADFdisp(size_t index) { return m_buffers[1].at(index); }
};




// Cum Sum filter
// due numpy/mt5 NAN's usage better use float as storage instead of int
class CCumSum : public CWindowIndicator<float, float, 1>
{
protected:
    double m_cum_reset; // cumsum increment and reset level
    double m_cum_up; // up cumsum
    double m_cum_down;  // down cumsum

public:

    CCumSum(int buffersize);

    void Init(double cum_reset);

    void Calculate(float* indata, int size, std::array<std::vector<float>, 1>& outdata) override;
};


// Cum Sum filter
// due numpy/mt5 NAN's usage better use float as storage instead of int
class CCumSumSADF : public CCumSum
{
protected:
    int m_sadf_prevn;

public:

    CCumSumSADF(int buffersize);

    void Init(double cum_reset, CSADF* pSADF);

    void Calculate(float* indata, int size, std::array<std::vector<float>, 1>& outdata) override;

    inline double At(size_t index) { return m_buffers[0].at(index); }
};










// Volatility on Money Bars average Returns - STDEV on Returns
// also use inside day from money bars for calculation 
class CStdevMbReturn : public CTaSTDDEV
{

    int m_mbret_prevn;

public:

    CStdevMbReturn() : CTaSTDDEV() {};

    void Init(int window, CMbReturn* pMbReturns);

    void Calculate(double* indata, int size, std::array<std::vector<double>, 1>& outdata) override;

    inline double At(size_t index) { return m_buffers[0].at(index); }
};











///// previous implementation
// probably useless but I am unwilling to delete
// due all effort envolved


// Cum Sum filter
// due numpy/mt5 NAN's usage better use float as storage instead of int
class CCumSumPair : public CWindowIndicator<float, std::pair<float, int>, 1>
{
protected:
    double m_cum_reset; // cumsum increment and reset level
    double m_cum_up; // up cumsum
    double m_cum_down;  // down cumsum

public:

    CCumSumPair(int buffersize);

    void Init(double cum_reset);

    void Calculate(std::pair<float, int>* indata, int size, std::array<std::vector<float>, 1>& outdata) override;
};

// valid intraday operational window - not needed in fact
// 0 for not valid 1 for valid
// applied on timebars
//class CIntraday : public CWindowIndicator<int, MoneyBar, 1>
//{
//    float m_start_hour, m_end_hour;
//
//public:
//
//    CIntraday(int buffersize);
//
//    void Init(float start_hour, float end_hour);
//
//    void Calculate(MoneyBar* indata, int size, std::array<std::vector<int>, 1> & outdata) override;
//
//    inline int isIntraday(size_t index) { return m_buffers[0].at(index); }
//
//};


//// Cum Sum filter on SADF
class CCumSumSADFPair : public CCumSumPair
{
    int m_sadf_prevn;
public:

    CCumSumSADFPair(int buffersize);

    void Init(double cum_reset, CSADF* pSADF, MoneyBarBuffer* pBars);

    void Calculate(std::pair<float, int>* indata, int size, std::array<std::vector<float>, 1>& outdata) override;
};
