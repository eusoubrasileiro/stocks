#include "..\Buffers.mqh"
#include "Ticks.mqh"

const int Max_Tick_Copy = 10e3;
// 10k ticks maximum downloaded every time Refresh is called
// 1 ms time-frame suggested using OnTimer
class CBufferMqlTicks : public CStructBuffer<MqlTick>
{
protected:
  
  int m_bgidx; // temp:: where the new ticks starts on array m_copied_ticks
  string m_symbol;
  MqlTick m_copied_ticks[]; // fixed size number of ticks copied every x seconds
  int m_ncopied; // last count of ticks copied on buffer
  int m_nnew; // number of new ticks last copied

public:
  long m_last_ms;
  
  CBufferMqlTicks(void);
    
  CBufferMqlTicks(string symbol){
      m_symbol = symbol;
      ArrayResize(m_copied_ticks, Max_Tick_Copy);
      m_ncopied = 0;
      // time now in ms - 0 will work 'coz next tick.ms value will be bigger
      // and will take its place
      m_last_ms = long (TimeCurrent())*1000; // turn in 'fake' ms
      m_nnew = 0;
  }

  ~CBufferMqlTicks(){ArrayFree(m_copied_ticks);}

  int nNew(){ return m_nnew; } // number of new ticks after calling Refresh()

  int Refresh(){
   // copy all ticks from last copy time - 1 milisecond to now
   // to avoid missing ticks on same ms)
   m_ncopied = CopyTicksRange(m_symbol, m_copied_ticks,
        COPY_TICKS_ALL, m_last_ms-1, 0);
        
    if(m_ncopied < 1){
      //int error = GetLastError();
      // better threatment here ??...better ignore and wait for next call
      // The number of copied tick or -1 in case of an error. GetLastError() is able to return the following errors:
      // ERR_HISTORY_TIMEOUT – ticks synchronization waiting time is up, the function has sent all it had.
      // ERR_HISTORY_SMALL_BUFFER – static buffer is too small. Only the amount the array can store has been sent.
      // ERR_NOT_ENOUGH_MEMORY – insufficient memory for receiving a history from the specified range to the dynamic
      // tick array. Failed to allocate enough memory for the tick array.
      //m_nnew = 0;
      //m_ncopied = 0;
      return -1;
    }

    // ticks are indexed from the past to the present,
    ArraySetAsSeries(m_copied_ticks, false); // cancel this

    // find index of first tick with time greater than last copied
    // begin of new ticks
    m_bgidx = 0;
    if(m_ncopied > 1){
      for(int i=0; i<m_ncopied; i++)
        if(m_copied_ticks[i].time_msc > m_last_ms){
          m_bgidx = i;
          break;
        }
    }
    m_nnew = m_ncopied - m_bgidx;
    // allign array of new ticks to zero
    ArrayCopy(m_copied_ticks, m_copied_ticks, 0, m_bgidx, m_nnew);
    // count ticks from last.time_msc-1 to end
    // the difference is the number of new ticks

    FixArrayTicks(m_copied_ticks); // firx nonsense last, ask, bid empty
    CStructBuffer<MqlTick>::AddRange(m_copied_ticks, m_nnew);
    // get last tick ms inserted on buffer
    m_last_ms = m_copied_ticks[m_ncopied-1].time_msc;

    return m_nnew;
  }

  // called every any milliseconds
  // index new ticks added
  int beginNewTicks(){
    //Refresh();
    //newTicks = GetPointer(m_data[m_data_total-m_nnew]);
    return m_data_total-m_nnew;
  }

};
