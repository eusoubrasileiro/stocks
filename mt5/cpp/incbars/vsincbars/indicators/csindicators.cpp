#include "cwindicators.h"
#include "pytorchcpp.h"

// nonsense but needed by eincbands constructor
CFracDiffIndicator::CFracDiffIndicator() {
    m_dfraction = 0.5;
    CWindowIndicator::Init(1);
}

CFracDiffIndicator::CFracDiffIndicator(int window, double dfraction){
    m_dfraction = dfraction;
    CWindowIndicator::Init(window);
    setFracDifCoefs(m_dfraction, window);
};

int CFracDiffIndicator::Calculate(double indata[], int size, double outdata[])
{
    return FracDifApply(indata, size, outdata);
}