#include "..\Buffers.mqh"

// there is no multiple inheritance on mql5
// an indicator the each sample calculated depends itself + window-1 samples before it
// or u need window samples to produce 1 output
class CWindowIndicator: public CBuffer<double>
{

protected:

  int m_window;
  // last data
  // used to make previous calculations size window
  // needed to calculate next batch of new samples
  // true or false for existing previous values
  CBuffer<double> m_prev_data;
  double m_calculating[]; // now m_calculating


  void Init(int window){
      //m_window1 = window-1;
      m_window = window;
      m_prev_data.ResizeFixed(m_window-1);
      m_calculated = 0;
  }

public:
    int m_calculated; // number of samples calculated in the last call

    ~CWindowIndicator(){delete GetPointer(m_prev_data); ArrayFree(m_calculating);}

    // re-calculate indicator values based on array of new data
    // where new data starts at start and has size count
    bool Refresh(double &newdata[], int start=0, int count=0){
      count = (count > 0)? count : ArraySize(newdata);

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
      ArrayResize(m_calculating, nprev+count);
      // copy previous data if any
      // dst, src, dst_idx_srt, src_idx_srt, count_to_copy
      ArrayCopy(m_calculating, m_prev_data.m_data, 0, 0, nprev);
      // copy in sequence new data
      ArrayCopy(m_calculating, newdata, nprev, start, count);

      m_calculated = Calculate(m_calculating, nprev+count, m_calculating);
      AddRange(m_calculating, 0, m_calculated);

      // copy the now previous data for the subsequent call
      m_prev_data.AddRange(newdata, start, count);

      return true;
    }

    // perform indicator calculation on indata
    // will only be called with enough samples
    // return number of new samples calculated
    // output can and should be same array
    virtual int Calculate(double &indata[], int size, double &outdata[]){ return 0; }

};
