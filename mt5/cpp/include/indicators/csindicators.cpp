#include "cwindicators.h"
#include <algorithm>
#include <array>




void CFracDiff::Init(int window, double dfraction){
    m_dfraction = dfraction;
    CWindowIndicator::Init(window);
    setFracDifCoefs(m_dfraction, window);
};

void CFracDiff::Calculate(double* indata, int size, std::array<std::vector<double>, 1> &outdata)
{
    FracDifApply(indata, size, outdata[0].data());
}


//
// Return on MoneyBars price[i+1]/price[i] - 1.
// weighted return considering h, l and open inside each bar
//

void CMbReturn::Init(){
    CWindowIndicator::Init(2);    
};

void CMbReturn::Calculate(MoneyBar* inbars, int size, std::array<std::vector<double>, 1>& outdata)
{
    double p2; // previous bar: last value  high or low
    double co, c1, c2;  // current bar, open, h or l, l or l // correct sequence of events
    auto out = outdata[0].data();

    for (int i = 1; i < size; i++) {        
        p2 = (inbars[i - 1].highfirst)? inbars[i - 1].low: inbars[i - 1].high; // second point of previous bar low or high

        co = inbars[i].open;
        if (inbars[i].highfirst) { c1 = inbars[i].high; c2 = inbars[i].low; }
        else { c1 = inbars[i].low; c2 = inbars[i].high; }

        // calculate returns on prices
        // co/p2-1, c1/co-1, c2/c1-1
        // make a weighted average 0.75, 0.85, 1.0
        // the closer current point stronger
        out[i - 1] = 0.75 * (co / p2 - 1.) + 0.85 * (c1 / co - 1.) + (c2 / c1 - 1.);
        out[i - 1] /= 0.75 + 0.85 + 1.0;        
    }
}


//
// Windowed Augmented Dickey-Fuller test or SADF (supremum) ADF
// SADF very optimized for GPU using Libtorch C++ Pytorch
//

CSADF::CSADF(int buffersize) : CWindowIndicator(buffersize) {
    m_usedrift = false;
    m_gpumemgb = 2.0;
    m_verbose = false;
};

