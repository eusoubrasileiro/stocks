#include "stdbars.h"

#define SECS12H 12*3600 // 12 hours in seconds
#define SECSBAR 60 // 60 seconds bars 
// need to change bellow if changed to 30/15 seconds bar


// add one tick and create a bar or none
// return 1 or 0
// identical bars won't be created
// it means it's expected skipped minutes
size_t StdBarBuffer::AddTick(MqlTick tick) {
    m_nnew = 0;
    ///////////////////////////
    if (tick.volume > 0) { // there was a deal
        secs_prevbar = tick.time - m_bar.time; // time diff to current bar open time
        if (secs_prevbar < SECSBAR) { // 1. (most commom) inside same bar time span 
            // not asserting > 0            
            m_bar.high = std::max(m_bar.high, tick.last);
            m_bar.low = std::min(m_bar.low, tick.last);
        }
        else // >= SECSBAR
        if (secs_prevbar < SECS12H) { // 2. (a bit less commom) 
            // start a new bar and close last bar (identical bars won't be created)
            add(m_bar);
            m_nnew = 1;
            // convert to the second the define the begin of this new minute
            m_bar.time += ( (unixtime) (secs_prevbar/SECSBAR) )*SECSBAR;  // need to change this if not 60 seconds bars 
            m_bar.high = m_bar.low = tick.last;            
        } // >= SECS12H
        else { // 3. (uncommom a new day tick)
            // crossed to a new day (comparing with previous)
            // control to not have bars with ticks of different days
            AddLast();
            // reset everything for a new day has started
            // get ctime sctruct again to get first minute
            gmtime_s(&ctime, &tick.time);
            // get the start time (unix_time) for this new bar
            // remove seconds so it's in the begin of this minute
            m_bar.time = tick.time - ctime.tm_sec; // need to change this if not 60 seconds bars            
            m_bar.high = -DBL_MAX; // reset min, max 
            m_bar.low = DBL_MAX;  
        }
    }
    return m_nnew;
}

// must process ticks sequentially
size_t StdBarBuffer::AddTicks(buffer<MqlTick>::iterator start,
    buffer<MqlTick>::iterator end) {
    size_t nnew = 0;
    for (auto element = start; element != end; element++)
        nnew += AddTick(*element);
    m_nnew = nnew; // overwrite internal added increment    
    return m_nnew;
}

// add the last bar -> except if it's the first tick of all
// can be used to add the last bar not finished due not having 
// a new tick with time greater than it
inline void StdBarBuffer::AddLast() {
    if (m_bar.time != 0) { 
        add(m_bar);
        m_nnew = 1;
    }
}

