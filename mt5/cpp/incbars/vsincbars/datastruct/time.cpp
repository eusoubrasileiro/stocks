#include "time.h"

// from here
//https://stackoverflow.com/questions/21593692/convert-unix-timestamp-to-date-without-system-libs
// and here
//http://git.musl-libc.org/cgit/musl/tree/src/time/__secs_to_tm.c?h=v0.9.15

// 2000-03-01 (mod 400 year, immediately after feb29 */
#define LEAPOCH (946684800 + 86400*(31+29))
#define DAYS_PER_400Y (365*400 + 97)
#define DAYS_PER_100Y (365*100 + 24)
#define DAYS_PER_4Y   (365*4   + 1)

static const char days_in_month[] = { 31,30,31,30,31,31,30,31,30,31,31,29 };
long days, secs;
int remdays, remsecs, remyears;
int qc_cycles, c_cycles, q_cycles;
int years, months;
int wday, yday, leap;

// get week day can be used as unique day identifier
// for backtesting
int timestampWDay(long t)
{
	// static const char days_in_month[] = {31,30,31,30,31,31,30,31,30,31,31,29};
	// long days, secs;
	// int remdays, remsecs, remyears;
	// int qc_cycles, c_cycles, q_cycles;
	// int years, months;
	// int wday, yday, leap;
	  //* Reject time_t values whose year would overflow int */
	  //if (t < INT_MIN * 31622400LL || t > INT_MAX * 31622400LL)
	  //	return -1;
	secs = t - LEAPOCH;
	days = secs / 86400;
	remsecs = secs % 86400;
	if (remsecs < 0) {
		remsecs += 86400;
		days--;
	}

	wday = (3 + days) % 7;
	if (wday < 0) wday += 7;

	return wday;
}

// to be replaced
// Use std::sort to sort your std::vector.
// After sorting your std::vector use std::binary_search to find out whether a certain element exists 
// in your std::vector or use std::lower_bound or 
// std::upper_bound to find and get an element from your std::vector.

// code from clong array
// quick search for a sorted array
int CCTimeDayBuffer::m_QuickSearch(long element, int start, int end)
{
	int  i, j, m = -1;
	long t_long;
	//--- search
	i = start;
	j = end - 1;
	while (j >= i)
	{
		//--- ">>1" is quick division by 2
		m = (j + i) >> 1;
		if (m < start || m >= end)
			break;
		t_long = m_data[m].ms;
		if (t_long == element)
			break;
		if (t_long > element)
			j = m - 1;
		else
			i = m + 1;
	}
	//--- position
	return (t_long == element) ? m : -1;
}

int CCTimeDayBuffer::QuickSearch(long element_ms)
{
	int nparts, start, end, ifound, i, parts[4];
	ifound = -1;
	nparts = indexesData(0, Count(), parts);
	for (i = 0; i < nparts; i++) {
		start = parts[i * 2];
		end = parts[1 + i * 2];
		ifound = m_QuickSearch(element_ms, start, end);
		if (ifound != -1)
			break;
	}
	// convert to start based index
	return (ifound != -1) ? toIndex(ifound) : ifound;
}

int CCTimeDayBuffer::QuickSearch(timeday element) {
	return QuickSearch(element.ms);
}

// the only way to make this go faster is
// by calling in parallel when range has a certain size
void CCTimeDayBuffer::AddTimeMs(long time_ms) {
	// convert ms timestamp to linux epoch (time-zone is already accounted for)
	m_last.ms = time_ms;
	m_last.day = timestampWDay(time_ms / 1000);
	Add(m_last);
}


// you may want to insert a range smaller than the full size of elements array
void CCTimeDayBuffer::AddRangeTimeMs(long time_ms[], int start, int tsize) {
	for (int i = start; i < tsize; i++)
		AddTimeMs(time_ms[i]);
}