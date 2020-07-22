#include "indicators.h"


// Indicator SADF
// just for reload case store these
std::shared_ptr<CSADFIndicator> m_SADFi;
int m_minw, m_maxw, m_order;
bool m_usedrift;

// Indicator Cum Sum
double m_reset = 0.5;
std::shared_ptr<CCumSumSADFIndicator> m_CumSumi;


void RefreshIndicators();

// can only be called after DataBuffersInit
void IndicatorsInit(int maxwindow,
                    int minwindow,
                    int order,
                    bool usedrift)
{
    if (m_bars == nullptr)
        return;
    m_SADFi.reset(new CSADFIndicator(m_bars->capacity()));
    m_minw = minwindow;
    m_maxw = maxwindow;
    m_order = order;
    m_usedrift = usedrift;
    m_SADFi->Init(maxwindow, minwindow, order, usedrift);    
    // wether sell-buy operation is valid or not
    //m_Intraday.reset(new CIntraday(m_bars->capacity()));

    m_CumSumi.reset(new CCumSumSADFIndicator(m_bars->capacity()));
    // automatically refreshed when m_SADFi is refreshed
    m_CumSumi->Init(m_reset, &(*m_SADFi), &(*m_bars));
    // subscribe to OnNewBars event
    m_bars->AddOnNewBars(RefreshIndicators);
}

void RefreshIndicators() {
    try
    {
        // copying average weighted price from money bars
        auto bar_wprices = vecMoneyBarBufferLast<double, float>(&MoneyBar::wprice, &(*m_bars));
        // altough nice and cool, If I need multiple members so... I loop is imperative
        m_SADFi->Refresh<vec_iterator<float>>(bar_wprices.begin(), bar_wprices.end());
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
