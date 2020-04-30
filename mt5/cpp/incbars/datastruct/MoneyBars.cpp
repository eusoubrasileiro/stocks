// dllmain.cpp : Defines the entry point for the DLL application.
#define WIN32_LEAN_AND_MEAN
#include "windows.h"
#include "databuffers.h"
#include "cwindicators.h"
#include <algorithm>

#ifdef DEBUG
#include <fstream>
std::ofstream debugfile("databufferlog.txt");
#else
#define debugfile std::cout
#endif

//#include <mutex>

//std::mutex m;

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

// Indicator SADF
bool m_sadfindicator = false;
std::shared_ptr<CSADFIndicator> m_SADFi;
int m_maxbars; // maximum values to copy to indicator buffer pointer passed by MT5
// just for reload case store these
int m_minw, m_maxw, m_order;
bool m_usedrift;
int m_numcolors; // number of colors to plot max
int m_adfscount; // to define color of indicator dots
// Indicator Cum Sum
double m_reset = 0.5;
std::shared_ptr<CCumSumIndicator> m_CumSumi;


void CppDataBuffersInit(double ticksize, double tickvalue,
    double moneybar_size,  // R$ to form 1 money bar
    char* cs_symbol,  // cs_symbol is a char[] null terminated string (0) value at end
    bool sadfindicator, // wether to load sadf indicator or not in bakground
    // SADF part
    int maxwindow, 
    int minwindow, 
    int order, 
    bool usedrift, 
    int maxbars, 
    int numcolors) { // use to get begin of day

#ifdef DEBUG
    try {
#endif
        m_moneybar_size = moneybar_size;

        // m.lock(); // prevent resource anavaible multiple indicator threads on MT5

        m_ticks.reset(new BufferMqlTicks());
        m_bars.reset(new MoneyBarBuffer());
        m_ticks->Init(std::string(cs_symbol));
        m_bars->Init(tickvalue,
            ticksize, m_moneybar_size);
        m_newbars = false;
        m_maxbars = maxbars;
        m_sadfindicator = sadfindicator;
        // SADF part
        if (sadfindicator) { 
            m_SADFi.reset(new CSADFIndicator(m_bars->capacity()));
            m_minw = minwindow;
            m_maxw = maxwindow;
            m_order = order;
            m_numcolors = numcolors;
            m_usedrift = usedrift;
            m_SADFi->Init(maxwindow, minwindow, order, usedrift);
            m_adfscount = m_maxw - m_minw;
            //m_CumSumi.reset(new CCumSumIndicator(m_bars->capacity()));
        }
#ifdef  DEBUG
        }
        catch (const std::exception& ex) {
            debugfile << "c++ exception: " << std::endl;
            debugfile << ex.what() << std::endl;
        }
        catch (...) {
            debugfile << "Weird no idea exception" << std::endl;
            //cmpbegin_time = -1;<
        }
#endif //  DEBUG
    // m.unlock();
}

// for use on SADF Money Bars plot only indicator
void CppGetSADFWindows(int *minwin, int *maxwin){
    if (m_sadfindicator) {
        *minwin = m_minw;
        *maxwin = m_maxw;
    }
}

#include <cmath>

void RefreshIndicators() {
    debugfile << "number of bars " << m_bars->size() << std::endl;
#ifdef DEBUG
    try {
#endif
        if (m_sadfindicator) {
            auto bar_values = new float[m_bars->m_nnew];
            auto bar_count = m_bars->size();
            int errvalue = 0;

            for (int i = bar_count - m_bars->m_nnew; i < bar_count; i++) {
                auto value = m_bars->at(i).wprice;
                auto err = fpclassify(value);
                if (err == FP_INFINITE || err == FP_NAN) {
                    errvalue++;
                    value = 0;
                }
                bar_values[i] = value;
            }
#ifdef  DEBUG
            if (errvalue > 0)
                debugfile << "values with error on money bars" << errvalue << std::endl;
            m_SADFi->Refresh(bar_values, m_bars->m_nnew);
        }
    }
    catch (const std::exception& ex) {
        debugfile << "c++ exception: " << std::endl;
        debugfile << ex.what() << std::endl;
    }
    catch (...) {
        debugfile << "Weird no idea exception" << std::endl;
        //cmpbegin_time = -1;<
    }
#endif //  DEBUG
        // delete[] bar_values;
        //// I dont care for if first value is EMPTY, cumsum will be also EMPTY?
        //// to think about...
        //double* sadfv = new double[m_bars->m_nnew];
        //for (int i = m_SADFi->size()-m_bars->m_nnew, j=0; i < m_SADFi->size(); i++, j++)
        //    sadfv[j] = m_SADFi->at(i).sadf;

        //m_CumSumi->Refresh(sadfv, m_bars->m_nnew);

        //delete[] sadfv;
    
}


int64_t next_bgticktime = 0; // if an exception occour will

