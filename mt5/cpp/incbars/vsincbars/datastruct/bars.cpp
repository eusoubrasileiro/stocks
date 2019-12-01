#include "bars.h"

int MoneyBarBuffer::indexesNewBars(int parts[]) {
    return indexesData(Count() - m_nnew, m_nnew, parts);
}

// copy last data of new bars m_nnew
// to local arrays
void MoneyBarBuffer::RefreshArrays() {
    int parts[4];
    int i, j, k, nparts, start, end;
    nparts = indexesNewBars(parts);
    for (i = 0, k = 0; i < nparts; i++) {
        // money bars circular buffer copy in two parts
        start = parts[i * 2];
        end = parts[1 + i * 2];
        for (j = start; j < end; j++, k++) {
            m_last[k] = m_data[j].avgprice; // moneybar.last price
            m_times[k] = m_data[j].uid; //moneybar.uid
        }
    }
}

// local arrays have max size equal buffer size
void MoneyBarBuffer::SetSize(int size) {
    CCBuffer::SetSize(size);
    m_last.resize(size);
    m_times.resize(size);
}


MoneyBarBuffer::MoneyBarBuffer(double tickvalue, double ticksize, double moneybarsize, 
        int buffersize) {
    m_point_value = tickvalue / ticksize;
    m_moneybarsize = moneybarsize;
    m_count_money = 0; // count_money amount to form 1 money bar
    m_nnew = m_nticks = 0;
    m_pvs = m_vs = 0; // v. weighted price for this bar
    SetSize(buffersize);
}

// add one tick and create as many money bars as needed (or 0)
// return number created
int MoneyBarBuffer::AddTick(MqlTick tick) {
    m_nnew = 0;
    if (tick.volume > 0) { // there was a deal        
        // control to not have bars with ticks of different days
        cwday = timestampWDay(tick.time);
        if (m_bar.nticks == 0) {// entry time for this bar
            m_bar.smsc = tick.time_msc;
            m_bar.wday = cwday;
        }
        else // just crossed to a new day
        if(cwday != m_bar.wday){ 
            // clean up (ignore) previous data 
            // for starting a new bar
            m_bar.nticks = 0;
            m_bar.smsc = tick.time_msc;
            m_bar.wday = cwday;
            m_count_money = 0;
            m_pvs = m_vs = 0;
        }
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
            Add(m_bar);
            m_pvs = m_vs = 0;
            m_bar.emsc = m_bar.smsc = 0;
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
    for (auto element = start; element != end; ++element)
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
inline bool cmpMoneyBarSmallUid(const MoneyBar& a, unsigned long long uid) {
    return a.uid < uid;
}
// or if it comes after that uid
//inline bool CmpMoneyBarGreaterUid(const MoneyBar& a, const unsigned long long uid) {
//    return a.uid > uid;
//}
// then you can use 
// std::lower_bound(m_data.begin(), m_data.end(), value, CmpMoneyBarSmallUid) - m_data.begin()
// to get the index where it is equal

// return index on ring buffer for the uid
int MoneyBarBuffer::Search(unsigned long long uid)
{
    int nparts, start, end, ifound, i, parts[4];
    ifound = -1;
    nparts = indexesData(0, Count(), parts);
    for (i = 0; i < nparts; i++) {
        start = parts[i * 2];
        end = parts[1 + i * 2];
        ifound = _Search(uid, start, end);
        if (ifound != -1)
            break;
    }
    // convert to start based index (ring buffer index)
    return (ifound != -1) ? toIndex(ifound) : ifound;
}

// std::lower_bound
// Returns an iterator pointing to the first element in the range [first,last) 
// which does not compare less than val.
// The elements are compared using operator< or comparator function (cmpMoneyBarSmallUid)
// since uid 
// 1. does not repeat
// 2. are sorted
// above will get the index of the uid on the buffer of bars

int MoneyBarBuffer::_Search(unsigned long long uid, int start, int end) {
    auto iter = std::lower_bound(m_data.begin() + start, m_data.begin() + end,
        uid, cmpMoneyBarSmallUid);
    // in case did not find returns -1
    return (iter == m_data.end())? -1 : iter - m_data.begin();
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