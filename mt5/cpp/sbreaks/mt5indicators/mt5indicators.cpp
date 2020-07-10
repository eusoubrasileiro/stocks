#include "mt5indicators.h"


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

// from databuffers.h/cpp
extern std::shared_ptr<MoneyBarBuffer> m_bars; // buffer of money bars base for everything

// Indicator SADF
std::shared_ptr<CSADFIndicator> m_SADFi;
int m_maxbars; // maximum values to copy to indicator buffer pointer passed by MT5
// just for reload case store these
int m_minw, m_maxw, m_order;
bool m_usedrift;
int m_numcolors; // number of colors to plot max
int m_adfscount; // to define color of indicator dots
// Indicator Cum Sum
double m_reset = 0.5;
std::shared_ptr<CCumSumSADFIndicator> m_CumSumi;

void RefreshIndicators();

// can only be called after CppDataBuffersInit
void CppMoneyIndicatorsInit(int maxwindow,
                            int minwindow,
                            int order,
                            bool usedrift,
                            int maxbars,
                            int numcolors){
    if (m_bars == nullptr)
        return;
    m_maxbars = maxbars;
    m_SADFi.reset(new CSADFIndicator(m_bars->capacity()));
    m_minw = minwindow;
    m_maxw = maxwindow;
    m_order = order;
    m_numcolors = numcolors;
    m_usedrift = usedrift;
    m_SADFi->Init(maxwindow, minwindow, order, usedrift);
    m_adfscount = m_maxw - m_minw;
    m_CumSumi.reset(new CCumSumSADFIndicator(m_bars->capacity()));
    // automatically refreshed when m_SADFi is refreshed
    m_CumSumi->Init(m_reset, &(*m_SADFi));
    // subscribe to OnNewBars event
    m_bars->AddOnNewBars(RefreshIndicators);
}

// for use on SADF Money Bars plot only indicator
void CppGetSADFWindows(int *minwin, int *maxwin){
    if (m_SADFi != nullptr){
        *minwin = m_minw;
        *maxwin = m_maxw;
    }
}

void RefreshIndicators(){
    try
    {
        // copying average weighted price from money bars
        auto bar_values = new float[m_bars->m_nnew];
        int i = 0;
        for (auto bar = m_bars->BeginNewBars(); bar != m_bars->end(); bar++) {
            bar_values[i] = (float) bar->wprice; i++;
        }
        // should accept an stl iterator instead on ctyle array
        m_SADFi->Refresh(bar_values, m_bars->m_nnew);
        if(m_SADFi->m_ncalculated > 0)
            debugfile << "number of bars " << m_bars->size() 
            << " calculated SADF: " << m_SADFi->m_ncalculated 
            << " Total SADF: " << m_SADFi->Count() 
            << std::endl;
    }
    catch (const std::exception& ex) {
        debugfile << "c++ exception: " << std::endl;
        debugfile << ex.what() << std::endl;
    }
    catch (...) {
        debugfile << "Weird no idea exception" << std::endl;
    }    
}




// MoneyBar indicator in Metatrader 5, candle style Open High Low Close
// High and Low show max, min value negotiated
// Open, Close show percentile 10, 90 of weighted volume prices
// mt5_pM shows the average weigthed price of entire bar, will be plot with DRAW_ARROW
int CppMoneyBarMt5Indicator(double* mt5_ptO, double* mt5_ptH, double* mt5_ptL, double* mt5_ptC, double* mt5_ptM, 
    unixtime *mt5_petimes, double* mt5_bearbull, int mt5ptsize) {
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
            mt5_bearbull[i] = (int)((m_numcolors - 1) * (1+m_bars->at(j).netvol)/2.); // from -1/+1 to 0/1 where 0.5 is no bear no bull
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

    if (m_SADFi == nullptr) // just if pre-loaded
        return 0;

#ifdef  DEBUG
    try {
#endif //  DEBUG
        mbarsize = m_bars->size();

        // use only data available for plotting
        maxbarsplot = min(m_SADFi->Count(), (size_t)m_maxbars);

        if (maxbarsplot > 0) {
            for (i = mt5ptsize - maxbarsplot, j = m_SADFi->Count() - maxbarsplot; i < mt5ptsize; i++, j++) {
                mt5_SADFline[i] = m_SADFi->SADFt(j); // average weighted price of entire bar
                mt5_SADFdots[i] = m_SADFi->SADFt(j);
                mt5_imaxadfcolor[i] = (int)((m_numcolors - 1) * (m_SADFi->MaxADFi(j) / m_adfscount));
            }
            *mt5_imaxadflast = m_SADFi->MaxADFi(m_SADFi->Count() - 1);
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

