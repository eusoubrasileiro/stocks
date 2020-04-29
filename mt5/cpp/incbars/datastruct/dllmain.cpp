// dllmain.cpp : Defines the entry point for the DLL application.
#define WIN32_LEAN_AND_MEAN
#include "windows.h"
#include "databuffers.h"

#ifdef DEBUG
#include <fstream>
std::ofstream debugfile("databufferlog.txt");
#else
#define debugfile std::cout
#endif

#include <mutex>

std::mutex m;

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

//#define MT5_MAX_INDICATORS 10 // maximum number of indicators using money bars

// Base Data
std::shared_ptr<BufferMqlTicks> m_ticks; // buffer of ticks w. control to download unique ones.

std::shared_ptr<MoneyBarBuffer> m_bars; // buffer of money bars base for everything
bool m_newbars; // any new bars from previous call to CppOnTicks?
double m_moneybar_size; // R$ to form 1 money bar

void CppDataBuffersInit(double ticksize, double tickvalue,
    double moneybar_size,  // R$ to form 1 money bar
    char* cs_symbol,  // cs_symbol is a char[] null terminated string (0) value at end
    int64_t mt5_timenow) { // use to get begin of day
    m_moneybar_size = moneybar_size;

    m.lock(); // prevent resource anavaible multiple indicator threads on MT5

    m_ticks.reset(new BufferMqlTicks());
    m_bars.reset(new MoneyBarBuffer());

    m_ticks->Init(std::string(cs_symbol), mt5_timenow);

    m_bars->Init(tickvalue,
        ticksize, m_moneybar_size);

    m_newbars = false;

    m.unlock();
}

// returns the next cmpbegin_time
int64_t CppOnTicks(MqlTick* mt5_pticks, int mt5_nticks){

    int64_t cmpbegin_time = 0;
    m_newbars = false;

    int ticks_left = mt5_nticks - MAX_TICKS;

    m.lock(); // prevent resource anavaible multiple indicator threads on MT5

#ifdef  DEBUG
    try {
#endif //  DEBUG

    cmpbegin_time = m_ticks->Refresh(mt5_pticks, min(mt5_nticks, MAX_TICKS), true);

    if (m_ticks->nNew() > 0) {
        if (m_bars->AddTicks(m_ticks->end() - m_ticks->nNew(),
            m_ticks->end()) > 0)
            m_newbars = true;
    }

    

    // send ticks on 'batches' of MAX_TICKS
    // changed this to avoid needing a HUGE buffer for ticks
    while(ticks_left > 0) {

        cmpbegin_time = m_ticks->Refresh(mt5_pticks, min(ticks_left, MAX_TICKS), false);
        ticks_left -= MAX_TICKS;

        if (m_ticks->nNew() > 0) {
            if (m_bars->AddTicks(m_ticks->end() - m_ticks->nNew(),
                m_ticks->end()) > 0)
                m_newbars = true;
        }
    }

#ifdef  DEBUG
    }
    catch (const std::exception& ex) {
        debugfile << "c++ exception: " << std::endl;
        debugfile << ex.what() << std::endl;
        // I suspect that on periods of high volatility
        // or internet connection problems
        // many more ticks are dropped (also commented on MT5 forums)
        // so for those days would have to write some work around
        // to make money bars restart from MT5 side
        //cmpbegin_time = -1;
        // when iceberg orders show up
        // just saw iceberg orders on history

    }
    catch (...) {
        debugfile << "Weird no idea exception" << std::endl;
        debugfile << "mt5_nticks " << mt5_nticks << std::endl;
        debugfile << "ticks_left " << mt5_nticks << std::endl;
        //cmpbegin_time = -1;<
    }
#endif //  DEBUG

    m.unlock();

    return cmpbegin_time;
}

void CppTicksToFile(char* cs_filename) {
    SaveTicks(&(*m_ticks), std::string(cs_filename));
}

bool CppisInsideFile(char* cs_filename) {
    return isInFile(&(*m_ticks), std::string(cs_filename));
}

bool CppNewBars() // are there any new bars after call of CppOnTicks
{
    return m_newbars;
}

// MoneyBar indicator in Metatrader 5, candle style Open High Low Close
// High and Low show max, min value negotiated
// Open, Close show percentile 10, 90 of weighted volume prices
// mt5_pM shows the average weigthed price of entire bar, will be plot with DRAW_ARROW
int CppMoneyBarMt5Indicator(double* mt5_ptO, double* mt5_ptH, double* mt5_ptL, double* mt5_ptC, double* mt5_ptM, 
    unixtime *mt5_petimes, int mt5ptsize, int maxbars) {
    // receives the buffer indicator pointers from metatrader for fill in
    // taking in to account maxbars that will be filled
    // and size that is the maximum size, fill in the latest (maxbar values only)
    auto mbarsize = m_bars->size();    
    maxbars = min(mbarsize, maxbars); // use only bars available
    int i, j;
#ifdef  DEBUG
    try {
#endif //  DEBUG

    if (maxbars > 0) {
        for (i = mt5ptsize - maxbars, j = mbarsize - maxbars; i < mt5ptsize; i++, j++) {
            mt5_ptO[i] = m_bars->at(j).wprice10; // average weighted price percentile 10
            mt5_ptC[i] = m_bars->at(j).wprice90; // average weighted price percentile 90
            mt5_ptH[i] = m_bars->at(j).max; // max value negotiated
            mt5_ptL[i] = m_bars->at(j).min; // min value negotiated
            mt5_ptM[i] = m_bars->at(j).wprice; // average weighted price of entire bar
            mt5_petimes[i] = (unixtime)(m_bars->at(j).emsc * 0.001);
        }
    }


#ifdef  DEBUG
}
catch (const std::exception& ex) {
    debugfile << mt5ptsize << maxbars << mbarsize << i << j << std::endl;
    debugfile << "c++ exception: " << std::endl;
    debugfile << ex.what() << std::endl;
}
catch (...) {
    debugfile << "Weird no idea exception" << std::endl;
}
#endif //  DEBUG
    return maxbars;
}


DLL_EXPORT size_t SearchMoneyBars(buffer<MoneyBar> mbars, uint64_t uid) {
    auto iter = std::lower_bound(mbars.begin(), mbars.end(), uid, cmpMoneyBarSmallUid);
    return (iter == mbars.end()) ? -1 : iter - mbars.begin();
}