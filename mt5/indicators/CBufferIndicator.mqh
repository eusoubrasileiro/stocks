#include "..\Buffers.mqh"

// Base clas indicator  for calculations using ctalib
template<typename Type>
class CBufferIndicator : CBuffer<Type>
{
protected:

public:
    CBufferIndicator(void);
    
    // re-calculate indicator values
    virtual bool Refresh(double &new_data[]);

};

// an indicator the each sample depend on window-1 samples before it
class CWindowIndicator: public CBuffer<double>
{

protected:

  int m_window;
  double m_previous_data[]; // last data
  // used to make previous calculations size window
  // needed to calculate next batch of new samples
  bool m_previous;
  double m_calculating[]; // now m_calculating
  
  void setWindow(int window){
      m_window = window;
      ArrayResize(m_previous_data, m_window);
      m_previous = false; // no previous data yet
  }
  
public:

  ~CWindowIndicator(){ArrayFree(m_previous_data); ArrayFree(m_calculating);}

    // re-calculate indicator values
    bool Refresh(double &newdata[]){
      int nnew_data = ArraySize(newdata);

      if( nnew_data == 0 || // not enough data  or no data
          (!m_previous && nnew_data < m_window))
        return false;

      // if exists previous data include its size for array of calculation
      int ncalc = (m_previous)? m_window: 0;
      ArrayResize(m_calculating, ncalc+nnew_data);
      // src, dst, dst_idx_srt, src_idx_srt, count_to_copy
      // copy previous data if any
      ArrayCopy(m_previous_data, m_calculating, 0, 0, ncalc);
      // copy in sequence new data
      ArrayCopy(newdata, m_calculating, ncalc, 0, nnew_data);
      
      int outsize = Calculate(m_calculating, ncalc+nnew_data, m_calculating);      
      AddRange(m_calculating, outsize);
      
      // copy the now previous data for the subsequent call
      ArrayCopy(newdata, m_previous_data, nnew_data-m_window, m_window);
      m_previous = true;

      return true;      
    }
    
    // perform indicator calculation on indata
    // will only be called with enough samples
    // return number of new samples calculated
    // output can and should be same array
    virtual int Calculate(double &indata[], int size, double &outdata[]){ return 0; }
  
};
