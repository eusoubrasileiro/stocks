#pragma once
#ifndef BUFFERS_H
#define BUFFERS_H

#include <float.h>
#include <vector>
#include <memory>

#define DBL_EMPTY_VALUE DBL_MAX
#define INT_EMPTY_VALUE INT_MAX


// dst, src, dst_start, src_start, count
template<class Type>
void ArrayCopy(void* dst, void* src, int dst_start, int src_start, int count) {
    //ArrayCopy(dest, sbuffer.m_data, 0, start1, count);
    memcpy((Type*)dst + dst_start, (Type*)src + src_start, count * sizeof(Type));

}

// FIFO first in first out
// Buffer single is an array when new data is added
// it deletes the oldest bigger than buffer size
// like a Queue or FIFO
// Will not use series index convention of mql5 unecessary pain
// oldest samples are in the begging of array
// only one method will cover mql5 as series convention that GetData
// Works based on ArrayCopy so pay attention that:
// Array of classes and structures containing objects that require initialization aren't copied.
// An array of structures can be copied into an array of the same type only.

template<class Type>
class CBuffer
{
protected:
	int m_data_total;
	int m_data_max;
	int m_step_resize;

	void MakeSpace(int space_needed) {
		if (space_needed >= m_data_max) { // otherwise bellow will
			m_data_total = 0; // resize m_data array space
			return;
		}
		// copy data overwriting the oldest sample - overwriting the firsts
		// (shift left array)  making space for new samples in the end
		// dst, src, dst_idx_srt, src_idx_srt, count_to_copy
        // void * memcpy ( void * destination, const void * source, size_t num );
		ArrayCopy<Type>(m_data.data(), m_data.data(), 0, space_needed, m_data_total - space_needed);
		m_data_total -= space_needed;
	}

public:
	std::vector<Type> m_data;

	CBuffer(void) {
		m_data_total = 0;
		m_data_max = 0;
		m_step_resize = 16;
	}

	Type operator[](const int index) const { return m_data[index]; }

	int Count(void) { return m_data_total; }

	void RemoveLast(void) { if (m_data_total > 0) m_data_total--; }

	bool Add(Type element) {
		if (m_data_total >= m_data_max) // m_data
			MakeSpace(1);
		//--- add data in the end
		m_data[m_data_total++] = element;
		return(true);
	}

	// you may want to insert a range smaller than the full size
	// of elements array
	void AddRange(Type elements[], int start, int end) {		
		//start = (tsize > m_data_max)? tsize-m_data_max : start;
		for (int i = start; i < end; i++)
			Add(elements[i]);
	}

	void Resize(const int size)
	{
		m_data_max = size;
		m_data.resize(size);
	}

	// Get Data At index position using Array As Series Convention
	// youngest sample is at (0)
	Type GetData(const int index) const
	{
		return(m_data[m_data_total - 1 - index]);
	}

	void SetData(const int index, Type value)
	{
		m_data[m_data_total - 1 - index] = value;
	}

	int BufferSize() { return m_data_max; }

};

//////////////////////////////////
// Circular Buffers - Fixed Size//
/////////////////////////////////

// much faster because dont need to ArrayCopy
// everything once full

template<class Type>
class CCBuffer
{
protected:
	int m_data_max;
	int m_intdiv;

public:
	std::vector<Type> m_data;
	int m_cposition; // current position of end of data
	int isfull;

    CCBuffer() { SetSize(1); };

	CCBuffer(int size) {
		SetSize(size);
	}

	void SetSize(int size) {
		isfull = 0;
		m_data_max = size;
		m_cposition = 0;
        m_data.resize(size);
    }

	Type operator[](const int index) const {
		return m_data[(isfull != 0) ? (m_cposition + index - 1) % m_data_max : index];
	}
	// convert to index based on start of data
	// from absolute index
	int toIndex(int abs_index)
	{
		if (isfull != 0 && m_cposition != 0)
			return (abs_index >= m_cposition) ? abs_index - m_cposition : abs_index + (m_data_max - m_cposition);
		return abs_index;
	}

	int Count(void) { return (isfull != 0) ? m_data_max : m_cposition; }

	void RemoveLast(void) {
		if (m_cposition == 0)
			m_cposition = m_data_max - 1;
		else
			m_cposition--;
	}

	void Add(Type element) {
		//--- add data in the end
		m_data[m_cposition] = element;
		m_cposition++;
		// integer division
		m_intdiv = m_cposition / m_data_max; // if bigger is gonna be 1
		m_cposition -= m_intdiv * m_data_max; // same as making (m_cposition +1)%m_data_max
		isfull = (isfull != 0) ? isfull : m_intdiv;
	}

	// you may want to insert a range smaller than 
	// the full size of elements array 
	// e. g. AddRange(v.begin()+2, v.end());
	void AddRange(typename std::vector<Type>::iterator start, 
		typename std::vector<Type>::iterator end){
		for (auto element = start; element != end; ++element)
			Add(*element);
	}

    void AddRange(Type *carray, int csize) {
        for (int i=0; i<csize; i++)
            Add(carray[i]);
    }


	int Size() { return m_data_max; }

	// given begin - (included)
	// -- index based on start of data
	// and count
	// -- number of elements to copy
	// returns indexes of data to performs copy
	// in two parts or one part
	// start and end of two parts
	// depending on the m_cposition
	// return 1 or 2 depending if two parts
	// or only 1 part needed to be copied
	int indexesData(int  begin, int count,
		int& start1, int& end1,
		int& start2, int& end2) {
		int tmpcount = Count();
		if (count > m_data_max || count > tmpcount || tmpcount == 0)
			return 0;
		if (m_cposition != 0 && isfull != 0) {
			// buffer has two parts full filled
			start1 = (m_cposition + begin);
			if (start1 > m_data_max) {
				start1 %= m_data_max;
				end1 = start1 + count; // cannot go around again
				// otherwise count would be bigger than m_data_max
				start2 = 0;
				end2 = 0;
				return 1;
			}
			end1 = start1 + count;
			if (end1 > m_data_max) {
				start2 = 0;
				end2 = end1 % m_data_max;
				end1 = m_data_max;
				return 2;
			}
			start2 = 0;
			end2 = 0;
			return 1;
		}
		// one part only
		start1 = begin;
		end1 = begin + count;
		start2 = 0;
		end2 = 0;
		return 1;
	}

	// parts[4] = start1, end1, start2, end2
	int indexesData(int  begin, int count, int parts[])
	{
		return indexesData(begin, count, parts[0], parts[1], parts[2], parts[3]);
	}

    void AddEmpty(int count);

};



#endif 