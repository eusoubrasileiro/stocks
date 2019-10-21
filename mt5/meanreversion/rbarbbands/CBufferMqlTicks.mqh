m_ncopied#include "..\..\Buffers.mqh"

const int Max_Tick_Copy = 10e3;
// 10k ticks maximum downloaded every time Refresh is called
// 1 ms time-frame

class CBufferMqlTicks : public CStructBuffer<MqlTick>
{
protected:
  long m_last_ms;
  int m_last_ms_bgidx; // where the last ms tick starts index
  string m_symbol;
  MqlTick m_copied_ticks[]; // fixed size number of ticks copied every x seconds
  int m_ncopied; // last count of ticks copied on buffer
  int m_nnew; // number of new ticks last copied

public:
  void CBufferMqlTicks(string symbol){
      m_symbol = symbol;
      ArrayResize(m_copied_ticks, Max_Tick_Copy);
      m_ncopied = 0;
      m_last_ms = GetTickCount(); // time now in ms
  }

  int Refresh(){
    // from_msc = last tick downloaded Minus in ms - 1 (to avoid missing ticks on same ms)
   m_ncopied = CopyTicksRange(m_symbol, m_copied_ticks,
        COPY_TICKS_ALL, m_last_ms-1, 0);

    if(m_ncopied == -1){
      int error = GetLastError();
      // better threatment here ...
      // The number of copied tick or -1 in case of an error. GetLastError() is able to return the following errors:
      // ERR_HISTORY_TIMEOUT – ticks synchronization waiting time is up, the function has sent all it had.
      // ERR_HISTORY_SMALL_BUFFER – static buffer is too small. Only the amount the array can store has been sent.
      // ERR_NOT_ENOUGH_MEMORY – insufficient memory for receiving a history from the specified range to the dynamic
      // tick array. Failed to allocate enough memory for the tick array.
    }
    // get last tick ms
    // ticks are indexed from the past to the present,
    // i.e. the 0 indexed tick is the oldest one in the array.
    m_last_ms = m_copied_ticks[0].time_msc;
    // find index of first tick with this time
    if(m_ncopied > 1){
      m_last_ms_bgidx = 0;
      for(int i=1; i<m_ncopied; i++)
        if(m_copied_ticks[i].time_msc < m_last_ms){
          m_last_ms_bgidx = i+1;
          break;
        }
    }
    // correct offset of adding it

    // count ticks from last.time_msc-1 to end
    // the difference is the number of new ticks
    AddRange(m_copied_ticks, copied);
  }

// docs : returns a tick even if the same as previous
// 10k MqlTicks size doesnt matter since it will be allways fast
// copy due small number of ticks
// binary & comparision between mqltick structs in c++
// use the last mqltick download to find initial position of missing
// ticks
// called every x seconds
  int getNewTicks(double newTicks[]){
    Refresh();
  }

};
