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
long long days, secs;
int remdays, remsecs, remyears;
int qc_cycles, c_cycles, q_cycles;
int years, months;
int wday, yday, leap;

// get week day can be used as unique day identifier
// for backtesting
int timestampWDay(long long t)
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

// you might have more than one bar with same 
// time in ms ... that's entirely possible ...
// week day and unique identifier are a must for labelling
