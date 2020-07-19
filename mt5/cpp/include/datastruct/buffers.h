#pragma once
#ifndef BUFFERS_H
#define BUFFERS_H

#include <stdint.h>
#include <float.h>
#include <vector>
#include <memory>
#include <boost/circular_buffer.hpp>

#define DBL_EMPTY_VALUE DBL_MAX
#define INT_EMPTY_VALUE INT_MAX
#define FLT_EMPTY_VALUE FLT_MAX

// indicators, bars etc. buffer needed all must respect this to compatibility of indexes between DLL's
#define BUFFERSIZE       1000000      

// FIFO first in first out
// Buffer single is an array when new data is added
// it deletes the oldest bigger than buffer size
// like a Queue or FIFO
using boost::circular_buffer;


// fixed size buffer max - BUFFERSIZE

template<class Type>
class buffer : public circular_buffer<Type>
{

public:

    // ring-buffer iterator
    typedef typename circular_buffer<Type>::iterator iterator;
    // ring-buffer const iterator
    typedef typename circular_buffer<Type>::const_iterator const_iterator;
        
    buffer() : circular_buffer<Type>::circular_buffer(BUFFERSIZE) {};

    buffer(int bufsize) : circular_buffer<Type>::circular_buffer(bufsize) {};

    void add(Type element) {
        circular_buffer<Type>::push_back(element);
    }

    // addrange from c style array
    void addrange(Type values[], int count) {
        for (int i = 0; i < count; i++)
            circular_buffer<Type>::push_back(values[i]);
    }

    // you may want to insert a range smaller than 
    // the full size of elements array 
    // e. g. AddRange(v.begin()+2, v.end());
    template<class iterator_type>
    void addrange(iterator_type start,
        iterator_type end) {
        //for (auto element = start; element != end; ++element)
        //    circular_buffer<Type>::push_back(*element);
        circular_buffer<Type>::insert(this->end(), start, end);
    }

    void removelast(void) {
        circular_buffer<Type>::pop_back();
    }

    void addempty(int count);
};

// dst, src, dst_start, src_start, count
template<class Type>
void ArrayCopy(void* dst, void* src, int dst_start, int src_start, int count) {
    //ArrayCopy(dest, sbuffer.m_data, 0, start1, count);
    memcpy((Type*)dst + dst_start, (Type*)src + src_start, count * sizeof(Type));
    //std::copy()
}



#endif 