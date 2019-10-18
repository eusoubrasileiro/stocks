#include "..\Buffers.mqh"

// Base clas indicator  for calculations using ctalib
template<typename Type>
class CBufferIndicator : CBuffer<Type>
{
protected:

public:
    CBufferIndicator(void) {};

    // re-calculate indicator values
    bool Refresh(){

    }

};
