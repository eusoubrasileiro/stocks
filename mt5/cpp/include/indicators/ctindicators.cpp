#include "cwindicators.h"
#include <array>


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

void CTaMA::Init(int window, int tama_type){
    m_tama_type = tama_type;
    CWindowIndicator::Init(window);
};

void CTaMA::Calculate(double* indata, int size, std::array<std::vector<double>, 1> &outdata)
{
    taMA(0, size, indata, m_window, m_tama_type, outdata[0].data());
}


// STDDEV

void CTaSTDDEV::Init(int window){ CWindowIndicator::Init(window); };

void CTaSTDDEV::Calculate(double* indata, int size, std::array<std::vector<double>, 1> &outdata)
{
    taSTDDEV(0,  size, indata, m_window, outdata[0].data());
}

// BBANDS
// triple buffer indicator, but only 2 used middle is trashed
// cleanear code with templates

void CTaBBANDS::Init(int window, double devs, int ma_type) {
    m_devs = devs;
    m_tama_type = ma_type;
    CWindowIndicator::Init(window);
};

void CTaBBANDS::Calculate(double* indata, int size, std::array<std::vector<double>, 2> &outdata)
{
    int ncalculated = taBBANDS(0, // start the calculation at index
        size, // end the calculation at index
        indata,
        m_window, // From 1 to 100000  - MA window
        m_devs,
        m_tama_type, // MA type
        outdata[0].data(),
        m_out_middle.data(), // not used just trashed
        outdata[1].data());
}
