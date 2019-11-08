#include "..\Buffers.mqh"
#include "Ticks.mqh"

// struct MqlTick
//   {
//    datetime     time;          // Time of the last prices update 1.
//    double       bid;           // Current Bid price
//    double       ask;           // Current Ask price
//    double       last;          // Price of the last deal (Last)
//    ulong        volume;        // Volume for the current Last price
//    long         time_msc;      // Time of a price last update in milliseconds == 1. but with milliseconds precision
//    uint         flags;         // Tick flags
//    double       volume_real;   // Volume for the current Last price with greater accuracy
//   };
//
// 2      TICK_FLAG_BID –  tick has changed a Bid price
// 4      TICK_FLAG_ASK  – a tick has changed an Ask price
// 8      TICK_FLAG_LAST – a tick has changed the last deal price
// 16     TICK_FLAG_VOLUME – a tick has changed a volume
// 32     TICK_FLAG_BUY – a tick is a result of a buy deal
// 64     TICK_FLAG_SELL – a tick is a result of a sell deal

// Fix array of ticks so if last ask, bid or last is 0
// they get filled with previous non-zero value as should be
// volume i dont care because it will only be usefull in change if flag >= 16
void FixArrayTicks(MqlTick &ticks[]){
  int nticks = ArraySize(ticks);
  double       bid = 0;           // Current Bid price
  double       ask = 0;           // Current Ask price
  double       last = 0;          // Price of the last deal (Last)
  double       volume = 0;
  for(int i=0; i<nticks;i++){ // cannot be done in parallel?
    if(ticks[i].bid == 0)
       ticks[i].bid = bid;
    else
       bid = ticks[i].bid; // save this for use next
    if(ticks[i].ask == 0)
       ticks[i].ask = ask;
    else
       ask = ticks[i].ask; // save this for use next
    if(ticks[i].last == 0)
       ticks[i].last = last;
    else
       last = ticks[i].last; // save this for use next
  }
}


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
    AddRange(m_copied_ticks, m_nnew);
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

// circular buffer version
// 10k ticks maximum downloaded every time Refresh is called
// 1 ms time-frame suggested using OnTimer
class CCBufferMqlTicks : public CCStructBuffer<MqlTick>
{
protected:

  int m_bgidx; // temp:: where the new ticks starts on array m_copied_ticks
  string m_symbol;
  MqlTick m_copied_ticks[]; // fixed size number of ticks copied every x seconds
  int m_ncopied; // last count of ticks copied on buffer
  int m_nnew; // number of new ticks last copied

public:
  long m_last_ms;

  CCBufferMqlTicks(void);

  CCBufferMqlTicks(string symbol){
      m_symbol = symbol;
      ArrayResize(m_copied_ticks, Max_Tick_Copy);
      m_ncopied = 0;
      // time now in ms - 0 will work 'coz next tick.ms value will be bigger
      // and will take its place
      m_last_ms = long (TimeCurrent())*1000; // turn in 'fake' ms
      m_nnew = 0;
  }

  ~CCBufferMqlTicks(){ArrayFree(m_copied_ticks);}

  int nNew(){ return m_nnew; } // number of new ticks after calling Refresh()

  int Refresh(){
   // copy all ticks from last copy time - 1 milisecond to now
   // to avoid missing ticks on same ms)
   m_ncopied = CopyTicksRange(m_symbol, m_copied_ticks,
        COPY_TICKS_ALL, m_last_ms-1, 0);

   if(m_ncopied < 1){
      int error = GetLastError();
      if(error == ERR_HISTORY_TIMEOUT)
        // ticks synchronization waiting time is up, the function has sent all it had.
        Print("ERR_HISTORY_TIMEOUT");
      else
      if(error == ERR_HISTORY_SMALL_BUFFER)
        //static buffer is too small. Only the amount the array can store has been sent.
        Print("ERR_HISTORY_SMALL_BUFFER");
      else
      if(error == ERR_NOT_ENOUGH_MEMORY)
        // insufficient memory for receiving a history from the specified range to the dynamic
        // tick array. Failed to allocate enough memory for the tick array.
        Print("ERR_NOT_ENOUGH_MEMORY");
      // better threatment here ??...better ignore and wait for next call
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

    FixArrayTicks(m_copied_ticks); // fix nonsense last, ask, bid empty
    AddRange(m_copied_ticks, 0, m_nnew);
    // get last tick ms inserted on buffer
    m_last_ms = m_copied_ticks[m_ncopied-1].time_msc;

    return m_nnew;
  }

  int indexesNewTicks(int &start1, int &end1,
                      int &start2, int &end2){

  return indexesData(Count()-m_nnew, m_nnew,
                   start1, end1, start2, end2);

  }

};


int ReadTicks(MqlTick &mqlticks[], string filename, int file_acces=0){

    ArrayResize(mqlticks, 100e6); // just to guarantee 100MM
    int handle = FileOpen(filename, file_acces|FILE_READ|FILE_BIN);    
    if(handle==INVALID_HANDLE)
        return -1;    
    int size = FileReadArray(handle, mqlticks);
    FileClose(handle);    
    ArrayResize(mqlticks, size);    
    
    
    return size;
}