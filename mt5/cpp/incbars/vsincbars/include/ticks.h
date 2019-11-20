#pragma once

#include "buffers.h"
#include <string>
#include <iostream>

typedef long datetime;

typedef struct mqltick
{
    datetime     time;          // Time of the last prices update 1.
    double       bid;           // Current Bid price
    double       ask;           // Current Ask price
    double       last;          // Price of the last deal (Last)
    long        volume;        // Volume for the current Last price
    long         time_msc;      // Time of a price last update in milliseconds == 1. but with milliseconds precision
    int         flags;         // Tick flags
    double       volume_real;   // Volume for the current Last price with greater accuracy
} MqlTick;

// 2      TICK_FLAG_BID �  tick has changed a Bid price
// 4      TICK_FLAG_ASK  � a tick has changed an Ask price
// 8      TICK_FLAG_LAST � a tick has changed the last deal price
// 16     TICK_FLAG_VOLUME � a tick has changed a volume
// 32     TICK_FLAG_BUY � a tick is a result of a buy deal
// 64     TICK_FLAG_SELL � a tick is a result of a sell deal



const int Max_Tick_Copy = 10e3;

// circular buffer version
// 10k ticks maximum downloaded every time Refresh is called
// 1 ms time-frame suggested using OnTimer
class CCBufferMqlTicks : public CCBuffer<MqlTick>
{
protected:
    std::string m_symbol;
	int m_nnew;
public:
	int gticks = 0; // global counter of ticks
	CCBufferMqlTicks(std::string symbol);
	int nNew(); // number of new ticks after calling Refresh()
	int Refresh(std::vector<MqlTick>::iterator start, std::vector<MqlTick>::iterator end);
	int indexesNewTicks(int& start1, int& end1,
		int& start2, int& end2);
};

