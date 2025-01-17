#include "..\Buffers.mqh"

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
void fixArrayTicks(MqlTick &ticks[]){
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

uint gticks = 0;
const int Max_Tick_Copy = 10e3;


// circular buffer version
// 10k ticks maximum downloaded every time Refresh is called
// 1 ms time-frame suggested using OnTimer
class CCBufferMqlTicks : public CCStructBuffer<MqlTick>
{
protected:
  string m_symbol;
  MqlTick m_copied_ticks[]; // fixed size number of ticks copied every x seconds
  int m_ncopied; // last count of ticks copied on buffer
  int m_nnew; // number of new ticks last copied
  bool scheck; // security check of sync with data server
  long m_cmpbegin_time;
  int m_cmpbegin;
  int m_bgidx; // temp:: where the new ticks starts on array m_copied_ticks

  //////////////////////////////////////////////
  ////// backtest workaround for stupid mt5 fake ticks creation
  bool isbacktest;
  MqlTick m_refticks[]; // correct ticks without stupid modifications
  int m_refsize; // size of reference data
  // on volume etc by MT5 to fuc** with everything serious
  string m_refcticks_file;
  int m_refpos; // position on reference ticks file
  int m_trash_count;
  ////////////////////////////////////////////
  ////// for testing against Python/C++ since i need to
  ////// know boundaries of new ticks (since data comes in chuncks)
  int m_bound_ticks[];
  int ib_tick; // count of calls to AddRange

  bool correctMt5UnrealTicks(){
    int real_nnew = 0;
    int begin_refpos = m_refpos;
    for(int i=0; i<m_nnew && m_refpos < m_refsize; i++){
      if(m_copied_ticks[i].time_msc
          == m_refticks[m_refpos].time_msc){
        real_nnew++; m_refpos++;
      }
    }
    // testing needs boundaries of data chuncks
    m_bound_ticks[ib_tick++] = m_refpos;
    ///////
    AddRange(m_refticks, begin_refpos, m_refpos);
    m_nnew = real_nnew;
    if(m_nnew==0){ // number of trash ticks created by Metatrader
      m_trash_count++;
      return false;
    }
    if(m_trash_count > 0){
      Print("Ignored metatrader 5 created unreal ticks : ",
      m_trash_count, " at ",
      (datetime) m_cmpbegin_time/1000);
      m_trash_count = 0;
    }
    return true;
  }

  void loadCorrectTicks(string symbol){
    isbacktest = false;
    if(MQLInfoInteger(MQL_TESTER)){ // must load specific file with ticks
      // located on COMMON folder
      // if ticks on this file is not compatible with
      // your tick history problems will come
      // advised to use a custom symbol to avoid that
      isbacktest = true;
      StringAdd(m_refcticks_file, symbol);
      StringAdd(m_refcticks_file,"_mqltick.bin");
      m_refsize = ReadTicks(m_refticks, m_refcticks_file, FILE_COMMON);
      // find begin for current backtest
      long timebegin_ms = (long) TimeCurrent()*1000;
      for(m_refpos = 0; m_refpos<m_refsize; m_refpos++)
          if(m_refticks[m_refpos].time_msc >= timebegin_ms) break;
      m_trash_count = 0;

      ArrayResize(m_bound_ticks, m_refsize); // certainly will be smaller
      ib_tick = 0;
    }
  }

//////////////////////////////////////////////
//////////////////////////////////////////////

  bool beginNewTicks(){
    scheck = false;
    // ticks are indexed from the past to the present,
    ArraySetAsSeries(m_copied_ticks, false); // cancel this
    // find index of first tick different
    // from what we already have using the comparision indexes
    if(isfull == 0 && m_cposition == 0){ // we have nothing - copy everything
      m_bgidx = 0; scheck = true;
    }
    else{
      for(int i=0; i<m_ncopied; i++)
        if(m_copied_ticks[i].time_msc == At(m_cmpbegin+i).time_msc){
          scheck=true; // must have at least one sample equal
          // for security check of sync with server
        }
        else{ // found begin new data
          m_bgidx = i;
          break;
        }
    }
    // the difference is the number of new ticks
    m_nnew = m_ncopied - m_bgidx;
    return (scheck);
  }

public:

  CCBufferMqlTicks(void);

  CCBufferMqlTicks(string symbol){
      m_symbol = symbol;
      ArrayResize(m_copied_ticks, Max_Tick_Copy);
      m_ncopied = 0;
      m_nnew = 0;
      // time now in ms will work 'coz next tick.ms value will be bigger
      // and will take its place
      m_cmpbegin_time = long (TimeCurrent())*1000; // turn in ms
      m_cmpbegin = 0;

      loadCorrectTicks(symbol);
  }

  ~CCBufferMqlTicks(){
    ArrayFree(m_copied_ticks);
  }

  int nNew(){ return m_nnew; } // number of new ticks after calling Refresh()

  int Refresh(){
   // copy all ticks from last copy time - 1 milisecond to now
   // to avoid missing ticks on same ms)
   m_ncopied = CopyTicksRange(m_symbol, m_copied_ticks,
        COPY_TICKS_ALL, m_cmpbegin_time, 0);

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

   if(!beginNewTicks()){
     Print("Data Without Sync With Server!");
     Print("Forcing Stop");
     int error = 0; error = 1/error;  // division by 0 stops EA
   }

    // allign array of new ticks to zero
    ArrayCopy(m_copied_ticks, m_copied_ticks, 0, m_bgidx, m_nnew);
    // fix nonsense last, ask, bid empty
    fixArrayTicks(m_copied_ticks);

    // if on back-test
    // dont use ticks from metatrader, only times!
    if(isbacktest){
      if(!correctMt5UnrealTicks())
        return 0; // no new ticks
    }
    else{ // real time operation
      AddRange(m_copied_ticks, 0, m_nnew);
    }

    // get begin time of comparison to check for new ticks
    // last tick ms -1 on buffer
    // get index of first tick with time >= (last tick ms) - (1 ms)
    m_cmpbegin_time = m_copied_ticks[m_nnew-1].time_msc-1;
    m_cmpbegin = 0;
    for(int i=Count()-1; i>=0; i--)
      if(At(i).time_msc < m_cmpbegin_time){
        m_cmpbegin = i+1; break;
      }

    gticks += m_nnew;

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
