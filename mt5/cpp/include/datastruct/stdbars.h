#pragma once

#include <algorithm>    // std::max
#include <ctime>
#include "buffers.h"
#include "ticks.h"

// standard 1 minute bar since it's easier
// to do everythin with this first
// used to classify signals
// created on the fly by ticks received

// Using only for tripple barrier method
// so will only fill high and low
#pragma pack(push, 2)
struct StdBar //MqlRates from MT5
{
    unixtime time;         // Period start time
    //double   open;         // Open deal price -- will not fill
    double   high;         // The highest deal price of the period
    double   low;          // The lowest deal price of the period
    //double   close;        // Close deal price -- will not fill - same as open of next bar
    //int64_t  tickv;        // Tick volume  -- will not fill
    //int      spread;       // Spread       -- will not fill
    //int64_t  realv;        // Trade volume -- will not fill
};
#pragma pack(pop)


// Visual Studio
// typedef unsigned int64_t size_t;
// ## 9, 223, 372, 036, 854, 775, 807
// UINT64_MAX 
//> 9 quintillion
// ---  I still need a circular buffer:
// + to avoid memory explodes
// + ticks amount is huge 


/// <summary>
/// this will be hugests size possible
/// memory limmited
/// </summary>
class StdBarBuffer : public buffer<StdBar>
{
    StdBar m_bar; // current bar being formed
    unixtime secs_prevbar; // diff time in seconds to begin of current bar
    size_t m_nnew; // number of bars formed on last AddTicks call
    tm ctime;

public:
    StdBarBuffer(size_t size) : buffer<StdBar>(size) 
    {
        // won't be filled coz I dont care
        // set current bar default value
        m_bar.time = 0;
        m_bar.high = -DBL_MAX;
        m_bar.low = +DBL_MAX;
        m_nnew = 0;
        ctime.tm_sec = 0; // quiet stupid warning
    };

    // add one tick and create or not stdbars
     // return number created
    size_t AddTick(MqlTick tick);
    // add ticks for metatrader support
    size_t AddTicks(buffer<MqlTick>::iterator start,
        buffer<MqlTick>::iterator end);

    // void AddBars(MqlTick tick);

    // add the last bar -> except if it's the first tick of all
    // can be used to add the last bar not finished due not having 
    // a new tick with time greater than it
    inline void AddLast(); // add last bar open but not finished
};


