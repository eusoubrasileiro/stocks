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

CCumSumIndicator::CCumSumIndicator(int buffersize) : CWindowIndicatorDouble(buffersize){
    m_cum_up = m_cum_down = 0; m_cum_reset = 1;
};

void CCumSumIndicator::Init(double cum_reset) {
    m_cum_reset = cum_reset;
    CWindowIndicator::Init(2);
}

void CCumSumIndicator::Calculate(double* indata, int size, std::array<std::vector<double>, 1> & outdata) {

    for (int i = 1; i < size; i++) {
        auto diff = indata[i] - indata[i - 1];
        m_cum_up = std::max((double) 0, m_cum_up + diff);
        m_cum_up = std::min((double) 0, m_cum_up + diff);
        if (m_cum_up > m_cum_reset) {
            m_cum_up = 0;
            outdata[0][i-1] = +1;
        }
        else
        if (m_cum_down < -m_cum_reset) {
            m_cum_down = 0;
            outdata[0][i-1] = -1;
        }
        else
            outdata[0][i-1] = 0;
    }
}

// CCumSum on SADF(t) values

CCumSumSADFIndicator::CCumSumSADFIndicator(int buffersize) : CCumSumIndicator(buffersize)
{};

void CCumSumSADFIndicator::Init(double cum_reset, CSADFIndicator *pSADF) {
    m_cum_reset = cum_reset;
    // initialize cum sum filter
    CWindowIndicator::Init(pSADF->Window()+1); //1 + SADF window due 1st diff

    // this the custom refresh function (lambda)
    // using the SADF(t) just calculated data to call the CumSum this->Refresh
    // also taking care of empties
    auto thisrefresh = [this, pSADF](int nempty) {
        this->AddEmpty(nempty);        
        std::vector<double> new_data;
        new_data.assign(pSADF->vBegin(0), pSADF->End(0));
        this->Refresh(new_data.data(), new_data.size());
    };

    pSADF->addSonRefresh(std::function<void(int)>(thisrefresh));
}