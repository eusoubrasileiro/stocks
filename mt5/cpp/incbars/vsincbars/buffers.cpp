#include "buffers.h"

// template specialization for double
template<>
void CCBuffer<double>::AddEmpty(int count) { // add count samples with EMPTY value
	for (int i = 0; i < count; i++)
		Add(EMPTY_VALUE);
}

