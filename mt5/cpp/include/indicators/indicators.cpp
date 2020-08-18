#include "indicators.h"


// Indicator SADF
// just for reload case store these
std::shared_ptr<CSADF> m_SADFi;
int m_minw, m_maxw, m_order;
bool m_usedrift;

// Indicator Cum Sum
std::shared_ptr<CCumSumSADF> m_CumSumi;
std::shared_ptr<CMbReturn> m_MbReturn;
std::shared_ptr<CStdevMbReturn> m_StdevMbReturn;

void RefreshIndicators();

// can only be called after DataBuffersInit
void IndicatorsInit(int maxwindow,
                    int minwindow,
                    int order,
                    bool usedrift,
                    float cum_reset)
{
    if (m_bars == nullptr)
        return;
    m_SADFi.reset(new CSADF(m_bars->capacity()));
    m_MbReturn.reset(new CMbReturn());
    m_StdevMbReturn.reset(new CStdevMbReturn());
    m_CumSumi.reset(new CCumSumSADF(m_bars->capacity()));
    m_minw = minwindow;
    m_maxw = maxwindow;
    m_order = order;
    m_usedrift = usedrift;
    m_SADFi->Init(maxwindow, minwindow, order, usedrift);    
    // automatically refreshed when m_SADFi is refreshed
    m_CumSumi->Init(cum_reset, &(*m_SADFi));
    // subscribe to OnNewBars event
    // SADF and MB Returns subscribe 
    m_bars->AddOnNewBars(RefreshIndicators);
    // Returns and volatility of returns
    m_MbReturn->Init(); // father refreshes son bellow
    // remember that average of returns is allways around zero! so to have an idea of volatility 
    // that why we need to calculated a spread around this mean, that's stddev
    // 'average return expected around the mean' shows how much return variation expect 
    // on average considering xxx bars
    m_StdevMbReturn->Init(300, &(*m_MbReturn)); // 300 bars moving average stddev of returns
    
}

void RefreshIndicators() {
    try
    {
        // copying average weighted price from money bars
        auto bar_wprices = vecMoneyBarBufferLast<double, float>(&MoneyBar::wprice, &(*m_bars));
        // altough nice and cool, If I need multiple members so... I loop is imperative
        m_SADFi->Refresh<vec_iterator<float>>(bar_wprices.begin(), bar_wprices.end());
        m_MbReturn->Refresh<buffer<MoneyBar>::iterator>(m_bars->LastBegin(), m_bars->end());
        if (m_SADFi->nCalculated() > 0)
            debugfile << "number of bars " << m_bars->size()
            << " calculated SADF: " << m_SADFi->nCalculated()
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
