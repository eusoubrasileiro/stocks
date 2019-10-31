#include "CWindowIndicator.mqh"


#import "pytorchcpp.dll"
	int FracDifApply(double &signal[], int size, double &output[]);
	void setFracDifCoefs(double d, int size);
#import

class CFracDiffIndicator : public CWindowIndicator
{
protected:
  double m_dfraction; // fractional difference

public:

    CFracDiffIndicator();
    
    CFracDiffIndicator(int window, double dfraction){
        m_dfraction = dfraction;
        CWindowIndicator::Init(window);
        setFracDifCoefs(m_dfraction, window);
    };

    int Calculate(double &indata[], int size, double &outdata[])
    {
      return FracDifApply(indata, size, outdata);
    }

};
