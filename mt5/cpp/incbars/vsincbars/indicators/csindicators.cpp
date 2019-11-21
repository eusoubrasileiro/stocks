#include "cwindicators.h"
#include "pytorchcpp.h"

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