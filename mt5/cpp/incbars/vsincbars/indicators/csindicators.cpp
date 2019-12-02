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

int CFracDiffIndicator::Calculate(double indata[], int size, double outdata[])
{
    return FracDifApply(indata, size, outdata);
}


// Based on a bollinger band defined by upper-band and lower-band
// calculate signals:
// buy   1 : crossing down-outside it's buy
// sell -1 : crossing up-outside it's sell
// hold  0 : nothing usefull happend

void CBandSignal::Init(int window, double devs, int ma_type) {
    bands.Init(window, devs, ma_type);
}    

bool CBandSignal::Refresh(double newdata[], int count) {
    bool result = bands.Refresh(newdata, count); // update bands 
    addempty(bands.m_nempty);
    if (!result)
        return false;
    // at least one calculated sample on the tripple buffer
    // newdata[] have samples that just 'created' empty ones
    // calculated samples dont have empty samples
    int inew = bands.m_nempty;
    for (int i=0; i <bands.m_calculated; i++) {
        if (newdata[inew+i] > bands.m_out_upper[i])
            add(-1); // sell
        else
            if (newdata[inew+i] < bands.m_out_down[i])
                add(+1); // buy
            else
                add(0); // nothing
    }
    return true;
}