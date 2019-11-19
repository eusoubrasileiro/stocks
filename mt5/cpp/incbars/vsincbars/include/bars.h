#pragma once
#include "..\buffers.h"
#include "ticks.h"

// ask and bid prices are not present on summary symbols like WIN@ WDO@ WIN$
// but are present on WINV19 etc. stocks PETR4 etc...

// Money Bar solves many problems of analysis of stocks data
// stationarity and also empty volumes
struct MoneyBar
{
    //   datetime     time;          // Time of the last prices update 1.
    double       bid;           // Current Bid price - might be 0 WHEN Equal the LAST
    double       ask;           // Current Ask price - might be 0 WHEN Equal the LAST
    double       last;          // Price of the last deal (Last) - never 0 when volume not 0 - zero when no match between ask-bid
 //   ulong        volume;        // Volume for the current Last price 2.
    long         time_msc;       // Time of a price last update in milliseconds == 1. but with milliseconds precision
 //   int          day;             // day of year
 //   uint         flags;         // Tick flags - no reason to use this on money-bar - shows what changed from the previous tick
 //   double       volume_real;   // Volume for the current Last price with greater accuracy same as 2. but better
};

class MoneyBarBuffer : public CCBuffer<MoneyBar>
{
protected:
    double m_count_money;
    double m_point_value;
    double m_moneybarsize;
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
    MoneyBarBuffer(double tickvalue, double ticksize, double moneybarsize);
    // add one tick and create as many money bars as needed (or 0)
    // return number created
    int AddTick(MqlTick tick);
    // add ticks from bg_idx until size
    int AddTicks(std::vector<MqlTick>::iterator start, std::vector<MqlTick>::iterator end);
    int AddTicks(CCBufferMqlTicks ticks);
};