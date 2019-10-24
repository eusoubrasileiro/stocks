#include "..\Buffers.mqh"

// there is no multiple inheritance on mql5
// an indicator the each sample calculated depends itself + window-1 samples before it
class CWindowIndicator: public CBuffer<double>
{

protected:

  int m_window;
  double m_previous_data[]; // last data
  // used to make previous calculations size window
  // needed to calculate next batch of new samples
  bool m_previous;
  double m_calculating[]; // now m_calculating


  void Init(int window){
      m_window = window;
      ArrayResize(m_previous_data, m_window-1);
      m_previous = false; // no previous data yet
      m_calculated = 0;
  }

public:
    int m_calculated; // number of samples calculated in the last call

    ~CWindowIndicator(){ArrayFree(m_previous_data); ArrayFree(m_calculating);}

    // re-calculate indicator values based on array of new data
    // where new data starts at start and has size count
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

      m_calculated = Calculate(m_calculating, ncalc+nnew_data, m_calculating);
      AddRange(m_calculating, m_calculated);

      // copy the now previous data for the subsequent call
      ArrayCopy(m_previous_data, newdata, 0, start+nnew_data-m_window+1, m_window-1);
      m_previous = true;

      return true;
    }

    // perform indicator calculation on indata
    // will only be called with enough samples
    // return number of new samples calculated
    // output can and should be same array
    virtual int Calculate(double &indata[], int size, double &outdata[]){ return 0; }

};
