#pragma once
#ifndef BUFFERS_H
#define BUFFERS_H

#include <stdint.h>
#include <float.h>
#include <vector>
#include <memory>
#include <boost/circular_buffer.hpp>

typedef  int64_t unixtime;
typedef  int64_t unixtime_ms;

#define FLT_NAN      std::numeric_limits<float>::quiet_NaN() // quiet NAN dont raise exceptions
#define INT_NAN      INT_MAX // there no nan for ints
#define DBL_NAN      std::numeric_limits<double>::quiet_NaN() // quiet NAN dont raise exceptions
#define DBL_NAN_MT5  DBL_MAX // only seeing by Metatrader 5 as NAN - DBL_EMPTY_VALUE

// indicators, bars etc. buffer needed all must respect this to compatibility of indexes 
// except ticks
// must only not explode memory 
// sizeof(MoneyBar) ~ 96 bytes
// BUFFERSIZE x sizeof(MoneyBar) = 2x96MB = 192MB
#define MB 2<<20
#define BUFFERSIZE       (size_t (2*MB) )

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

    buffer(size_t bufsize) : circular_buffer<Type>::circular_buffer(bufsize) {};

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


//debugging old style
//#define dprintcbuff(buf){ std::cout << "buffer:" ;  \
//                      for (auto it = buf.begin(); it < buf.end(); it++) \
//                        std::cout << " " << *it;  \
//                      std::cout << std::endl;}
//
//#define dprintcbuffArray {std::cout << "array one:";  \
//                          auto arr = std::get<0>(buf.array_one()); \
//                          auto size = std::get<1>(buf.array_one()); \
//                          for (auto it = 0; it < size ; it++) \
//                            std::cout << " " << arr[it];  \
//                          std::cout << std::endl; \
//                      std::cout << "array two:";  \
//                          arr = std::get<0>(buf.array_two()); \
//                          size = std::get<1>(buf.array_two()); \
//                          for (auto it = 0; it < size ; it++) \
//                            std::cout << " " << arr[it];  \
//                          std::cout << std::endl; }\


#endif 