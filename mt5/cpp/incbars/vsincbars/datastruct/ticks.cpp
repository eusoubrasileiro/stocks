#include "ticks.h"

// Fix array of ticks so if last ask, bid or last is 0
// they get filled with previous non-zero value as should be
// volume i dont care because it will only be usefull in change if flag >= 16
void fixArrayTicks(std::vector<MqlTick> ticks){
	int nticks = ticks.capacity();
	double       bid = 0;           // Current Bid price
	double       ask = 0;           // Current Ask price
	double       last = 0;          // Price of the last deal (Last)
	double       volume = 0;
	for (int i = 0; i < nticks; i++) { // cannot be done in parallel?
		if (ticks[i].bid == 0)
			ticks[i].bid = bid;
		else
			bid = ticks[i].bid; // save this for use next
		if (ticks[i].ask == 0)
			ticks[i].ask = ask;
		else
			ask = ticks[i].ask; // save this for use next
		if (ticks[i].last == 0)
			ticks[i].last = last;
		else
			last = ticks[i].last; // save this for use next
	}
}

// circular buffer version
// 10k ticks maximum downloaded every time Refresh is called
// 1 ms time-frame suggested using OnTimer
CCBufferMqlTicks::CCBufferMqlTicks(){
	m_nnew = 0;
	gticks = 0;
}

int CCBufferMqlTicks::nNew() { return m_nnew; } // number of new ticks after calling Refresh()

// just receive ticks from Python/Metatrader and add them

int CCBufferMqlTicks::Refresh(std::vector<MqlTick>::iterator start, 
	std::vector<MqlTick>::iterator end){
	AddRange(start, end);
	m_nnew = std::distance(end, start);
	gticks += m_nnew;
	return m_nnew;
}

int CCBufferMqlTicks::Refresh(MqlTick *carray, int csize) {
    AddRange(carray, csize);
    m_nnew = csize;
    gticks += m_nnew;
    return m_nnew;
}



int CCBufferMqlTicks::indexesNewTicks(int& start1, int& end1,
		int& start2, int& end2) {

		return indexesData(Count() - m_nnew, m_nnew,
			start1, end1, start2, end2);
}