#pragma once
#include "buffers.h"

// minimum needed for identify day of a tick timestamp in milliseconds
// can be inserted on MoneyBar struct
// and C++ could create array of timedays from
// array of moneybar structs
struct timeday {
	int day; // day of week C code bellow
	long ms;
};


// Circular Buffer Version
// Same as CStructBuffer but for timeday struct
// adding QuickSearch functinality
class CCTimeDayBuffer : public CCBuffer<timeday>
{
	// code from clong array
	// quick search for a sorted array
	int m_QuickSearch(long element, int start, int end);
public:
	timeday     m_last;
	int QuickSearch(long element_ms);
	int QuickSearch(timeday element);
	// the only way to make this go faster is
	// by calling in parallel when range has a certain size
	void AddTimeMs(long time_ms);
	// you may want to insert a range smaller than the full size of elements array
	void AddRangeTimeMs(long time_ms[], int start, int tsize);

};
#pragma once
