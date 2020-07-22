
#include "moneybars.h"
#include <algorithm>    // std::max
#include <ctime>

// calculate percentile simple
double percentile(std::vector<double> data, double perc, bool sort) {
    // inplace using the input array
    if(sort)
        std::sort(data.begin(), data.end());
    size_t asize = data.size();
    auto n = (size_t) std::max(std::round(perc * asize + 0.5), 2.0);
    return data[n - 2];
}

// index based
size_t MoneyBarBuffer::BeginNewBarsIdx() {
    return size()-m_nnew;
}

MoneyBarBuffer::MoneyBarBuffer(){
    MoneyBarBuffer(BUFFERSIZE);
    cuid = 0;
    m_count_money = 0; // count_money amount to form 1 money bar
    m_nnew = 0;
    m_pvs = m_vs = 0; // current v. weighted price for this bar
    m_moneybarsize = DBL_EMPTY_VALUE;
    m_point_value = 0;
    m_bar.smsc = m_bar.emsc = 0;
    m_bar.time.tm_yday = -1;  // first dtp calc. needs this    
    // so first dtp gets zeroed as if crossing to a new day
    m_bar.inday = -1; // inside operational day information undef at first
    // default operational window
    m_start_hour = 10;
    m_end_hour = 16.5;
    // ctime = 0;
    m_bar.netvol = 0;
    m_bar.min = m_bar.max = DBL_EMPTY_VALUE;
    // m_ticks // has a default constructor that uses MAX_TICKS
}

// copy constructor, incomplete
//MoneyBarBuffer::MoneyBarBuffer(const MoneyBarBuffer &moneybars) {
//    m_nnew = moneybars.m_nnew;
//    m_hash = moneybars.m_hash;
//    *this = moneybars; // copy circular buffer of money bars using inner copy constructor
//    uidtimes = moneybars.uidtimes;
//    /// need to change design probably
//    // something messyy here
//}

void MoneyBarBuffer::Init(double tickvalue, double ticksize, double moneybarsize){
    m_point_value = tickvalue / ticksize;
    m_moneybarsize = moneybarsize;
    auto hasher = std::hash<std::string>(); // to create a unique hash for this money bar
    m_hash = hasher(std::to_string(m_point_value) + std::to_string(m_moneybarsize) + std::to_string(std::time(0)));
}

// set operational window for flag '.inday' inside money bars
void MoneyBarBuffer::SetHours(float start_hour, float end_hour) {
    m_start_hour = start_hour;
    m_end_hour = end_hour;
}

// add one tick and create as many money bars as needed (or 0)
// return number created
size_t MoneyBarBuffer::AddTick(MqlTick tick) {
    m_nnew = 0;

    ///////////////////////////
    if (tick.volume > 0) { // there was a deal
        // control to not have bars with ticks of different days
        gmtime_s(&ctime, &tick.time);
        auto day_hour = (float) ctime.tm_hour + ctime.tm_min / 60. + ctime.tm_sec / 3600.;
        // crossed to a new day (comparing with previous)
        if (ctime.tm_yday != m_bar.time.tm_yday) {
            // clean up (ignore) previous data
            // for starting a new bar
            m_bar.nticks = 0;
            m_bar.time = ctime;
            m_bar.inday = (day_hour > m_start_hour && day_hour < m_end_hour)? 1 : 0;
            m_bar.smsc = tick.time_msc;
            m_bar.dtp = 0; // no previous bar
            m_count_money = 0;
            m_pvs = m_vs = 0;
            m_bar.netvol = 0;
            m_bar.min = DBL_MAX;
            m_bar.max = -DBL_MAX;
            m_wprices.clear();
        }
        else // same day
        if (m_bar.nticks == 0) { // new bar
            m_bar.time = ctime; // entry time for this bar
            m_bar.inday = (day_hour > m_start_hour && day_hour < m_end_hour) ? 1 : 0;
            m_bar.smsc = tick.time_msc;
            // time difference in seconds to previous bar
            m_bar.dtp = 0.001 * (m_bar.smsc - m_bar.emsc);
            m_pvs = m_vs = 0;
            m_bar.netvol = 0;
            m_bar.min = DBL_MAX;
            m_bar.max = -DBL_MAX;
            m_wprices.clear();
        }        
        m_bar.netvol += tick.flags*(tick.volume_real * tick.last * m_point_value); // buy-sell volume 
        m_count_money += tick.volume_real * tick.last * m_point_value;
        m_bar.min = std::min(m_bar.min, tick.last);
        m_bar.max = std::max(m_bar.max, tick.last);
        m_pvs += tick.volume_real * tick.last; // summing (prices * volumes)
        m_vs += tick.volume_real; // summing (volumes)
        m_wprices.push_back(m_pvs/m_vs); // current weighted price
        m_bar.nticks++;
        auto sametick = false;
        while (m_count_money >= m_moneybarsize) { 

            // may need to create many bars for one same tick
            // in this case nticks == 0 for the second one and so forth
            // start/exit time also == 0
            if (!sametick) {
                m_bar.wprice = m_wprices.back(); // last weighted price
                m_bar.wprice10 = percentile(m_wprices, 0.1, true);
                m_bar.wprice90 = percentile(m_wprices, 0.9, false);
                m_bar.emsc = tick.time_msc; // exit time
                m_bar.netvol /= m_pvs * m_point_value; // net volume negotiated divided by total volume (-1/1+)
            }
            m_bar.uid = cuid++;
            uidtimes.add(m_bar.uid);
            add(m_bar);
            /////////
            // case when multiple bars created from same tick
            // start and exit time are the same
            m_bar.smsc = tick.time_msc;
            m_bar.dtp = 0;
            //m_pvs = m_vs = 0;
            //m_bar.netvol = 0; -- all will have same vbuysell
            m_bar.nticks = 0;            
            // wil be overwritten when a new tick
            // createas a new bar
            m_count_money -= m_moneybarsize;
            m_nnew++;
            sametick = true; // if, keeps on this loop
        }
    }
    return m_nnew;
}

