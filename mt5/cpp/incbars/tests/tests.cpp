#include "pch.h"
#include "../vsincbars/include/buffers.h"
#include <array>

// dst, src, dst_start, src_start, count
template<class Type>
void ArrayCopy(void* dst, void* src, int dst_start, int src_start, int count) {
    //ArrayCopy(dest, sbuffer.m_data, 0, start1, count);
    memcpy((Type*)dst + dst_start, (Type*)src + src_start, count * sizeof(Type));
}

TEST(TestCCBuffer, Add) {

    CCBuffer<double> sbuffer(16); // buffer size 16
    for (int i = 0; i < 12; i++)
        sbuffer.Add(0);
    for (int i = 1; i < 7; i++) // exceeds the buffer
        sbuffer.Add(i);

    EXPECT_EQ(sbuffer[15], 5);
    EXPECT_EQ(sbuffer[16], 6);
    EXPECT_EQ(sbuffer[13], 3);
    EXPECT_EQ(sbuffer[14], 4);
    //EXPECT_TRUE(true);
}

TEST(TestCBuffer, Add){
    CBuffer<double> sbuffer;
    sbuffer.Resize(4);
    for (int i = 0; i < 12; i++) // minimum buffer size is 16
        sbuffer.Add(0);
    for (int i = 1; i < 7; i++) // exceeds the buffer
        sbuffer.Add(i);

    EXPECT_EQ(sbuffer.GetData(0), 6);
    EXPECT_EQ(sbuffer.GetData(1), 5);
    EXPECT_EQ(sbuffer.GetData(3), 3);
    EXPECT_EQ(sbuffer.GetData(2), 4);
}

TEST(CCBuffer, Indexes_Data) {
    CCBuffer<double> sbuffer(5); // buffer size 5

    for (int i = 0; i < 3; i++)
        sbuffer.Add(0);
    for (int i = 1; i < 5; i++) // exceeds the buffer
        sbuffer.Add(i);

    int start1, start2, end1, end2;
    double dest[4];
    int parts, count;

    // get 4 elements
    // beginning at the first 0
    parts = sbuffer.indexesData(0, 4, start1, end1, start2, end2);  // get 4 elements

    count = end1 - start1;
    // dst, src, dst_start, src_start, count
    ArrayCopy<double>(dest, sbuffer.m_data.data(), 0, start1, count);    
    ArrayCopy<double>(dest, sbuffer.m_data.data(), count, start2, end2 - start2);

    std::vector<double> array_dest;
    array_dest.resize(4);
    memcpy(array_dest.data(), dest, 4*sizeof(double));

    ASSERT_THAT(array_dest, testing::ElementsAre(0, 1, 2, 3));
    EXPECT_EQ(parts, 2);

    // get 1 element
    // beginning at the 4th
    parts = sbuffer.indexesData(4, 1, start1, end1, start2, end2);

    count = end1 - start1;
    ArrayCopy<double>(dest, sbuffer.m_data.data(), 0, start1, count);
    ArrayCopy<double>(dest, sbuffer.m_data.data(), count, start2, end2 - start2);

    array_dest.resize(1);
    memcpy(array_dest.data(), dest, 1 * sizeof(double));

    ASSERT_THAT(array_dest, testing::ElementsAre(4));
    EXPECT_EQ(parts, 1);

    // get 3 elements
    // beginning at 0
    int arr_parts[4]; // start1, end1, start2, end2
    parts = sbuffer.indexesData(0, 3, arr_parts);

    count = arr_parts[1] - arr_parts[0];
    ArrayCopy<double>(dest, sbuffer.m_data.data(), 0, arr_parts[0], count);
    ArrayCopy<double>(dest, sbuffer.m_data.data(), count,
        arr_parts[2], arr_parts[3] - arr_parts[2]);

    array_dest.resize(3);
    memcpy(array_dest.data(), dest, 3 * sizeof(double));

    ASSERT_THAT(array_dest, testing::ElementsAre(0, 1, 2));
    EXPECT_EQ(parts, 1);
}