#pragma once
#include "events.h"
#include "indicators.h"

// Code name convention following totally the Book 
// Advances in Financial Machine Learning
//
// Triple Barrier Labelling with Side Definition 

// Based on Chapter : Part 1 : Data Analysis - 3 Labelling
// Based on an event
// There are several possibilities for setting the side (buy/sell)
// for one position based on the tripple barrier method
// lets use the simplest first, SL/TP on both sides are simetric
// for example we assume its a long (buy)
// whatever get's hit first gives the side
// if stop-loss then it was instead a sell and vice-versa
//

// things that could vary creating thousands of Event's from a seed Event
//  1. stop-loss and take profit reference target value 0.01%, 0.03% etc
//  2. stop-loss and take profit target assymetric 3:1, 4:1 etc.
//  3. vertical barrier 2Hours, 10 money bars, 2 days etc...

// I will use 3. above to create multiple events 
// there will be multiple events only if sltp not hit 

inline bool check_sltp_hit(MoneyBar *bar, Event& event, double open_price, double sltp, double *_retrn) // return in % decimal
{
    double prices[3]; // o h l correct sequence of values on current money bar
    // unpack 3 values inside each bar
    prices[0] = bar->open;
    if (bar->highfirst) { prices[1] = bar->high; prices[2] = bar->low; }
    else { prices[1] = bar->low; prices[2] = bar->high; }

    for (auto price = prices; price != &prices[3]; price++) {
        *_retrn = *price / open_price - 1.;
        if (*_retrn >= sltp) { // a real long/buy, take profit of long reached
            event.side = 1;
            event.ret = *_retrn;
            event.tdone = bar->uid;
            return true;
        }
        else
            if (*_retrn <= -sltp) { // stop-loss reached it was a sell in fact, take profit of short reached
                event.side = -1;
                event.ret = *_retrn;
                event.tdone = bar->uid;
                return true;
            }
    }
    return false;
}

/// <summary>
/// From a seed event (unlabelled)
/// creates multiple labelled events using tripple barrier method
/// using `nbarriers` vertical barriers.
/// Side is found by sign of return of first hit stop-loss or take-profit.
/// Stop-loss, Take-profit are defined by volatility of returns.
/// </summary>
/// <param name="event">seed event to be labeled</param>
/// <param name="mtgt">multiplier for target</param>
/// <param name="barrier_str">first vertical barrier (in bars)</param>
/// <param name="nbarriers">number of vertical barriers (in bars)</param>
/// <param name="barrier_inc">increment between vertical barriers (in bars)</param>
/// <returns></returns>
std::vector<Event> LabelEvent(Event &event, double mtgt, int barrier_str, int nbarriers, int barrier_inc)
{
    std::vector<Event> events;
    double _retrn; // return in % decimal
    auto start_idx = m_bars->Search(event.twhen);

    if (start_idx == -1)
        return events;
    // idx != -1 : it exists on all buffers
    // assuming long first target profit (in return % decimal)
    // symetric stop loss and take profit
    auto sltp = m_StdevMbReturn->At(start_idx)* mtgt;
    //auto ibar = m_bars->begin() + idx;
    auto open_price = m_bars->at(start_idx).wprice;
    event.sltp = sltp;

    // A REAL PROBLEM. 
    // get 'real' price of execution of this signal
    // first bar considering the operational time delay - 
    // Should I deal with that here? -- I dont think so... since 
    // 1. I use weighted average price
    // 2. I want to see if low frequency works first
    // first bar with time >= expert delay included
    // int start_bfidx = m_bars->SearchStime(m_bars->at(bfidx).emsc + Expert_Delay);
    
    auto idx = start_idx;
    // vertical barriers, in bars (TODO: for later time...)
    for (auto vertbarrier = barrier_str; vertbarrier < barrier_str + barrier_inc*nbarriers; vertbarrier+=barrier_inc) {
        // testing multiple vertical barriers for each event observed
        event.vertbar = vertbarrier;
        // checking if it's possible to label it (enough bars on buffer)
        if (vertbarrier > m_bars->size()) // cannot anymore
            break;
        // since vertical barrier are increasing dont need to restart idx
        for(; idx < m_bars->size(); idx++){
            if (check_sltp_hit(&m_bars->at(idx), event, open_price, sltp, &_retrn)) {
                // horizontal barrier hit, cannot continue further vertical barriers 
                events.push_back(event);                
                return events;
            }
            // vertical barrier hit, can continue other vertical barriers 
            if (idx >= start_idx + vertbarrier) {
                event.ret = _retrn;
                event.tdone = m_bars->at(idx).uid;
                event.side = (_retrn > 0) ? 1 : -1; // set side by return sign, 0 goes to -1
                events.push_back(event);                
                break;
            }
        }
        CLEAN_EVENT_LABEL(event); // ready for next barrier check
    }

    return events;
}

// uid starts at 0 alligned with bars (and all buffers) BUT
// when buffers gets full allignment is lost.
// but a constant offset exists betwen uid and indexes of buffers
// m_bfoffset = m_bars->at(0).uid - m_bars->Search(m_bars->at(0).uid);

std::vector<Event> LabelEvents(std::vector<Event>::iterator begin, std::vector<Event>::iterator end, double mtgt,
                               int barrier_str, int nbarriers, int barrier_inc){
    std::vector<Event> labeleds;
    for (auto v = begin; v != end; v++) {
        auto newlabeleds = LabelEvent(*v, mtgt, barrier_str, nbarriers, barrier_inc);
        labeleds.insert(labeleds.end(), newlabeleds.begin(), newlabeleds.end());
    }
    return labeleds;
}