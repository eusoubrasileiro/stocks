#include "bars.h"

// iterator 'like' begin of new bars
MoneyBar *MoneyBarBuffer::BeginNewBars() {
    return &(end()-(m_nnew-1))[0];
}

// index based
int MoneyBarBuffer::BeginNewBarsIdx() {
    return size()-m_nnew;
}

MoneyBarBuffer::MoneyBarBuffer() {
    cuid = 0;
    m_count_money = 0; // count_money amount to form 1 money bar
    m_nnew = m_nticks = 0;
    m_pvs = m_vs = 0; // v. weighted price for this bar
    m_moneybarsize = DBL_EMPTY_VALUE;
    m_point_value = 0;
}

// return only new ... data added
void MoneyBarBuffer::RefreshArrays() {
    auto bar = BeginNewBars();
    for(int i=0; i<m_nnew; i++)
        new_avgprices[i] = bar[i].avgprice;
}

void MoneyBarBuffer::Init(double tickvalue, double ticksize, double moneybarsize){
    m_point_value = tickvalue / ticksize;
    m_moneybarsize = moneybarsize;
}

// add one tick and create as many money bars as needed (or 0)
// return number created
int MoneyBarBuffer::AddTick(MqlTick tick) {
    m_nnew = 0;
    // get high and low bid-ask 
    if (tick.ask > m_bar.askh)
        m_bar.askh = tick.ask;
    else
        if (tick.ask < m_bar.askl)
            m_bar.askl = tick.ask;
    if (tick.bid > m_bar.bidh)
        m_bar.bidh = tick.bid;
    else
        if (tick.bid < m_bar.bidl)
            m_bar.bidl = tick.bid;
    ///////////////////////////
    if (tick.volume > 0) { // there was a deal        
        // control to not have bars with ticks of different days
        ctime = *gmtime(&tick.time);
        if (m_bar.nticks == 0) {// entry time for this bar
            m_bar.smsc = tick.time_msc;
            m_bar.bidh = m_bar.bidl = tick.bid;
            m_bar.askh = m_bar.askl = tick.ask;            
        }
        else // just crossed to a new day
        if(ctime.tm_wday != m_bar.time.tm_wday){ 
            // clean up (ignore) previous data 
            // for starting a new bar
            m_bar.nticks = 0;
            m_bar.smsc = tick.time_msc;
            m_count_money = 0;
            m_pvs = m_vs = 0;
            m_bar.bidh = m_bar.bidl = tick.bid;
            m_bar.askh = m_bar.askl = tick.ask;
        }
        m_bar.time = ctime;
        m_bar.nticks++;
        m_count_money += tick.volume_real * tick.last * m_point_value;        
        while (m_count_money >= m_moneybarsize) { // may need to create many bars for one tick
            // in this case nticks == 0 for the second one and so forth
            // start/exit time also == 0
            m_pvs += tick.volume_real * tick.last; // summing (prices * volumes)
            m_vs += tick.volume_real; // summing (volumes)
            m_bar.avgprice = m_pvs/m_vs;
            m_bar.emsc = tick.time_msc; // exit time
            m_bar.uid = cuid++;
            uidtimes.add(m_bar.uid);
            add(m_bar);
            m_pvs = m_vs = 0;
            m_bar.nticks = 0;
            m_count_money -= m_moneybarsize;
            m_nnew++;
        }
    }
    return m_nnew;
}

// add ticks for python support
int MoneyBarBuffer::AddTicks(std::vector<MqlTick>::iterator start, std::vector<MqlTick>::iterator end) {
    int nnew = 0;
    for (auto element = start; element != end; element++)
        nnew += AddTick(*element);
    m_nnew = nnew; // overwrite internal added increment
    return m_nnew;
}

// add ticks for metatrader support
int MoneyBarBuffer::AddTicks(const MqlTick *cticks, int size)
{
    int nnew = 0;
    for (int i =0; i<size; i++)
        nnew += AddTick(cticks[i]);
    m_nnew = nnew; // overwrite internal added increment
    return m_nnew;
}

// MoneyBar isnt a class so I will not add this operator bellow on it or anything else
//inline bool operator<(const MoneyBar& a, const MoneyBar& b) or
// inline bool operator<(const MoneyBar& a, unsigned long long uid)
//{
//    return a.uid < b.uid;
//    return a.uid < uid;
//}
// we will use this comparator
// compare a MoneyBar's to a uid to see if it comes before it or not
inline bool cmpMoneyBarSmallUid(const MoneyBar& a, uint64_t uid) {
    return a.uid < uid;
}
// or if it comes after that uid
//inline bool CmpMoneyBarGreaterUid(const MoneyBar& a, const unsigned long long uid) {
//    return a.uid > uid;
//}
// then you can use 
// return index on ring buffer or -1 on not finding
int MoneyBarBuffer::Search(uint64_t uid)
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
inline bool cmpMoneyBarStime(const MoneyBar& a, uint64_t smsc) {
    return a.smsc < smsc;
}

// return index on ring buffer for 
// for first money bar with start time bigger or equal than or -1 not finding
int MoneyBarBuffer::SearchStime(uint64_t smsc)
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