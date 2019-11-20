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

CTaMAIndicator::CTaMAIndicator(int window, int tama_type){
    m_tama_type = tama_type;
    CWindowIndicator::Init(window);
};

int CTaMAIndicator::Calculate(double indata[], int size, double outdata[])
{
    return taMA(0, size, indata, m_window, m_tama_type, outdata);
}


// STDDEV

CTaSTDDEV::CTaSTDDEV(int window){ CWindowIndicator::Init(window); };

int CTaSTDDEV::Calculate(double indata[], int size, double outdata[])
{
    return taSTDDEV(0,  size, indata, m_window, outdata);
}



// BBANDS


// triple buffer indicator
// cleanear code with multiple inheritance

// nonsense but needed by eincbands constructor
CTaBBANDS::CTaBBANDS() {
    m_devs = 2.5;
    m_tama_type = 1;
    IWindowIndicator::Init(1);
};

CTaBBANDS::CTaBBANDS(int window, double devs, int ma_type){
    m_devs = devs;
    m_tama_type = ma_type;
    IWindowIndicator::Init(window);
};

void CTaBBANDS::AddEmpty(int count){
    m_upper.AddEmpty(count);
    m_middle.AddEmpty(count);
    m_down.AddEmpty(count);
}

int CTaBBANDS::Calculate(double indata[], int size, double outdata[])
{
    m_out_upper.resize(size);
    m_out_middle.resize(size);
    m_out_down.resize(size);

    return taBBANDS(0, // start the calculation at index
        size, // end the calculation at index
        indata,
        m_window, // From 1 to 100000  - MA window
        m_devs,
        m_tama_type, // MA type
        m_out_upper.data(),
        m_out_middle.data(),
        m_out_down.data());
}

void CTaBBANDS::AddRange(std::vector<double>::iterator start, std::vector<double>::iterator end){
    // totally ignores signature since I have to add in three buffers
    m_upper.AddRange(m_out_upper.begin(), m_out_upper.begin()+m_calculated);
    m_middle.AddRange(m_out_middle.begin(), m_out_middle.begin()+m_calculated);
    m_down.AddRange(m_out_down.begin(), m_out_down.begin()+m_calculated);
};

void CTaBBANDS::SetSize(const int size)
{
    m_upper.SetSize(size);
    m_middle.SetSize(size); 
    m_down.SetSize(size);
}

int CTaBBANDS::Size(){
    return m_middle.Size();
}
