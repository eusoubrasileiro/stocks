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

// can only be called after CppDataBuffersInit
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
    m_CumSumi.reset(new CCumSumSADFIndicator(m_bars->capacity()));
    // automatically refreshed when m_SADFi is refreshed
    m_CumSumi->Init(m_reset, &(*m_SADFi));
    // subscribe to OnNewBars event
    m_bars->AddOnNewBars(RefreshIndicators);
}

void RefreshIndicators() {
    try
    {
        // copying average weighted price from money bars
        auto bar_values = new float[m_bars->m_nnew];
        int i = 0;
        for (auto bar = m_bars->BeginNewBars(); bar != m_bars->end(); bar++) {
            bar_values[i] = (float)bar->wprice; i++;
        }
        // should accept an stl iterator instead on ctyle array
        m_SADFi->Refresh(bar_values, m_bars->m_nnew);
        if (m_SADFi->m_ncalculated > 0)
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
