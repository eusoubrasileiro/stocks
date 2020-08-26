#pragma once
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <algorithm>
#include <cmath>
#include "databuffers.h"
#include "..\events\events.h"

// Indicator SADF
// just for reload case store these
extern std::shared_ptr<CSADF> m_SADFi;
extern int m_minw, m_maxw, m_order;
extern bool m_usedrift;

// Indicators
extern std::shared_ptr<CCumSumSADF> m_CumSumi;
extern std::shared_ptr<CMbReturn> m_MbReturn; // Standard Deviation of Average Returns of Money Bars
extern std::shared_ptr<CStdevMbReturn> m_StdevMbReturn;
extern std::shared_ptr<std::vector<Event>> m_Events;
extern std::shared_ptr<CFracDiff> m_FdMb;

void IndicatorsInit(int maxwindow,
    int minwindow,
    int order,
    bool usedrift,
    float cum_reset=1.0);
