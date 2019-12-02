#pragma once
#include "buffers.h"
#include "ticks.h"
#include "time.h"

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
    // unique identifier for this bar - for searching etc..
    unsigned long long uid; // emsc and smsc might repeat on different bars
    int wday; // day of week for this bar
};
#pragma pack(pop)

class MoneyBarBuffer : public buffer<MoneyBar>
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
    // current unique identifier - starts w. 0 and goes forever increasing
    // max size is 18,446,744,073,709,551,615 == 2^64-1
    // will never have in any scenary this ammount of money bars so rest safe
    // there is be no problems on binary search for money bars
    unsigned long long cuid; 
    // current week day for the bar being formed
    // if a day is crossed the data for this bar is ignored
    int cwday;    

public:
    int m_nnew; // number of bars nnew on last call    
    double new_avgprices[BUFFERSIZE];

    MoneyBarBuffer();

    void Init(double tickvalue, double ticksize, double moneybarsize);
    // add one tick and create as many money bars as needed (or 0)
    // return number created
    int AddTick(MqlTick tick);
    // add ticks for python support
    int AddTicks(std::vector<MqlTick>::iterator start, std::vector<MqlTick>::iterator end);
    // add ticks for metatrader support
    int AddTicks(const MqlTick* cticks, int size);

    MoneyBar* BeginNewBars();
    int BeginNewBarsIdx();

    void RefreshArrays();

    // return buffer index position
    int Search(unsigned long long uid);
    int SearchStime(unsigned long long emsc);
};
