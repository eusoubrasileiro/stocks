#pragma once

#include <string>
#include <iostream>
#include <time.h>
#include <algorithm>
#include <functional>
#include "buffers.h"
#define NOMINMAX


#define MAX_TICKS 20000000

typedef  int64_t unixtime;
typedef  int64_t unixtime_ms;

#pragma pack(push, 2)
struct MqlTick
{
    unixtime     time;          // Time of the last prices update 1.
    double       bid;           // Current Bid price
    double       ask;           // Current Ask price
    double       last;          // Price of the last deal (Last)
    int64_t      volume;        // Volume for the current Last price
    unixtime_ms  time_msc;      // Time of a price last update in milliseconds (1). But with milliseconds precision.
    int          flags;         // Tick flags
    double       volume_real;   // Volume for the current Last price with greater accuracy
}; // sizeof is 60 bytes to avoid auto alignment to 64 by compiler #pragma pack(2) above is needed
#pragma pack(pop)

//#define TICK_FLAG_BID 2      // �  tick has changed a Bid price
//#define TICK_FLAG_ASK 4      // � a tick has changed an Ask price
//#define TICK_FLAG_LAST 8     // � a tick has changed the last deal price
//#define TICK_FLAG_VOLUME 16  // � a tick has changed a volume
//#define TICK_FLAG_BUY 32     // � a tick is a result of a buy deal
//#define TICK_FLAG_SELL 64    // � a tick is a result of a sell deal
// redifinition for my own use - seams useless so far
#define TICK_FLAG_BUY 1     // � a tick is a result of a buy deal
#define TICK_FLAG_SELL -1    // � a tick is a result of a sell deal

int64_t ReadTicks(std::vector<MqlTick> *mqlticks, 
    std::string filename, size_t nticks);

void fixArrayTicks(MqlTick* ticks, size_t nticks);

// return index on ring buffer / std::vector for
// for first tick with time_ms bigger or equal than or -1 not finding
size_t MqltickTimeGtEqIdx(std::vector<MqlTick> ticks, unixtime_ms time);


// Messed ticks from Meta5 only on every tick
// use only every tick based on real ticks

// circular buffer version
// 10k ticks maximum downloaded every time Refresh is called
// 1 second time-frame suggested using OnTimer
class BufferMqlTicks : public buffer<MqlTick>
{
protected:
  std::string m_symbol;
  // will come from metatrader already allocated
  MqlTick *m_mt5ticks; 
  int m_mt5ncopied; // last count of ticks sent by metatrader 
  
  // sync check 
  bool m_scheck; // security check of sync with data server
  unixtime_ms m_request; // time of/for last/next request of ticks by MT5
  int m_nnew; // number of new ticks added
  double m_perc_lost; // lost ticks in percentage (number of lost)/(number received) - last 

//////////////////////////////////////////////

  // secury check of sync with server of ticks
  // and seek to begin of new ticks
  bool seekBeginMt5Ticks();

public:
    // only one callback
    std::function<void(BufferMqlTicks*)> m_event_newticks;

    // call all event callback functions subscribed
    void OnNewTicks() {
        if (m_event_newticks)
            m_event_newticks(this);
    }

    // on new ticks call back function
    void AddOnNewTicks(std::function<void(BufferMqlTicks*)> onnewticks) {
        m_event_newticks = onnewticks;
    }

  // Buffer size for MqlTicks is the only that can be different from the rest
  // if it is small for example when operating Money Bars as a indicator
  // and for example 15MM arrive
  // bars will be creted only for last BUFFERSIZE < 15 MM
  // 20 MM seams reasonable, for 10/15 days of ticks 
  // think this should be changed with a loop, with max buffersize 
  // creating moneybars...

  BufferMqlTicks() : buffer<MqlTick>(MAX_TICKS) {
      m_scheck = false;
      m_mt5ncopied = 0;
      m_nnew = 0;
      m_request = 0;
      m_mt5ticks = NULL;
      m_perc_lost = 0; 
  };

  // new ticks added recently
  buffer<MqlTick>::iterator LastBegin() {
      return this->end() - m_nnew;
  }

  buffer<MqlTick>::iterator End() {
      return this->end();
  }

  void Init(std::string symbol);

  // will be called somehow by mt5
  unixtime_ms Refresh(MqlTick *mt5_pmqlticks, int64_t mt5_ncopied, double *perc_lost); 

  unixtime_ms innerRefresh(MqlTick* mt5_pmqlticks, int64_t mt5_ncopied, double* perc_lost);

};


void SaveTicks(BufferMqlTicks* ticks, std::string filename);

bool isInFile(BufferMqlTicks* ticks, std::string filename);