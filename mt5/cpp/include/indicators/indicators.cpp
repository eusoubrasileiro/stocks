#include "indicators.h"


// Indicator SADF
// just for reload case store these
std::shared_ptr<CSADF> m_SADFi;
int m_minw, m_maxw, m_order;
bool m_usedrift;

// Indicator Cum Sum
std::shared_ptr<CCumSumSADF> m_CumSumi;
std::shared_ptr<CFracDiff> m_FdMb;
std::shared_ptr<CMbReturn> m_MbReturn;
std::shared_ptr<CStdevMbReturn> m_StdevMbReturn;

// Events classification
std::shared_ptr<std::vector<Event>> m_Events; // events YET to be labbeled and filled with features
// feature vector needed for training, only exists for those comming out of LabelEvents
// same size as m_Events_feat
std::shared_ptr<std::vector<std::vector<double>>> m_X_feat;
std::shared_ptr<std::vector<Event>> m_Events_feat; // events labelled and with features


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
    m_FdMb.reset(new CFracDiff());
    m_StdevMbReturn.reset(new CStdevMbReturn());
    m_CumSumi.reset(new CCumSumSADF(m_bars->capacity()));
    m_Events.reset(new std::vector<Event>());

    // not exacly indicators
    m_Events_feat.reset(new std::vector<Event>());
    m_X_feat.reset(new std::vector<std::vector<double>>());

    m_minw = minwindow;
    m_maxw = maxwindow;
    m_order = order;
    m_usedrift = usedrift;
    m_SADFi->Init(maxwindow, minwindow, order, usedrift);    
    // automatically refreshed when m_SADFi is refreshed
    m_CumSumi->Init(cum_reset, &(*m_SADFi));
    m_FdMb->Init(128, 0.56);

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

    // Events triggered when CUMSUM SADF != 0
    auto fillEvents = [pEvents=&*m_Events, pCSum = &*m_CumSumi, pbars=&*m_bars](int  n_){
         Event event;
        // add events from new data, only valid data
         for (auto idx = pCSum->Count()-pCSum->nCalculated(); idx < pCSum->Count(); idx++)
             if (pCSum->At(idx) != 0.0) {
                 event.cumsum = pCSum->At(idx);
                 event.twhen = pbars->at(idx).uid;
                 pEvents->push_back(event);
             }        
    };

    m_CumSumi->addOnRefresh(fillEvents);    
}

void RefreshIndicators() {
    try
    {
        // copying average weighted price from money bars
        std::vector<float> bar_wprices;
        for (auto elem = m_bars->LastBegin(); elem != m_bars->end(); elem++)
            bar_wprices.push_back(std::log(elem->wprice)); // per-book log(prices)
        // altough nice and cool, If I need multiple members so... I loop is imperative
        m_SADFi->Refresh<vec_iterator<float>>(bar_wprices.begin(), bar_wprices.end());
        m_MbReturn->Refresh<buffer<MoneyBar>::iterator>(m_bars->LastBegin(), m_bars->end());
        m_FdMb->Refresh<vec_iterator<float>>(bar_wprices.begin(), bar_wprices.end());
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
