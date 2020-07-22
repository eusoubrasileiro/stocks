#include "cwindicators.h"
#include <algorithm>
#include <array>

void CFracDiffIndicator::Init(int window, double dfraction){
    m_dfraction = dfraction;
    CWindowIndicator::Init(window);
    setFracDifCoefs(m_dfraction, window);
};

void CFracDiffIndicator::Calculate(double* indata, int size, std::array<std::vector<double>, 1> &outdata)
{
    FracDifApply(indata, size, outdata[0].data());
}


//
// Windowed Augmented Dickey-Fuller test or SADF (supremum) ADF
// SADF very optimized for GPU using Libtorch C++ Pytorch
//

CSADFIndicator::CSADFIndicator(int buffersize) : CWindowIndicator(buffersize) {
    m_usedrift = false;
    m_gpumemgb = 2.0;
    m_verbose = false;
};

void CSADFIndicator::Init(int maxwindow, int minwindow, int order, bool usedrift, float gpumemgb, bool verbose) {
    m_usedrift = usedrift;
    m_gpumemgb = gpumemgb;
    m_verbose = verbose;
    m_minw = minwindow;
    m_maxw = maxwindow;
    m_order = order;
    CWindowIndicator::Init(m_maxw); // inside m_prev_data.resize(m_maxw+1-1) -> 1 output when == m_maxw
};

//sadf(float* signal, float* outsadf, float* outadfmaxidx, int n,
// int maxw, int minw, int order, bool drift, float gpumem_gb, bool verbose)

void CSADFIndicator::Calculate(float* indata, int size, std::array<std::vector<float>, 2> &outdata)
{
    //m_sadf.resize(size - m_maxw); // has buffesize suppose to be allways enough
    //m_imaxadf.resize(size - m_maxw);

    auto sadf_out = &outdata[0];
    auto imaxadf_out = &outdata[1];

    int ncalculated = sadf(indata, sadf_out->data(), imaxadf_out->data(), size,
        m_maxw, m_minw, m_order, m_usedrift, m_gpumemgb, m_verbose);
}


// Cum sum simetric filter
// +1 for up entries, -1 for down entries

CCumSumIndicator::CCumSumIndicator(int buffersize) : CWindowIndicator(buffersize){
    m_cum_up = m_cum_down = 0; m_cum_reset = 1;
};

void CCumSumIndicator::Init(double cum_reset) {
    m_cum_reset = cum_reset;
    CWindowIndicator::Init(2);
}

void CCumSumIndicator::Calculate(std::pair<float, int>* indata, int size, std::array<std::vector<int>, 1> & outdata) {

    for (int i = 1; i < size; i++) {
        // guarantee that cum sum is calculated only on of hours that are valid operationally
        // only calculate cum sum if second param (int) is True/Valid Sample (1 or 0)
        if (indata[i].second && indata[i - 1].second) 
        {
            auto diff = indata[i].first - indata[i - 1].first;
            m_cum_up = std::max((double)0, m_cum_up + diff);
            m_cum_down = std::min((double)0, m_cum_down + diff);
            if (m_cum_up > m_cum_reset) {
                m_cum_up = 0;
                outdata[0][i - 1] = +1;
            }
            else
                if (m_cum_down < -m_cum_reset) {
                    m_cum_down = 0;
                    outdata[0][i - 1] = -1;
                }
                else
                    outdata[0][i - 1] = 0;
        }
        else { // reset on this area, cannot be calculated
            // a new day starting
            m_cum_up = m_cum_down = 0; 
            outdata[0][i - 1] = 0;
        }
    }
}

// valid intraday operational window
// 0 for not valid 1 for valid
// applied on timebars
//CIntraday::CIntraday(int buffersize) : CWindowIndicator(buffersize)
//{ 
//    m_start_hour = 10.0; m_end_hour = 16.5; 
//}
//
//void CIntraday::Init(float start_hour, float end_hour){
//    CWindowIndicator::Init(1); 
//    m_start_hour = start_hour; m_end_hour = end_hour;
//}
//
//void CIntraday::Calculate(MoneyBar* bar, int size, std::array<std::vector<int>, 1> & outdata){
//    for (int i = 1; i < size; i++) {
//        if (bar[i].dayhour > m_start_hour && bar[i].dayhour < m_end_hour)
//            outdata[0][i] = 1; // valid
//        else
//            outdata[0][i] = 0;
//    }
//};

// CCumSum on SADF(t) values

CCumSumSADFIndicator::CCumSumSADFIndicator(int buffersize) : CCumSumIndicator(buffersize)
{};

void CCumSumSADFIndicator::Init(double cum_reset, CSADFIndicator *pSADF, MoneyBarBuffer *pBars) {
    m_cum_reset = cum_reset;
    // initialize cum sum filter
    CWindowIndicator::Init(2); //2 due 1st diff

    // this the custom refresh function (lambda)
    // using the SADF(t) just calculated data to call the CumSum this->Refresh
    // also taking care of empties
    // region where I dont want cumsum computed (outside valid intraday)   
    auto thisrefresh = [this, pSADF, pBars](int nempty) {
        this->AddEmpty(nempty); // allign buffers
        // make pairs from new data where second indicate wether
        // the value should be used or not on cum_sum
        std::vector<std::pair<float, int>> sadf_pair;
        auto bar = pBars->LastBegin();
        auto sadf = pSADF->vLastBegin(0);
        for (; bar != pBars->end() && sadf != pSADF->End(0); bar++, sadf++)                        
            sadf_pair.push_back(std::make_pair(*sadf, bar->inday));        
        this->Refresh(sadf_pair.begin(), sadf_pair.end());
    };

    pSADF->addSonRefresh(std::function<void(int)>(thisrefresh));
}
