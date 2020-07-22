#include "buffers.h"

// template specialization for double
template<>
void buffer<double>::addempty(int count) { // add count samples with EMPTY value
	for (int i = 0; i < count; i++)
		add(DBL_NAN);
}

template<>
void buffer<int>::addempty(int count) { // add count samples with EMPTY value
    for (int i = 0; i < count; i++)
        add(INT_NAN);
}

template<>
void buffer<float>::addempty(int count) { // add count samples with EMPTY value
    for (int i = 0; i < count; i++)
        add(FLT_NAN);
}

