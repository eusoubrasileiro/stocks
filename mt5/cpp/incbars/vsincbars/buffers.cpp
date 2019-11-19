#include "buffers.h"

// template specialization for double
template<>
void CCBuffer<double>::AddEmpty(int count) { // add count samples with EMPTY value
	for (int i = 0; i < count; i++)
		Add(EMPTY_VALUE);
}

// dst, src, dst_start, src_start, count
template<class Type>
void ArrayCopy(void *dst, void *src, int dst_start, int src_start, int count){
    //ArrayCopy(dest, sbuffer.m_data, 0, start1, count);
    memcpy((Type*) dst + dst_start, (Type*) src + src_start, count * sizeof(Type));

}
