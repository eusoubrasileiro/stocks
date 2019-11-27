#include "cwindicators.h"
#include "pytorchcpp.h"

/////////////////////////////
//// special indicators ////
///////////////////////////

CFracDiffIndicator::CFracDiffIndicator(int window, double dfraction, int size){
    m_dfraction = dfraction;
    CWindowIndicator::Init(window);
    setFracDifCoefs(m_dfraction, window);
    CCBuffer::SetSize(size);
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

CBandSignal::CBandSignal(int window, double devs, int ma_type, int bfsize) {
    CWindowIndicator::Init(window);
    pbands.reset(new CTaBBANDS(window, devs, ma_type, bfsize));
    CCBuffer::SetSize(bfsize);
}    

bool CBandSignal::Refresh(double newdata[], int start, int count){
   return pbands->Refresh(newdata, start, count) &&   // update bands window mechanism
    CWindowIndicator::Refresh(newdata, start, count); // same for ... window ...
}

int CBandSignal::Calculate(double indata[], int size, double outdata[]) {
    // at least one calculated sample
    for (int i=0; i<size; i++) {
        if (indata[i] > pbands->m_upper[i])
            outdata[i] = -1; // sell
        else
            if (indata[i] < pbands->m_down[i])
                outdata[i] = +1; // buy
            else
                outdata[i] = 0; // nothing
    }
    return size;
}
