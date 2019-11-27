#include "pch.h"
#include "..\vsincbars\include\buffers.h"
#include "..\vsincbars\include\cwindicators.h"
#include "..\vsincbars\include\ticks.h"
#include <array>

// Since I just want to test the code
// and I dont want to put all code in the export table of the main dll
// I pass to the linker
// 1. ctindicators.obj
// 2. csindicators.obj
// 3. buffers.obj
// 4. pytorchcpp.lib - dlls required
// 5. ctalib.lib - dll required


#define EXPECT_FLOATS_NEARLY_EQ(expected, actual, size, thresh) \
        for(size_t idx = 0; idx < size; ++idx) \
        { \
            EXPECT_NEAR(expected[idx], actual[idx], thresh) << "at index: " << idx;\
        }
        //EXPECT_EQ(expected.size(), actual.size()) << "Array sizes differ."; \


TEST(CBuffer, Add) {
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

TEST(CCBuffer, Add) {

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

//ENUM_DEFINE(TA_MAType_SMA, Sma) = 0,
//ENUM_DEFINE(TA_MAType_EMA, Ema) = 1,
//ENUM_DEFINE(TA_MAType_WMA, Wma) = 2,
//ENUM_DEFINE(TA_MAType_DEMA, Dema) = 3,
//ENUM_DEFINE(TA_MAType_TEMA, Tema) = 4,
//ENUM_DEFINE(TA_MAType_TRIMA, Trima) = 5,
//ENUM_DEFINE(TA_MAType_KAMA, Kama) = 6,
//ENUM_DEFINE(TA_MAType_MAMA, Mama) = 7,
//ENUM_DEFINE(TA_MAType_T3, T3) = 8
TEST(Indicators, CFracDiffIndicator){
    // from Python
    double in[100] = { 0.23575223, 0.89348613, 0.43196633, 0.86018474, 0.59765737, // a
                   0.02537023, 0.7332872 , 0.4622992 , 0.96278162, 0.33838066,   // a
                   0.89851506, 0.90982346, 0.54238173, 0.68145741, 0.95061314,  // b
                   0.93442722, 0.2614748 , 0.00405999, 0.75008525, 0.64118048,  // b
                   0.32741177, 0.77945483, 0.10501869, 0.52248356, 0.569884  ,  // b
                   0.3254099 , 0.2309258 , 0.88311545, 0.5860409 , 0.8447372 ,  // b
                   0.81948561, 0.33338482, 0.30032552, 0.82731009, 0.68584524,  // b
                   0.99592614, 0.69919862, 0.88728765, 0.6370904 , 0.25931114,  // b
                   0.77664975, 0.00191125, 0.06043727, 0.81997762, 0.62892824,  // c
                   0.08543346, 0.77194193, 0.93350418, 0.41672853, 0.89452214,  // c
                   0.9859954 ,                                                  // c
                   0.23336376, 0.50903137, 0.36534072, 0.36527723,              // d
                   0.51881977, 0.38566805, 0.59163601, 0.8162303 , 0.48761801, // d
                   0.05340729, 0.80001708, 0.86127951, 0.72706986, 0.36959402, // d
                   0.11954723, 0.00799683, 0.56896299, 0.40747309, 0.50314072, // d
                   0.31325538, 0.3277913 , 0.75168065, 0.61296086, 0.16180291, // d
                   0.53711416, 0.37934352, 0.36650789, 0.22024702, 0.52589008, // d
                   0.12388826, 0.87836527, 0.20104567, 0.77184266, 0.28660699, // d
                   0.40697238, 0.11326645, 0.43511667, 0.96272681, 0.60386761, // d
                   0.01621923, 0.04200219, 0.93858386, 0.88837139, 0.20881766, // d
                   0.77023781, 0.30574534, 0.74073346, 0.62654889, 0.64183077 }; // d

    double pyfractruth[91] = { -0.35131172,
                    0.49928454,  0.25017423, -0.17289674,  0.15147366,
                    0.37461599,  0.1987023 , -0.49814806, -0.39431162,  0.57265172,
                    0.10862605, -0.19898058,  0.42240801, -0.4724218 ,  0.28296601,
                    0.16482447, -0.12555987, -0.09936503,  0.62507281, -0.01262697,
                    0.34505668,  0.17906916, -0.32295314, -0.1007382 ,  0.49591081,
                    0.08619913,  0.42358569, -0.05327547,  0.26209244, -0.07781242,
                   -0.33216276,
                   0.41951532, -0.5924319 , -0.13944678,  0.6705722 ,
                    0.0858732 , -0.42139628,  0.56218567,  0.40226974, -0.26175034,
                    0.4584858 ,  0.3245855 , -0.52701549,  0.13536349, -0.08450255,
                   -0.00425219,  0.16583032, -0.04016165,  0.23173595,  0.35197123,
                   -0.11896925, -0.40198151,  0.60707809,  0.31157253,  0.07938527,
                   -0.23780636, -0.28918318, -0.22134125,  0.45148736,  0.01694105,
                    0.1555569 , -0.09026478,  0.02155778,  0.45794614,  0.0894013 ,
                   -0.33406492,  0.28526841, -0.03589293,  0.01742604, -0.11140591,
                    0.282847  , -0.26990253,  0.68218233, -0.37655569,  0.49942645,
                   -0.25859327,  0.08411683, -0.24043617,  0.24573357,  0.62889871,
                   -0.04567096, -0.50740424, -0.13925831,  0.81331429,  0.2939484 ,
                   -0.45141761,  0.45552484, -0.26285934,  0.38984909,  0.07518225,
                    0.11830923 };
        double eps = 0.0001; // error tolerance in comparison
        int fsize = 10;
        CFracDiffIndicator c_fdiff(fsize, 0.56, 200); // whatever buffer size just need to be enough

        // partial calls until total array calculated
        double a[7];
        // dst, src, dst_idx_srt, src_idx_srt, count_to_copy
        ArrayCopy<double>(a, in, 0, 0, 7);
        double b[33];
        ArrayCopy<double>(b, in, 0, 7, 33); // 7:40 [left open - not included
        double c[11];
        ArrayCopy<double>(c, in, 0, 40, 11); // 40:51
        double d[49];
        ArrayCopy<double>(d, in, 0, 51, 49); // 51:100 = 49

        c_fdiff.Refresh(a, 0, 7);
        c_fdiff.Refresh(b, 0, 33);
        c_fdiff.Refresh(c, 0, 11);
        c_fdiff.Refresh(d, 0, 49);

        std::vector<double> pyfractruth_indicator;
        pyfractruth_indicator.resize(100);
        // prepend (size - 1) EMPTY_VALUES of indicator
        std::fill(pyfractruth_indicator.begin(), pyfractruth_indicator.end(), DBL_EMPTY_VALUE);

        ArrayCopy<double>(pyfractruth_indicator.data(), pyfractruth, fsize - 1, 0, 100-(fsize-1));

        EXPECT_FLOATS_NEARLY_EQ(pyfractruth_indicator, c_fdiff.m_data, 100, 0.001);
 }

TEST(Indicators, CTaMAIndicator){
    double in[] = { 1, 1, 2, 3, 4, 5, 5, 3, 1 };
    int size = 8;
    int window = 2;
    CTaMAIndicator cta_MA(window, 0);	 // simple MA
    cta_MA.SetSize(10);

    // partial calls until total array calculated
    double a[] = { 1 };
    double b[] = { 1 };
    double c[] = { 2, 3, 4, 5, 5 };
    double d[] = { 3, 1 };
    cta_MA.Refresh(a, 0, 1);
    cta_MA.Refresh(b, 0, 1);
    cta_MA.Refresh(c, 0, 5);
    cta_MA.Refresh(d, 0, 2);

    std::vector<double> ta_truth = { DBL_EMPTY_VALUE, 1. , 1.5, 2.5, 3.5, 4.5, 5. , 4. , 2. };

    EXPECT_FLOATS_NEARLY_EQ(ta_truth, cta_MA.m_data, 9, 0.001);

}

TEST(Indicators, CTaBBANDS){
    double in[] = { 1, 1, 2, 3, 4, 5, 5, 3, 1 };
    std::vector<double> ta_truth_upper = { DBL_EMPTY_VALUE,  1.  , 2.75, 3.75, 4.75, 5.75, 5.  , 6.5 , 4.5 };
    std::vector<double> ta_truth_middle = { DBL_EMPTY_VALUE, 1. , 1.5, 2.5, 3.5, 4.5, 5. , 4. , 2. };
    std::vector<double> ta_truth_down = { DBL_EMPTY_VALUE, 1.  ,  0.25,  1.25,  2.25,  3.25,  5.  ,  1.5 , -0.5 };

    int size = 8;
    int window = 2;
    CTaBBANDS cta_BBANDS(window, 2.5, 0, 10); // simple MA + 2.5 deviations

    // partial calls until total array calculated
    double a[] = { 1 };
    double b[] = { 1 };
    double c[] = { 2, 3, 4, 5, 5 };
    double d[] = { 3, 1 };
    cta_BBANDS.Refresh(a, 0, 1);
    cta_BBANDS.Refresh(b, 0, 1);
    cta_BBANDS.Refresh(c, 0, 5);
    cta_BBANDS.Refresh(d, 0, 2);

    EXPECT_FLOATS_NEARLY_EQ(ta_truth_upper, cta_BBANDS.m_upper.m_data, 9, 0.001);
    EXPECT_FLOATS_NEARLY_EQ(ta_truth_middle, cta_BBANDS.m_middle.m_data, 9, 0.001);
    EXPECT_FLOATS_NEARLY_EQ(ta_truth_down, cta_BBANDS.m_down.m_data, 9, 0.001);
}

TEST(Indicators, CBandSignal) {
    double in[] = { 1, 1, 1, 2, 5, 5, 5, 5, 1, 1, 1};
    std::vector<double> truth_band_signal = { DBL_EMPTY_VALUE, DBL_EMPTY_VALUE,
                                        DBL_EMPTY_VALUE, DBL_EMPTY_VALUE,
                                        1,   0,   0,   0,  -1,   0,   0};
    int window = 5;
    int bfsize = 20;
    CBandSignal cbsignal(window, 1.5, 2, bfsize); // 1.5 deviations + WeightedMA=2

    // partial calls until total array calculated
    double a[] = { 1 };
    double b[] = { 1 };
    double c[] = { 1, 2, 5, 5, 5};
    double d[] = { 5, 1, 1, 1};
    cbsignal.Refresh(a, 0, 1);
    cbsignal.Refresh(b, 0, 1);
    cbsignal.Refresh(c, 0, 5);
    cbsignal.Refresh(d, 0, 4);

    EXPECT_FLOATS_NEARLY_EQ(truth_band_signal, cbsignal.m_data, 11, 0.0001);
}

#include <iostream>
#include <fstream>

TEST(MoneyBarBuffer, AddTicks) {
    std::fstream fh;
    std::streampos begin, end;

    // calculate number of ticks on file
    fh.open("npticks.bin", std::fstream::in | std::fstream::binary);
    begin = fh.tellg();
    fh.seekg(0, std::ios::end);
    end = fh.tellg();
    long nticks= (end- begin)/sizeof(MqlTick);
    fh.seekg(0, std::ios::beg);

    MqlTick* cstyle;
    cstyle = new MqlTick[nticks];
    fh.read((char*) cstyle, end);
    std::vector<MqlTick> ticks(cstyle, cstyle+nticks);

    delete[] cstyle;


}

typedef int(__stdcall* funcpyTrainModel)(double*, int*, int, int, char*, int);
typedef int(__stdcall* funcpyPredictwModel)(double*, int, char*, int);

void test_pyTrainModel(funcpyTrainModel pyTrainModel) {
    // xor example
    double X[] = { 0, 0, 1, 1, 0, 1, 1, 0 };  // second dim = 2
    int y[] = { 0, 0, 1, 1 }; // first dim = 4
    int xdim = 2;
    int ntrain = 4;
    char* strmodel = (char*)malloc(1024 * 500); // 500 Kb
    int modelsize = pyTrainModel(X, y, 4, 2, strmodel, 1024 * 1500);
    EXPECT_TRUE(modelsize > 0);
}

void test_pyTrainAndPredict(funcpyTrainModel pyTrainModel, funcpyPredictwModel pyPredictwModel) {
    // xor example
    double X[] = { 0, 0, 1, 1, 0, 1, 1, 0 };  // second dim = 2
    int y[] = { 0, 0, 1, 1 }; // first dim = 4
    int ypred = -5;
    int xdim = 2;
    int ntrain = 4;
    char* strmodel = (char*)malloc(1024 * 500); // 500 Kb
    int modelsize = pyTrainModel(X, y, 4, 2, strmodel, 1024 * 1500);
    EXPECT_TRUE(modelsize > 0);
    // what is the prediction for [0, 1] = should be 1
    ypred = pyPredictwModel(&X[4], 2, strmodel, modelsize);
    EXPECT_EQ(ypred, 1);
}


// Testing with LoadLibrary is more realistic for Mt5
TEST(DllMain, PyTrain_n_Predict){
    HINSTANCE hinstDll;
    FARPROC ProcAdd = NULL;
    FARPROC ProcAdd_1 = NULL;
    std::string pyfile = R"(#script only for testing
import numpy as np
from sklearn.ensemble import ExtraTreesClassifier
import pickle

def pyTrainModel(X, y) :
    trees = ExtraTreesClassifier(n_estimators = 120, verbose = 0)
    trees.fit(X, y)
    # save model
    str_trees = pickle.dumps(trees)
    return str_trees # easier to pass to C++

def pyPredictwModel(X, str_trees) :
    trees = pickle.loads(str_trees);
    return trees.predict(X)[0]
)";

    std::ofstream out("python_code.py");
    out << pyfile;
    out.close();

    for (int i = 0; i < 3; i++) { // call 3 times the same function loading and unloading the dll
    // Get a handle to the DLL module.
        hinstDll = LoadLibrary(TEXT("incbars.dll"));
        EXPECT_TRUE(hinstDll != NULL);
        // If the handle is valid, try to get the function address.
        if (hinstDll != NULL) {
            ProcAdd = GetProcAddress(hinstDll, "pyTrainModel");
            ProcAdd_1 = GetProcAddress(hinstDll, "pyPredictwModel");
            EXPECT_TRUE(ProcAdd != NULL);
            EXPECT_TRUE(ProcAdd_1 != NULL);
            if (ProcAdd != NULL && ProcAdd_1 != NULL) {
                test_pyTrainAndPredict((funcpyTrainModel)ProcAdd, (funcpyPredictwModel)ProcAdd_1);
            }
            FreeLibrary(hinstDll); // Free the DLL module.
        }
    }
}
