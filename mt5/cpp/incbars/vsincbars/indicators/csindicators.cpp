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
    pbands.reset(new CTaBBANDS(window, devs, ma_type, bfsize));
    CCBuffer::SetSize(bfsize);
}    

bool CBandSignal::Refresh(double newdata[], int start, int count) {
    bool result = pbands->Refresh(newdata, start, count); // update bands 
    AddEmpty(pbands->m_nempty);
    if (!result)
        return false;
    // at least one calculated sample on the tripple buffer
    // newdata[] have samples that just 'created' empty ones
    // calculated samples dont have empty samples
    int inew = pbands->m_nempty;
    for (int i=0; i <pbands->m_calculated; i++) {
        if (newdata[inew+i] > pbands->m_out_upper[i])
            Add(-1); // sell
        else
            if (newdata[inew+i] < pbands->m_out_down[i])
                Add(+1); // buy
            else
                Add(0); // nothing
    }
    return true;
}