void CSADF::Init(int maxwindow, int minwindow, int order, bool usedrift, float gpumemgb, bool verbose) {
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

void CSADF::Calculate(float* indata, int size, std::array<std::vector<float>, 2> &outdata)
{
    //m_sadf.resize(size - m_maxw); // has buffesize suppose to be allways enough
    //m_imaxadf.resize(size - m_maxw);

    auto sadf_out = &outdata[0];
    auto imaxadf_out = &outdata[1];

    int ncalculated = sadf(indata, sadf_out->data(), imaxadf_out->data(), size,
        m_maxw, m_minw, m_order, m_usedrift, m_gpumemgb, m_verbose);

    // convert ADF_ERROR to FLT_NAN, so all NANS are consistent FLT_NAN
    std::replace(sadf_out->begin(), sadf_out->end(), ADF_ERROR, FLT_NAN);
}


// Cum sum simetric filter
// +1 for up entries, -1 for down entries

CCumSum::CCumSum(int buffersize) : CWindowIndicator(buffersize) {
    m_cum_up = m_cum_down = 0; m_cum_reset = 1;
};

void CCumSum::Init(double cum_reset) {
    m_cum_reset = cum_reset;
    CWindowIndicator::Init(2);
}

void CCumSum::Calculate(float* indata, int size, std::array<std::vector<float>, 1>& outdata) {

    // i=0 input is the previous value needed to calculate 
    // i=0 on output
    for (int i = 0; i < size - 1; i++) { // output size-1, fill starts i=0

            // check for nans on input, set 0 in that case
            // sadf - due colinearity causing singular matrices on ADF calculation
            // in that case dont calculate - but do not reset cum_sums
            if (std::isnan<float>(indata[i + 1]) || std::isnan<float>(indata[i])) {
                outdata[0][i] = 0;
                continue;
            }

            auto diff = indata[i + 1] - indata[i];
            m_cum_up = std::max(0.0, m_cum_up + diff);
            m_cum_down = std::min(0.0, m_cum_down + diff);
            if (m_cum_up > m_cum_reset) {
                m_cum_up = 0;
                outdata[0][i] = +1;
            }
            else
                if (m_cum_down < -m_cum_reset) {
                    m_cum_down = 0;
                    outdata[0][i] = -1;
                }
                else
                    outdata[0][i] = 0;
    }
}



// CCumSum on SADF(t) values
CCumSumSADF::CCumSumSADF(int buffersize) : CCumSum(buffersize)
{
    m_sadf_prevn = 0;
};

void CCumSumSADF::Init(double cum_reset, CSADF* pSADF){
    m_cum_reset = cum_reset;
    m_sadf_prevn = pSADF->Window() - 1;
    // initialize cum sum filter
    CWindowIndicator::Init(pSADF->Window() + 1); // due 1st diff will also take care of empties
    // there will be valids only after 2 SADF samples  are calculated, all the rest will be empties
    // this the custom refresh function (lambda)
    // using the SADF(t) just calculated data to call the CumSum this->Refresh
    auto thisrefresh = [this, pSADF](int nempties) {
        this->Refresh(pSADF->LastBegin(0), pSADF->End(0)); // buffer 0 is sadf value
    };

    pSADF->addOnRefresh(std::function<void(int)>(thisrefresh));
}

void CCumSumSADF::Calculate(float* indata, int size, std::array<std::vector<float>, 1>& outdata) {
    // TODO: think a better way of doing this
    // will arrive here with pSADF->Window() previous samples --> sadf prev samples + 1 previous samples 
    // but CumSum uses only 1 previous samples 
    CCumSum::Calculate(indata + m_sadf_prevn, size - m_sadf_prevn, outdata);
}


// Volatility on Money Bars average Returns - STDEV on Returns
// also use inside day from money bars for calculation 
// STDDEV is allways positive

void CStdevMbReturn::Init(int window, CMbReturn* pMbReturns) {
    m_mbret_prevn = pMbReturns->Window() - 1;
    CTaSTDDEV::Init(pMbReturns->Window() + window);

    auto thisrefresh = [this, pMbReturns](int nempties) {
        this->Refresh(pMbReturns->LastBegin(0), pMbReturns->End(0));
    };

    pMbReturns->addOnRefresh(std::function<void(int)>(thisrefresh));
}


void CStdevMbReturn::Calculate(double* indata, int size, std::array<std::vector<double>, 1>& outdata)
{   // TODO: should make it the 'correct' way  but not now... 
    CTaSTDDEV::Calculate(indata + m_mbret_prevn, size - m_mbret_prevn, outdata);
}




///// previous implementation
// probably useless but I am unwilling to delete
// due all effort envolved


// Cum sum simetric filter 
// Using valid inday region
// +1 for up entries, -1 for down entries

CCumSumPair::CCumSumPair(int buffersize) : CWindowIndicator(buffersize) {
    m_cum_up = m_cum_down = 0; m_cum_reset = 1;
};

void CCumSumPair::Init(double cum_reset) {
    m_cum_reset = cum_reset;
    CWindowIndicator::Init(2);
}

void CCumSumPair::Calculate(std::pair<float, int>* indata, int size, std::array<std::vector<float>, 1>& outdata) {

    // i=0 input is the previous value needed to calculate 
    // i=0 on output
    for (int i = 0; i < size - 1; i++) { // output size-1, fill starts i=0
        // guarantee that cum sum is calculated only on of hours that are valid operationally
        // only calculate cum sum if second param (int) is True/Valid Sample (1 or 0)
        if (indata[i + 1].second && indata[i].second)
        {
            // check for nans on input, set 0 in that case
            // sadf - due colinearity causing singular matrices on ADF calculation
            // in that case dont calculate - but do not reset cum_sums
            if (std::isnan<float>(indata[i + 1].first) || std::isnan<float>(indata[i].first)) {
                outdata[0][i] = 0;
                continue;
            }

            auto diff = indata[i + 1].first - indata[i].first;
            m_cum_up = std::max(0.0, m_cum_up + diff);
            m_cum_down = std::min(0.0, m_cum_down + diff);
            if (m_cum_up > m_cum_reset) {
                m_cum_up = 0;
                outdata[0][i] = +1;
            }
            else
                if (m_cum_down < -m_cum_reset) {
                    m_cum_down = 0;
                    outdata[0][i] = -1;
                }
                else
                    outdata[0][i] = 0;
        }
        else { // reset on this area, cannot be calculated
            // a new day starting
            m_cum_up = m_cum_down = 0;
            outdata[0][i] = 0;
        }
    }
}



// CCumSum on SADF(t) values
CCumSumSADFPair::CCumSumSADFPair(int buffersize) : CCumSumPair(buffersize)
{
    m_sadf_prevn = 0;
};

void CCumSumSADFPair::Init(double cum_reset, CSADF* pSADF, MoneyBarBuffer* pBars) {
    m_cum_reset = cum_reset;
    m_sadf_prevn = pSADF->Window() - 1;
    // initialize cum sum filter
    CWindowIndicator::Init(pSADF->Window() + 1); // due 1st diff
    // will also take care of empties
    // there will be valids only after 2 SADF samples
    // are calculated, all the rest will be empties
    // this the custom refresh function (lambda)
    // using the SADF(t) just calculated data to call the CumSum this->Refresh
    // region where I dont want cumsum computed (outside valid intraday)   
    auto thisrefresh = [this, pSADF, pBars](int nempties) {
        // make pairs from new data where second indicate wether
        // the value should be used or not on cum_sum
        std::vector<std::pair<float, int>> sadf_pair;
        auto bar = pBars->LastBegin();
        auto sadf = pSADF->LastBegin(0);
        for (; bar != pBars->end() && sadf != pSADF->End(0); bar++, sadf++)
            sadf_pair.push_back(std::make_pair(*sadf, bar->inday));
        this->Refresh(sadf_pair.begin(), sadf_pair.end());
    };

    pSADF->addOnRefresh(std::function<void(int)>(thisrefresh));
}

void CCumSumSADFPair::Calculate(std::pair<float, int>* indata, int size, std::array<std::vector<float>, 1>& outdata) {
    // TODO: think a better way of doing this
    // will arrive here with pSADF->Window() previous samples --> sadf prev samples + 1 previous samples 
    // but CumSum uses only 1 previous samples 
    CCumSumPair::Calculate(indata + m_sadf_prevn, size - m_sadf_prevn, outdata);
}
