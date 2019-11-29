#pragma once
#include "buffers.h"
#include "ticks.h"

// ask and bid prices are not present on summary symbols like WIN@ WDO@ WIN$
// but are present on WINV19 etc. stocks PETR4 etc...

// Money Bar solves many problems of analysis of stocks data
// - stationarity first
// - tick with no volume
#pragma pack(push, 2)
struct MoneyBar
{
    // preço médio
    double       avgprice;  // volume weighted price from all ticks on this bar
    int          nticks;  // number ticks to form this bar
    long long    smsc; // start time of this bar - first tick time 
    long long    emsc; // end time of this bar - last tick time
    // p10, p50, p90 of ticks.last?
};
#pragma pack(pop)

class MoneyBarBuffer : public CCBuffer<MoneyBar>
{
protected:
    double m_count_money;
    double m_point_value;
    double m_moneybarsize;
    // making average weight volume price
    double m_pvs; // \sum_{0}^{i} prices_i*volumes_i
    double m_vs; // \sum_{0}^{i} volumes_i
    // counting ticks for bar
    int m_nticks;
    MoneyBar m_bar; // temp variable

public:
    int m_nnew; // number of bars nnew on last call
    std::vector<double> m_last; // lastest added last prices
    std::vector<long> m_times; // ... . ...... times in msc

    int indexesNewBars(int parts[]);
    // copy last data of new bars m_nnew
    // to local arrays
    void RefreshArrays();
    // local arrays have max size equal buffer size
    void SetSize(int size);

    MoneyBarBuffer(double tickvalue, double ticksize, double moneybarsize,
        int buffersize);
    // add one tick and create as many money bars as needed (or 0)
    // return number created
    int AddTick(MqlTick tick);
    // add ticks for python support
    int AddTicks(std::vector<MqlTick>::iterator start, std::vector<MqlTick>::iterator end);
    // add ticks for metatrader support
    int AddTicks(const MqlTick* cticks, int size);
};
