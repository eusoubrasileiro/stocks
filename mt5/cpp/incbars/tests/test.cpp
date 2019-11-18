#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "../vsincbars/buffers.h"
#include <array>

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

//TEST(TestCBuffer, Add){
//    CBuffer<double> sbuffer();
//    sbuffer.Resize(4);
//    for (int i = 0; i < 12; i++) // minimum buffer size is 16
//        sbuffer.Add(0);
//    for (int i = 1; i < 7; i++) // exceeds the buffer
//        sbuffer.Add(i);
//
//    if (sbuffer.GetData(0) == 6 && sbuffer.GetData(1) == 5 &&
//        sbuffer.GetData(2) == 4 && sbuffer.GetData(3) == 3)
//        Print("Passed - Test CBuffer");
//    else
//        Print("Failed - Test CBuffer");
//}

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
    //ArrayCopy(dest, sbuffer.m_data, 0, start1, count);
    memcpy(dest, &sbuffer.m_data[start1], count*sizeof(double));
    //ArrayCopy(dest, sbuffer.m_data, count, start2, end2 - start2);
    memcpy(dest, &sbuffer.m_data[start2], (end2-start2) * sizeof(double));

    std::array<double, 4> array_dest;
    std::copy_n(std::begin(array_dest), 4, dest);

    ASSERT_THAT(array_dest, testing::ElementsAre(0, 1, 2, 3));
    EXPECT_EQ(parts, 2);
 


    //// get 1 element
    //// beginning at the 4th
    //parts = sbuffer.indexesData(4, 1, start1, end1, start2, end2);

    //count = end1 - start1;
    //ArrayCopy(dest, sbuffer.m_data, 0, start1, count);
    //ArrayCopy(dest, sbuffer.m_data, count, start2, end2 - start2);
    //double assert2[1] = { 4 };
    //if (parts == 1 && almostEqual(assert2, dest, 1, 1e-8))
    //    Print("Passed - Test CCBuffer indexesData 2");
    //else
    //    Print("Failed - Test CCBuffer indexesData 2");

    //// get 2 elements
    //// beginning at 0
    //int arr_parts[4]; // start1, end1, start2, end2
    //parts = sbuffer.indexesData(0, 3, arr_parts);

    //count = arr_parts[1] - arr_parts[0];
    //ArrayCopy(dest, sbuffer.m_data, 0, arr_parts[0], count);
    //ArrayCopy(dest, sbuffer.m_data, count,
    //    arr_parts[2], arr_parts[3] - arr_parts[2]);
    //double assert3[3] = { 0, 1, 2 };
    //if (parts == 1 && almostEqual(assert3, dest, 1, 1e-8))
    //    Print("Passed - Test CCBuffer indexesData 3");
    //else
    //    Print("Failed - Test CCBuffer indexesData 3");

}