// returns the next cmpbegin_time
int64_t CppOnTicks(MqlTick* mt5_pticks, int mt5_nticks){


    m_newbars = false;
    //m.lock(); // prevent resource anavaible multiple indicator threads on MT5

#ifdef  DEBUG
    try {
#endif //  DEBUG    
    
    int ticks_left = mt5_nticks;
    // send ticks on 'batches' of MAX_TICKS
    // changed this to avoid needing a HUGE buffer for ticks
    while(ticks_left > 0) {

        next_bgticktime = m_ticks->Refresh(mt5_pticks, min(ticks_left, MAX_TICKS));
        ticks_left -= MAX_TICKS;

        if (m_ticks->nNew() > 0) {
            if (m_bars->AddTicks(m_ticks->end() - m_ticks->nNew(),
                m_ticks->end()) > 0) {
                m_newbars = true;
                RefreshIndicators();
            }
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

    //m.unlock();

    return next_bgticktime;
}


// MoneyBar indicator in Metatrader 5, candle style Open High Low Close
// High and Low show max, min value negotiated
// Open, Close show percentile 10, 90 of weighted volume prices
// mt5_pM shows the average weigthed price of entire bar, will be plot with DRAW_ARROW
int CppMoneyBarMt5Indicator(double* mt5_ptO, double* mt5_ptH, double* mt5_ptL, double* mt5_ptC, double* mt5_ptM, 
    unixtime *mt5_petimes, int mt5ptsize) {
    // receives the buffer indicator pointers from metatrader for fill in
    // taking in to account maxbars that will be filled
    // and size that is the maximum size, fill in the latest (maxbar values only)
    int maxbarsplot;
    int mbarsize;
    int i, j;

#ifdef  DEBUG
    try {
#endif //  DEBUG
        mbarsize = m_bars->size();
        maxbarsplot = min(mbarsize, m_maxbars); // use only bars available
        

    if (maxbarsplot > 0) {
        for (i = mt5ptsize - maxbarsplot, j = mbarsize - maxbarsplot; i < mt5ptsize; i++, j++) {
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
    debugfile << mt5ptsize << maxbarsplot << mbarsize << i << j << std::endl;
    debugfile << "c++ exception: " << std::endl;
    debugfile << ex.what() << std::endl;
}
catch (...) {
    debugfile << "Weird no idea exception" << std::endl;
}
#endif //  DEBUG
    return maxbarsplot;
}

// SADF on MoneyBars
// metatrader 5 buffer arrays for plotting
// line DRAW_LINE
// DRAW_COLOR_ARROW  dots and color
// imaxadflast, is the last value calculated (for labelling)
int CppSADFMoneyBars(double* mt5_SADFline, double* mt5_SADFdots, double* mt5_imaxadfcolor, double* mt5_imaxadflast, int mt5ptsize) {
    // receives the buffer indicator pointers from metatrader for fill in
    // taking in to account maxbars that will be filled
    // and size that is the maximum size, fill in the latest (maxbar values only)

    int64_t maxbarsplot = 0;
    int i = 0, j = 0;
    size_t mbarsize = 0;

    if (!m_sadfindicator) // just if pre-loaded
        return 0;

#ifdef  DEBUG
    try {
#endif //  DEBUG

        mbarsize = m_bars->size();

        // use only data available for plotting
        maxbarsplot = min(m_SADFi->size(), (size_t)m_maxbars);

        if (maxbarsplot > 0) {
            for (i = mt5ptsize - maxbarsplot, j = m_SADFi->size() - maxbarsplot; i < mt5ptsize; i++, j++) {
                mt5_SADFline[i] = m_SADFi->at(j).sadf; // average weighted price of entire bar
                mt5_SADFdots[i] = m_SADFi->at(j).sadf;
                mt5_imaxadfcolor[i] = (int)((m_numcolors - 1) * (m_SADFi->at(j).imaxadf / m_adfscount));
            }
            *mt5_imaxadflast = m_SADFi->at(m_SADFi->size() - 1).imaxadf;
        }


#ifdef  DEBUG
    }
    catch (const std::exception& ex) {
        debugfile << "CppSADFMoneyBars " << mt5ptsize << " " << maxbarsplot << " " << mbarsize << i << j << std::endl;
        debugfile << "c++ exception: " << std::endl;
        debugfile << ex.what() << std::endl;
    }
    catch (...) {
        debugfile << "Weird no idea exception" << std::endl;
    }
#endif //  DEBUG

    return maxbarsplot;
}


DLL_EXPORT size_t SearchMoneyBars(buffer<MoneyBar> mbars, uint64_t uid) {
    auto iter = std::lower_bound(mbars.begin(), mbars.end(), uid, cmpMoneyBarSmallUid);
    return (iter == mbars.end()) ? -1 : iter - mbars.begin();
}


bool CppNewBars() // are there any new bars after call of CppOnTicks
{
    return m_newbars;
}



// for 
// debugging only
BufferMqlTicks* GetBufferMqlTicks() {
    return &*m_ticks;
}

void CppTicksToFile(char* cs_filename) {
    SaveTicks(&*m_ticks, std::string(cs_filename));
}

bool CppisInsideFile(char* cs_filename) {
    return isInFile(&*m_ticks, std::string(cs_filename));
}
