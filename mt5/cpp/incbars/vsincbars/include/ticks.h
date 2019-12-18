#pragma once


#include "buffers.h"
#include <string>
#include <iostream>

typedef long datetime;

#pragma pack(push, 2)
struct MqlTick
{
    int64_t      time;          // Time of the last prices update 1.
    double       bid;           // Current Bid price
    double       ask;           // Current Ask price
    double       last;          // Price of the last deal (Last)
    int64_t      volume;        // Volume for the current Last price
    int64_t      time_msc;      // Time of a price last update in milliseconds == 1. but with milliseconds precision
    int          flags;         // Tick flags
    double       volume_real;   // Volume for the current Last price with greater accuracy
}; // sizeof is 60 bytes to avoid auto alignment to 64 by compiler #pragma pack(2) above is needed
#pragma pack(pop)

// 2      TICK_FLAG_BID �  tick has changed a Bid price
// 4      TICK_FLAG_ASK  � a tick has changed an Ask price
// 8      TICK_FLAG_LAST � a tick has changed the last deal price
// 16     TICK_FLAG_VOLUME � a tick has changed a volume
// 32     TICK_FLAG_BUY � a tick is a result of a buy deal
// 64     TICK_FLAG_SELL � a tick is a result of a sell deal


// Messed ticks from Meta5 will also come to C++
// here we fix then replacing by a file of correct ones

const int Max_Tick_Copy = 10e3;

// circular buffer version
// 10k ticks maximum downloaded every time Refresh is called
// 1 second time-frame suggested using OnTimer
class BufferMqlTicks : public buffer<MqlTick>
{
protected:
  string m_symbol;
  MqlTick m_copied_ticks[]; // fixed size number of ticks copied every x seconds
  int m_ncopied; // last count of ticks copied on buffer
  int m_nnew; // number of new ticks last copied
  bool scheck; // security check of sync with data server
  int64_t m_cmpbegin_time;
  int m_cmpbegin;
  int m_bgidx; // temp:: where the new ticks starts on array m_copied_ticks

  //////////////////////////////////////////////
  ////// backtest workaround for stupid mt5 fake ticks creation
  bool isbacktest;
  MqlTick m_refticks[]; // correct ticks without stupid modifications
  int m_refsize; // size of reference data
  // on volume etc by MT5 to fuc** with everything serious
  std::string m_refcticks_file;
  int m_refpos; // position on reference ticks file
  int m_trash_count;
  ////////////////////////////////////////////
  ////// for testing against Python/C++ since i need to
  ////// know boundaries of new ticks (since data comes in chuncks)
  int m_bound_ticks[];
  int ib_tick; // count of calls to AddRange

  bool correctMt5UnrealTicks();

  void loadCorrectTicks(string symbol);
//////////////////////////////////////////////
//////////////////////////////////////////////
  bool beginNewTicks();

public:

  BufferMqlTicks(void);

  BufferMqlTicks(std::string symbol);

  int nNew(){ return m_nnew; } // number of new ticks after calling Refresh()

  int Refresh(MqlTick *ticks, int count); // will be called somehow by mt5

};
