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
/// <returns>Events, status</returns>
/// status = true means event should be removed from unlabbeled Events
/// status = false means event should NOT be removed 
std::pair<std::vector<Event>,bool> LabelEvent(Event &event, double mtgt, int barrier_str, int nbarriers, int barrier_inc)
{
    std::vector<Event> events;
    double _retrn; // return in % decimal
    auto start_idx = m_bars->Search(event.twhen);

    if (start_idx == -1) // this event should be removed it will never be labbeled
        return std::make_pair(events, true);    
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
        if (vertbarrier > m_bars->size()) // cannot label anymore, dont do anything
            return std::make_pair(events, false);  // it will be labbeled after

        // since vertical barrier are increasing dont need to restart idx
        for(; idx < m_bars->size(); idx++){
            if (check_sltp_hit(&m_bars->at(idx), event, open_price, sltp, &_retrn)) {
                // horizontal barrier hit, cannot continue further vertical barriers 
                events.push_back(event);                
                return std::make_pair(events, true);
            }
            // vertical barrier hit, can continue other vertical barriers 
            if (idx >= start_idx + vertbarrier) {
                event.ret = _retrn;
                event.tdone = m_bars->at(idx).uid;
                // TODO: test wether it's best to labell 0 or sign of return
                event.side = (_retrn > 0) ? 1 : -1; // set side by return sign, 0 goes to -1
                events.push_back(event);                
                break;
            }
        }
        CLEAN_EVENT_LABEL(event); // ready for next barrier check
    }

    return std::make_pair(events, true);;
}


// correction between uid's and current buffer index actual uid position on buffer is : uid - BUFFER_UID_OFFSET
#define BUFFER_UID_OFFSET m_bars->at(0).uid
    // uid starts at 0 alligned with bars (and all buffers) BUT
    // when buffers gets full allignment is lost
    // but a constant offset exists betwen uid and indexes of buffers
  

uint64_t uid_offset = 0;
uint64_t bufsize = 0;
uint64_t idx_valid = -1;
uint64_t min_bars = 1;

// uid starts at 0 alligned with bars (and all buffers) BUT
// when buffers gets full allignment is lost.
// but a constant offset exists betwen uid and indexes of buffers
// m_bfoffset = m_bars->at(0).uid - m_bars->Search(m_bars->at(0).uid);

// return labelled and featured events
std::pair<std::vector<Event>, std::vector<std::vector<double>>> 
LabelEvents(std::vector<Event> & events, double mtgt,
                               int barrier_str, int nbarriers, int barrier_inc, int batch_size){

    std::vector<std::vector<double>> X_feats;
    std::vector<Event> Events_feats;

    // update, further use on FillinXFeatures
    uid_offset = BUFFER_UID_OFFSET; 
    bufsize = m_bars->size();
    // at least one valid sample on indicator w. wider window
    if (m_CumSumi->vCount()==0)
        return make_pair(Events_feats, X_feats); // cannot label because cannot created features

    // widest window needed should be here 
    // CumSum on SADF has the widest window 
    // we need to get the 
    // min_bars = minimum number of bars to calculated all indicators 
    // at least 1 not nan sample
    min_bars = m_CumSumi->Window();    
    // the start index of valid (not nan) above-1 (index)
    idx_valid = std::distance(m_CumSumi->Begin(0), m_CumSumi->vBegin(0));
    auto batch_features = 11; // 11 features in the past
    auto add_features = 3; // aditional features not in batch only for Second Classifier (Binary)
    std::vector<double> X_feat;
    X_feat.reserve(batch_size * batch_features + add_features);
    
    for (auto ev = events.begin(); ev != events.end();) {
        auto labeleds_status = LabelEvent(*ev, mtgt, barrier_str, nbarriers, barrier_inc);
        auto labeleds = std::get<0>(labeleds_status);
        if (std::get<1>(labeleds_status)) // labelled/done. Must be removed!
           ev = events.erase(ev);
        else
            ++ev; // avoid increment past the end
        
        for (auto evl = labeleds.begin(); evl != labeleds.end(); evl++) {
            if (FillinXFeatures(*evl, X_feat, batch_size)) {
                Events_feats.push_back(*evl);
                X_feats.push_back(X_feat);
                X_feat.clear();
            }
        }

    }

    return make_pair(Events_feats, X_feats);
}


bool FillinXFeatures(Event &event, std::vector<double>& X, int batch_size){
    // find when current position on all buffers (have same size)
    // for the signal that triggered this event
    // uidxtime is allways sorted - dont need to find only correct by offset
    auto bfidxsg = event.twhen - uid_offset;

    // cannot assembly X feature train with such an old signal
    // not in buffer anymore - such should not be used
    // since it is unsigned check is with upper bounds
    if (bfidxsg > bufsize)
        return false;

    // I want to include the signal the originates this xypair
    bfidxsg += 1; // since it is < on for loop needs a +1 here
    auto batch_start_idx = bfidxsg - batch_size;
    // not enough : signal, indicator in buffer to form a batch of features
    // cannot create a X feature vector and will never will
    if (batch_start_idx > bufsize ||
        batch_start_idx < idx_valid) // batch_start at invalid data (nan), cannot use this event
        return false;
    
    // something like
    // double[Expert_BufferSize][n_features] would be awesome
    // easier to copy to X also to create cross-features
    // using a constant on the second dimension is possible
    for (int timeidx = batch_start_idx; timeidx < bfidxsg; timeidx++) {        
        X.push_back(m_bars->at(timeidx).dtp);
        X.push_back(m_bars->at(timeidx).wprice);
        // fracdif feature
        X.push_back(m_FdMb->Buffer()[timeidx]);
        // SADF feature
        X.push_back(m_SADFi->Buffer()[timeidx]);
        // CumSum on SADF
        X.push_back(m_CumSumi->Buffer()[timeidx]);
        // average volatility of returns
        X.push_back(m_StdevMbReturn->Buffer()[timeidx]);
        // time to form a bar in seconds
        auto dt = (m_bars->at(timeidx).emsc - m_bars->at(timeidx).smsc) / 1000.;
        X.push_back(dt);
        // number of ticks to form a bar
        auto nticks = m_bars->at(timeidx).nticks;
        X.push_back(nticks);
        // net-volume looks 
        X.push_back(m_bars->at(timeidx).netvol);
        // cross features
        // 9 - diff of times to form this bar with the previous
        // 10 - diff of ticks to form this bar with the previous
        X.push_back(dt - (m_bars->at(timeidx - 1).emsc - m_bars->at(timeidx - 1).smsc) / 1000.);
        X.push_back(nticks - m_bars->at(timeidx - 1).nticks);
        // some indicators might contain EMPTY_VALUE == DBL_MAX ??values
        // sklearn throws an exception because of that
        // anyhow that's not a problem for the code since the python exception
        // is catched behind the scenes and a valid model will only be available
        // once correct samples are available
    }    
    // First classifier gives the side buy or sell
    // Last 3 - features based on event 
    // These only be used for Second classifier 
    // should I go in or not? Binary classifier
    X.push_back(event.side);
    X.push_back(event.sltp);
    X.push_back(event.vertbar);
    
    // information span for this training sample
    // to avoid information leakage when cross-validate model    
    event.inf_start = event.twhen - ((uint64_t) batch_size - 1) - ((uint64_t) min_bars - 1);
    event.inf_end = event.tdone;

    return true; // suceeded
}