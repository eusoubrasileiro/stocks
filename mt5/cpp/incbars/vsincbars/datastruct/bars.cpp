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
            m_last[k] = m_data[j].last; // moneybar.last price
            m_times[k] = m_data[j].time_msc; //moneybar.time_msc
        }
    }
}

// local arrays have max size equal buffer size
void MoneyBarBuffer::SetSize(int size) {
    CCBuffer::SetSize(size);
    m_last.resize(size);
    m_times.resize(size);
}

MoneyBarBuffer::MoneyBarBuffer(double tickvalue, double ticksize, double moneybarsize) {
    m_point_value = tickvalue / ticksize;
    m_moneybarsize = moneybarsize;
    m_count_money = 0; // count_money amount to form 1 money bar
    m_nnew = 0;
}

// add one tick and create as many money bars as needed (or 0)
// return number created
int MoneyBarBuffer::AddTick(MqlTick tick) {
    m_nnew = 0;
    if (tick.volume > 0) { // there was a deal
        m_count_money += tick.volume_real * tick.last * m_point_value;
        while (m_count_money >= m_moneybarsize) { // may need to create many bars for one tick
            m_bar.bid = tick.bid;
            m_bar.ask = tick.ask;
            m_bar.last = tick.last;
            m_bar.time_msc = tick.time_msc;
            Add(m_bar);
            m_count_money -= m_moneybarsize;
            m_nnew++;
        }
    }
    return m_nnew;
}

// add ticks from bg_idx until size
int MoneyBarBuffer::AddTicks(std::vector<MqlTick>::iterator start, std::vector<MqlTick>::iterator end) {
    int nnew = 0;
    for (auto element = start; element != end; ++element)
        nnew += AddTick(*element);
    m_nnew = nnew; // overwrite internal added increment
    return m_nnew;
}

int MoneyBarBuffer::AddTicks(CCBufferMqlTicks ticks)
{
    int nnew = 0;
    int start1, start2, end1, end2;
    ticks.indexesNewTicks(start1, end1, start2, end2);
    for (int i = start1; i < end1; i++)
        nnew += AddTick(ticks.m_data[i]);
    for (int i = start2; i < end2; i++)
        nnew += AddTick(ticks.m_data[i]);
    m_nnew = nnew; // overwrite internal added increment
    return m_nnew;
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