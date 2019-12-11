#include "cwindicators.h"
#include "pytorchcpp.h"

/////////////////////////////
//// special indicators ////
///////////////////////////

void CFracDiffIndicator::Init(int window, double dfraction){
    m_dfraction = dfraction;
    CWindowIndicator::Init(window);
    setFracDifCoefs(m_dfraction, window);
};

void CFracDiffIndicator::Calculate(double indata[], int size, double outdata[])
{
    FracDifApply(indata, size, outdata);
}


// Based on a bollinger band defined by upper-band and lower-band
// calculate signals:
// buy   1 : crossing down-outside it's buy
// sell -1 : crossing up-outside it's sell
// hold  0 : nothing usefull happend

void CBandSignal::Init(int window, double devs, int ma_type) {
    // needs bbands window 2 samples
    // to produce 1 signal output
    CWindowIndicator::Init(window+1);
    bands.Init(window, devs, ma_type);  
}    

int CBandSignal::Refresh(double newdata[], int count) {
    // mwindow-1 size
    bands.Refresh(newdata, count); // update bands     
    // mwindow size
    return CWindowIndicator::Refresh(newdata, count); 
}

void CBandSignal::Calculate(double indata[], int size, int outdata[])
{
    // will come here only when 
    // size with >= m_window samples
    // first valid sample in indata is at 
    // m_window - 1
    // but bellow I work using -1 so lets add + 1
    int instart = (m_window - 1);
    // where starts the new calculated data on bands
    // but bellow I work using -1 so lets add + 1
    int idx_bands = bands.size() - bands.m_ncalculated;

    // this is uggly and possibly wrong?
    // effect of m_window difference of 1 
    // between bbands and this
    // seen just on first call?
    if (bands[idx_bands - 1].up == DBL_EMPTY_VALUE) // or idx_bands == bands.valididx() 
        idx_bands++;

    for (int i = instart; i < size; i++, idx_bands++) {
        if (indata[i] >= bands[idx_bands].up &&
            indata[i - 1] < bands[idx_bands - 1].up)
            outdata[i - instart] = -1; // sell
        else
            if (indata[i] <= bands[idx_bands].down &&
                indata[i - 1] > bands[idx_bands - 1].down)
                outdata[i - instart] = +1; // buy
            else
                outdata[i - instart] = 0; // nothing
    }
}


