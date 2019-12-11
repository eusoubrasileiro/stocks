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
    // needs bbands window samples + 1 
    // to produce 1 signal output
    CWindowIndicator::Init(window+1);
    bands.Init(window, devs, ma_type);  
}    

int CBandSignal::Calculate(double indata[], int size, int outdata[])
{
    // will arrive here only when indata has 
    // enough sample to calculate at least 
    // 1 band-signal value 
    // and consequently at least 
    // 2 bollinger bands values
    bands.Refresh(indata, size); // update bands 

    // indata[] will allways have >= m_window-1 previous samples 
    // that is enough for >= 1 output signal
    m_ncalculated = size - (m_window - 1);

    // where starts the new calculated data on bands
    int idx_data_bands = bands.size() - bands.m_ncalculated;

    // newdata[] might have samples that just 'created' empty ones
    // calculated samples dont have empty samples
    int inew = bands.m_nempty;

    // start of samples
    for (int i = 1; i < size; i++) {
        if (indata[inew + i] >= bands[idx_data_bands + i].up &&
            indata[inew + i - 1] < bands[idx_data_bands + i - 1].up)
            outdata[i - 1] = -1; // sell
        else
            if (indata[inew + i] <= bands[idx_data_bands + i].down &&
                indata[inew + i - 1] > bands[idx_data_bands + i - 1].down)
                outdata[i - 1] = +1; // buy
            else
                outdata[i - 1] = 0; // nothing
    }

    return m_ncalculated;
}


