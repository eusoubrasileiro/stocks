#pragma once
#ifndef BUFFERS_H
#define BUFFERS_H

#include <float.h>
#include <vector>
#include <memory>
#include <boost/circular_buffer.hpp>

#define DBL_EMPTY_VALUE DBL_MAX
#define INT_EMPTY_VALUE INT_MAX



// FIFO first in first out
// Buffer single is an array when new data is added
// it deletes the oldest bigger than buffer size
// like a Queue or FIFO
using boost::circular_buffer;

template<class Type>
class buffer : public circular_buffer<Type>
{

public:

    buffer(int size) :
        circular_buffer<Type>::circular_buffer(size) {
        circular_buffer<Type>::resize(size);
    }

    void add(Type element) {
        circular_buffer<Type>::push_back(element);
    }

    // you may want to insert a range smaller than 
    // the full size of elements array 
    // e. g. AddRange(v.begin()+2, v.end());
    void addrange(typename std::vector<Type>::iterator start,
        typename std::vector<Type>::iterator end) {
        for (auto element = start; element != end; ++element)
            circular_buffer<Type>::push_back(*element);
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

}



#endif 