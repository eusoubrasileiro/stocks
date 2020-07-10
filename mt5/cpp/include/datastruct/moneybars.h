#pragma once
#include <time.h>
#include <array>
#include <functional>
#include "buffers.h"
#include "ticks.h"


// ask and bid prices are not present on summary symbols like WIN@ WDO@ WIN$
// but are present on WINV19 etc. stocks PETR4 etc...

typedef  int64_t unixtime;
typedef  int64_t unixtime_ms;


// Money Bar solves many problems of analysis of stocks data
// - stationarity first
// - tick with no volume
// times are (GMT)
#pragma pack(push, 2)
struct MoneyBar
{
    // preço médio
    double       wprice;  // volume weighted price from all ticks on this bar
    double       wprice90; // volume weighted price from all ticks on this bar -  percentile 90
    double       wprice10; // volume weighted price from all ticks on this bar -  percentile 10
    int          nticks;  // number ticks to form this bar
    tm time; // start time of this bar ... datetime tm struct
    unixtime_ms    smsc; // start time of this bar - first tick time - timestamp ms
    unixtime_ms    emsc; // end time of this bar - last tick time - timestamp ms
    double min; // min and maximum value negotiated on this bar p0, p100
    double max;
    // p10, p50, p90 of ticks.last?
    // unique identifier for this bar - for searching etc..
    uint64_t uid; // emsc and smsc might repeat on different bars
    double dtp; // time difference to previous bar in seconds (only inside a day)
    double netvol; // number of buy ticks * volume bought + number of sell ticks * volume sold (sell/buy) power (net-volume)
    // last high  - to calculate volatility in this bar
    // last low
};
#pragma pack(pop)

class MoneyBarBuffer : public buffer<MoneyBar>
{

    double m_count_money;
    double m_point_value;
    double m_moneybarsize;
    // making average weight volume price
    double m_pvs; // \sum_{0}^{i} prices_i*volumes_i
    double m_vs; // \sum_{0}^{i} volumes_i
    std::vector<double> m_wprices; // store all weighted prices of current bar MAX 10e3 Ticks
    MoneyBar m_bar; // temp variable
    // current unique identifier - starts w. 0 and goes forever increasing
    // max size is 18,446,744,073,709,551,615 == 2^64-1
    // will never have in any scenary this ammount of money bars so rest safe
    // there is be no problems on binary search for money bars
    uint64_t cuid;
    // start tm time the bar being formed
    // if a day is crossed the data for this bar is ignored
    tm ctime;
    std::vector<std::function<void(void)>> m_event_newbars;

    //BufferMqlTicks m_ticks; // ticks used to build bars from mt5

    // call all event callback functions subscribed
    void OnNewBars() {
        for (auto& callbackfunc : m_event_newbars) // access by reference to avoid copying        
            callbackfunc();
    }

public:
    size_t m_hash; // unique identifier of this money bar instance
    size_t m_nnew; // number of bars nnew on last call

    buffer<uint64_t> uidtimes;

    MoneyBarBuffer();

    MoneyBarBuffer(const MoneyBarBuffer &moneybars); // copy constructor 

    void Init(double tickvalue, double ticksize, double moneybarsize);
    // add one tick and create as many money bars as needed (or 0)
    // return number created
    size_t AddTick(MqlTick tick);
    // add ticks for metatrader support
    size_t AddTicks(boost::circular_buffer<MqlTick>::iterator start,
        boost::circular_buffer<MqlTick>::iterator end);
    // add ticks for python support
    size_t AddTicks(const MqlTick* cticks, int size);

    // iterator begin of new bars
    auto BeginNewBars() {
        return end() - m_nnew;
    }

    // on new bars call back function
    void AddOnNewBars(std::function<void(void)> onnewbars) {
        m_event_newbars.push_back(onnewbars);
    }

    size_t BeginNewBarsIdx();

    // return buffer index position
    size_t Search(uint64_t uid);
    
    size_t SearchStime(int64_t emsc);
};

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


// standar 1 minute bar since it's easier
// to do everythin with this first

//#pragma pack(push, 2)
//struct TimeBar //MqlRates from MT5
//{
//    unixtime time;         // Period start time
//    double   open;         // Open price
//    double   high;         // The highest price of the period
//    double   low;          // The lowest price of the period
//    double   close;        // Close price
//    int64_t  tickv;        // Tick volume
//    int      spread;       // Spread
//    int64_t  realv;        // Trade volume
//};
//#pragma pack(pop)
//
//class TimeBarBuffer : public buffer<TimeBar>
//{
//
//};