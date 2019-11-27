#include "buffers.h"

// template specialization for double
template<>
void CCBuffer<double>::AddEmpty(int count) { // add count samples with EMPTY value
	for (int i = 0; i < count; i++)
		Add(DBL_EMPTY_VALUE);
}

template<>
void CCBuffer<int>::AddEmpty(int count) { // add count samples with EMPTY value
    for (int i = 0; i < count; i++)
        Add(INT_EMPTY_VALUE);
}

