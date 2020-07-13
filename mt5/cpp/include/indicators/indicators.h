#pragma once
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <algorithm>
#include <cmath>
#include "databuffers.h"

// Indicator SADF
// just for reload case store these
extern std::shared_ptr<CSADFIndicator> m_SADFi;
extern int m_minw, m_maxw, m_order;
extern bool m_usedrift;

// Indicator Cum Sum
extern double m_reset;
extern std::shared_ptr<CCumSumSADFIndicator> m_CumSumi;

void IndicatorsInit(int maxwindow,
    int minwindow,
    int order,
    bool usedrift);