// add ticks for metatrader support?
size_t MoneyBarBuffer::AddTicks(buffer<MqlTick>::iterator start,
    buffer<MqlTick>::iterator end) {
    size_t nnew = 0;
    for (auto element = start; element != end; element++)
        nnew += AddTick(*element);
    m_nnew = nnew; // overwrite internal added increment
    if (m_nnew > 0) OnNewBars();
    return m_nnew;
}

// add ticks for python support
size_t MoneyBarBuffer::AddTicks(const MqlTick *cticks, int size)
{
    size_t nnew = 0;
    for (int i =0; i<size; i++)
        nnew += AddTick(cticks[i]);
    m_nnew = nnew; // overwrite internal added increment
    if (m_nnew > 0) OnNewBars();
    return m_nnew;
}

// or if it comes after that uid
//inline bool CmpMoneyBarGreaterUid(const MoneyBar& a, const unsigned long long uid) {
//    return a.uid > uid;
//}
// then you can use
// return index on ring buffer or -1 on not finding
size_t MoneyBarBuffer::Search(uint64_t uid)
{
    auto iter = std::lower_bound(begin(), end(), uid, cmpMoneyBarSmallUid);
    return (iter == end()) ? -1 : iter - begin();
}

// std::lower_bound
// Returns an iterator pointing to the first element in the range [first,last)
// which does not compare less than val.
// The elements are compared using operator< or comparator function (cmpMoneyBarSmallUid)
// since uid
// 1. does not repeat
// 2. are sorted
// above will get the index of the uid on the buffer of bars

// find first money bar with start time bigger or equal than
inline bool cmpMoneyBarStime(const MoneyBar& a, int64_t smsc) {
    return a.smsc < smsc;
}

// return index on ring buffer for
// for first money bar with start time bigger or equal than or -1 not finding
size_t MoneyBarBuffer::SearchStime(int64_t smsc)
{
    auto iter = std::lower_bound(begin(), end(), smsc, cmpMoneyBarStime);
    return (iter == end()) ? -1 : iter - begin();
}



//
// this should be created using a MoneyBarBuffer class
//
//// Create Array of Money/Dollar/Real Bars from Array of MqlTicks
//// moneybarsize value in $$ to form a bar
//int MoneyBars(MqlTick ticks[], double tickvalue, double moneybarsize, MoneyBar bars[]) {
//    double count_money = 0;
//    int i, ibars, nticks = ArraySize(ticks);
//    ArrayResize(bars, nticks); // start big resize after
//    ibars = 0;
//    for (i = 0; i < nticks; i++) {
//        count_money += ticks[i].volume_real * ticks[i].last * tickvalue;
//        while (count_money >= moneybarsize) { // may need to create many bars for one tick
//            if (ibars == nticks)// ibars may explode if moneybarsize too small
//                return -1;
//            bars[ibars].bid = ticks[i].bid;
//            bars[ibars].ask = ticks[i].ask;
//            bars[ibars].last = ticks[i].last;
//            bars[ibars++].time_msc = ticks[i].time_msc;
//            count_money -= moneybarsize;
//        }
//
//    }
//    ArrayResize(bars, ibars);
//    return ibars;
//}
