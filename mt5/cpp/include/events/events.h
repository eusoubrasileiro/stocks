#pragma once
#include <time.h>
#include "buffers.h"


#define CLEAN_EVENT_LABEL(event) { event.ret = 0; event.side = 0; event.tdone = 0; }

// Traininig samples

#pragma pack(push, 2)
// Marcos uses dataframe for events that get filled with:
// expected volatility, sl, tp, return realized, labelled class
// here the better seams to use stl container of PODs
// adding new fields as needed
struct Event { // cumsum +1/-1 on sadf signal on money bars
    unixtime_ms time = 0; // time it was identified
    uint64_t twhen = 0; // when that happend - uid money bars
    uint64_t tdone = 0; // when that finished profiting - uid money bars
    int   cumsum = 0; // what sign cum sum 1/-1
    int side = 0; // -1/1 sell/buy - will be labeled
    int vertbar; // vertical barrier in money bars used
    int y = 0; // y label value, 1 sucess, -1 failure or 
    // 0 Stopped by vertical barrier number of bars:
    // 1. use the sign of the return to label as 1 or -1 or 0 nonetheless
    // 2. stopped by end of operational day  (not very good statistically)
    // -7 : I will not use this since it has not meaningfull relation 
    // with money bars (volume of money being negotiated - buy or sell intention)
    // better create another classifier to test if an entry will cross to another day
    // sample weight simpler 0 is bad 1 is good - 1 have higher weight?
    float ret = 0.; // return or profit in %
    float sltp = 0.;
    // information span 
    uint64_t inf_start = 0;
    uint64_t inf_end = 0;
   // int nconc; // number of concurrent labels within the time-span of this sample
   // uint64_t tnext; // time to next training sample
};
#pragma pack(pop)


std::pair<std::vector<Event>, bool> LabelEvent(Event& event, double mtgt, int barrier_str, int nbarriers, int barrier_inc);

bool FillinXFeatures(Event& event, std::vector<double>& X, int batch_size);

// return labelled and featured events
std::pair<std::vector<Event>, std::vector<std::vector<double>>> 
LabelEvents(std::vector<Event>& events, double mtgt,
    int barrier_str, int nbarriers, int barrier_inc, int batch_size);



