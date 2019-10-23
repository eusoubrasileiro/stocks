#include "CWindowIndicator.mqh"
#include "..\Util.mqh"

class CFracDiffIndicator : public CWindowIndicator
{
protected:
  double m_dfraction; // fractional difference
  double m_fcoefs[]; // filter coeficients

public:

    CFracDiffIndicator(int window, double dfraction){
        m_dfraction = dfraction;
        CWindowIndicator::Init(window);
        FracDifCoefs(m_dfraction, window, m_fcoefs);
    };

    int Calculate(double &indata[], int size, double &outdata[])
    {
      return FracDifApply(indata, size, m_fcoefs, m_window, outdata);
    }

};
