#include "cwindicators.h"
#include "ctalib.h"


// TaMA

//ENUM_DEFINE(TA_MAType_SMA, Sma) = 0,
//ENUM_DEFINE(TA_MAType_EMA, Ema) = 1,
//ENUM_DEFINE(TA_MAType_WMA, Wma) = 2,
//ENUM_DEFINE(TA_MAType_DEMA, Dema) = 3,
//ENUM_DEFINE(TA_MAType_TEMA, Tema) = 4,
//ENUM_DEFINE(TA_MAType_TRIMA, Trima) = 5,
//ENUM_DEFINE(TA_MAType_KAMA, Kama) = 6,
//ENUM_DEFINE(TA_MAType_MAMA, Mama) = 7,
//ENUM_DEFINE(TA_MAType_T3, T3) = 8

void CTaMAIndicator::Init(int window, int tama_type){
    m_tama_type = tama_type;
    CWindowIndicator::Init(window);
};

void CTaMAIndicator::Calculate(double indata[], int size, double outdata[])
{
    taMA(0, size, indata, m_window, m_tama_type, outdata);
}


// STDDEV

void CTaSTDDEV::Init(int window){ CWindowIndicator::Init(window); };

void CTaSTDDEV::Calculate(double indata[], int size, double outdata[])
{
    taSTDDEV(0,  size, indata, m_window, outdata);
}

// BBANDS
// triple buffer indicator
// cleanear code with multiple inheritance and templates

void CTaBBANDS::Init(int window, double devs, int ma_type) {
    m_devs = devs;
    m_tama_type = ma_type;
    IWindowIndicator::Init(window);
};

void CTaBBANDS::AddEmpty(int count) {
    for (int i = 0; i < count; i++)
        add({ DBL_EMPTY_VALUE, DBL_EMPTY_VALUE });
}

void CTaBBANDS::Calculate(double indata[], int size, bands outdata[])
{
    int ncalculated = taBBANDS(0, // start the calculation at index
        size, // end the calculation at index
        indata,
        m_window, // From 1 to 100000  - MA window
        m_devs,
        m_tama_type, // MA type
        m_out_upper.data(),
        m_out_middle.data(),
        m_out_down.data());

    for (int i = 0; i < ncalculated; i++) {
        outdata[i].up = m_out_upper[i];
        outdata[i].down = m_out_down[i];
    }
}
