#include "pch.h"
#include <array>
#include "cwindicators.h"

// Since I just want to test the code
// and I dont want to put all code in the export table of the main dll
// I pass to the linker
//$(SolutionDir)\$(Platform)\csindicators.obj
//$(SolutionDir)\$(Platform)\ctindicators.obj
//$(SolutionDir)\$(Platform)\buffers.obj
//$(SolutionDir)\$(Platform)\ticks.obj
//$(SolutionDir)\$(Platform)\stdafx.obj
//$(SolutionDir)\$(Platform)\callpython.obj
// 4. pytorchcpp.lib - dlls required
// 5. ctalib.lib - dll required

#ifdef DEBUG
#include <fstream> // debugging dll load by metatrader 5 output to txt file -> located where it started
std::ofstream debugfile("testing_file.txt");
#else
#define debugfile std::cout
#endif

// also remember that Python3 (python3.dll and others) is a dependency due
// python interpreter embedding
// so you must use `vs2019_py37.bat` script or `setpy37.bat` prior to launch vstudio
// when debugging

#define EXPECT_FLOATS_NEARLY_EQ(expected, actual, size, thresh) \
        for(size_t idx = 0; idx < size; ++idx) \
        { \
            EXPECT_NEAR(expected[idx], actual[idx], thresh) << "at index: " << idx;\
        }
        //EXPECT_EQ(expected.size(), actual.size()) << "Array sizes differ."; \

// distance between arrays/vectors
#define EXPECT_FLOATS_AVG_DIST_NEARLY(expected, actual, size, thresh) \
        auto dist=0;\
        for(size_t idx = 0; idx < size; ++idx) \
        { \
           dist += sqrt(pow(expected[idx]-actual[idx], 2)); \
        } \
        EXPECT_EQ(dist <= thresh, true) << dist << std::endl;

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
        CFracDiffIndicator c_fdiff;
        c_fdiff.Init(fsize, 0.56); 

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

        c_fdiff.Refresh(a, 7);
        c_fdiff.Refresh(b, 33);
        c_fdiff.Refresh(c, 11);
        c_fdiff.Refresh(d, 49);

        std::vector<double> pyfractruth_indicator;
        pyfractruth_indicator.resize(100);
        // prepend (size - 1) EMPTY_VALUES of indicator
        std::fill(pyfractruth_indicator.begin(), pyfractruth_indicator.end(), DBL_EMPTY_VALUE);

        ArrayCopy<double>(pyfractruth_indicator.data(), pyfractruth, fsize - 1, 0, 100-(fsize-1));

        EXPECT_FLOATS_NEARLY_EQ(pyfractruth_indicator, c_fdiff.begin(), 100, 0.001);
 }

TEST(Indicators, CTaMAIndicator){
    double in[] = { 1, 1, 2, 3, 4, 5, 5, 3, 1 };
    int size = 8;
    int window = 2;
    CTaMAIndicator cta_MA;
    cta_MA.Init(window, 0);	 // simple MA

    // partial calls until total array calculated
    double a[] = { 1 };
    double b[] = { 1 };
    double c[] = { 2, 3, 4, 5, 5 };
    double d[] = { 3, 1 };
    cta_MA.Refresh(a, 1);
    cta_MA.Refresh(b, 1);
    cta_MA.Refresh(c, 5);
    cta_MA.Refresh(d, 2);

    std::vector<double> ta_truth = { DBL_EMPTY_VALUE, 1. , 1.5, 2.5, 3.5, 4.5, 5. , 4. , 2. };

    EXPECT_FLOATS_NEARLY_EQ(ta_truth, cta_MA.begin(), 9, 0.01);

}



TEST(Indicators, validIdx) {
    int window = 2;
    CTaMAIndicator cta_MA;
    cta_MA.Init(window, 0);	 // simple MA

    double a[] = { 1 };
    double c[] = { 3, 4, 5, 5 };
    cta_MA.Refresh(a, 1);
    EXPECT_EQ(cta_MA.valididx(), -1);
    cta_MA.Refresh(a, 1);
    EXPECT_EQ(cta_MA.valididx(), 1);
    cta_MA.Refresh(c, 4);
    EXPECT_EQ(cta_MA.valididx(), 1);

    // fills the buffer
    for(int i=0; cta_MA.size()<BUFFERSIZE; i++)
        cta_MA.Refresh(c, 1);

    double last = cta_MA[BUFFERSIZE - 1];

    EXPECT_EQ(cta_MA.valididx(), 1);

    // overwrites the first
    for (int i = 0; i < window-1; i++)
        cta_MA.Refresh(a, 1);

    EXPECT_EQ(cta_MA.valididx(), 0);
}


TEST(Indicators, CTaBBANDS){
    double in[] = { 1, 1, 2, 3, 4, 5, 5, 3, 1 };
    std::vector<double> ta_truth_upper = { DBL_EMPTY_VALUE,  1.  , 2.75, 3.75, 4.75, 5.75, 5.  , 6.5 , 4.5 };
    std::vector<double> ta_truth_down = { DBL_EMPTY_VALUE, 1.  ,  0.25,  1.25,  2.25,  3.25,  5.  ,  1.5 , -0.5 };

    int size = 8;
    int window = 2;
    CTaBBANDS cta_BBANDS;
    cta_BBANDS.Init(window, 2.5, 0); // simple MA + 2.5 deviations

    // partial calls until total array calculated
    double a[] = { 1 };
    double b[] = { 1 };
    double c[] = { 2, 3, 4, 5, 5 };
    double d[] = { 3, 1 };
    cta_BBANDS.Refresh(a, 1);
    cta_BBANDS.Refresh(b, 1);
    cta_BBANDS.Refresh(c, 5);
    cta_BBANDS.Refresh(d, 2);

    double *up, *down;
    up = new double[cta_BBANDS.size()];
    down = new double[cta_BBANDS.size()];
    for (int i = 0; i < cta_BBANDS.size(); i++) {
        up[i] = cta_BBANDS[i].up;
        down[i] = cta_BBANDS[i].down;
    }
    
    EXPECT_FLOATS_NEARLY_EQ(ta_truth_upper, up, 9, 0.001);
    EXPECT_FLOATS_NEARLY_EQ(ta_truth_down, down, 9, 0.001);

    delete[] up;
    delete[] down;
}

// Python code - base for test
//x = np.array([1, 1, 1, 2, 5, 5, 5, 5, 1, 1, 1,  8, 5, 5], dtype = np.double)
//up, med, down = ta.BBANDS(x, 5, 1.5, 1.5, matype=2)
//plt.figure(figsize = (10, 4))
//plt.plot(x, '-k.')
//plt.plot(up, '-b.', lw = 0.5)
//plt.plot(down, '-r.', lw = 0.5)
//plt.grid()
TEST(Indicators, CBandSignal) {
    double in[14] = { 1, 1, 1, 2, 5, 5, 5, 5, 1, 1, 1, 8, 5, 5};
    std::vector<int> truth_band_signal = { INT_EMPTY_VALUE, INT_EMPTY_VALUE,
                                        INT_EMPTY_VALUE, INT_EMPTY_VALUE,
                                        INT_EMPTY_VALUE,   0,   0,   0,  
                                         1,   0,   0,  -1,  0,  0};
    int window = 5;
    int bfsize = 20;
    CBandSignal cbsignal;
    cbsignal.Init(window, 1.5, 2); // 1.5 deviations + WeightedMA=2

    // partial calls until total array calculated
    double a[] = { 1 };
    double b[] = { 1 };
    double c[] = { 1, 2, 5, 5, 5};
    double d[] = { 5, 1, 1, 1};
    double e[] = { 8, 5, 5 };
    cbsignal.Refresh(a, 1);
    cbsignal.Refresh(b, 1);
    cbsignal.Refresh(c, 5);
    cbsignal.Refresh(d, 4);
    cbsignal.Refresh(e, 3);

    EXPECT_FLOATS_NEARLY_EQ(truth_band_signal, cbsignal.begin(), 14, 0.0001);
}

#include <iostream>
#include <fstream>
#include "databuffers.h"

TEST(MoneyBars, InitializeNoSADF) {
    char symbol[6] = "PETR4"; // is null terminated by default - char* string literal C++

    CppDataBuffersInit(0.01, 0.01, 25E6, symbol, false, 250, 
        200, 15, false, 200, 5);
}


TEST(MoneyBars, OnTicks) {
    std::fstream fh;
    std::streampos begin, end;

    // calculate number of ticks on file
    std::string user_data = std::string(std::getenv("USERPROFILE")) + 
        std::string("\\Projects\\stocks\\data\\PETR4_mqltick.bin");

    std::vector<MqlTick> ticks;
    double lost_ticks;

    // Read a file and simulate CopyTicksRange 

    int64_t nticks = ReadTicks(&ticks, user_data, (size_t) 1e6); // 1MM
    BufferMqlTicks* pticks = GetBufferMqlTicks();                                                                 
    // send in chunck of 250k ticks
    size_t chunck_s = (size_t)250e3;

    auto next_timebg = CppOnTicks(ticks.data(), chunck_s, &lost_ticks);
    EXPECT_EQ(pticks->size(), chunck_s);

    // get allowed overlapping of 1ms
    // get idx first tick with time >= next_timebg 
    auto next_idx = MqltickTimeGtEqIdx(ticks, next_timebg);    
    auto overlap = chunck_s - next_idx;
    next_timebg = CppOnTicks(ticks.data()+ next_idx, chunck_s + overlap, &lost_ticks);
    EXPECT_EQ(pticks->size(), chunck_s*2);

    // get allowed overlapping of 1ms
    next_idx = MqltickTimeGtEqIdx(ticks, next_timebg);
    overlap = 2*chunck_s - next_idx;
    next_timebg = CppOnTicks(ticks.data()+ next_idx, chunck_s + overlap, &lost_ticks);
    EXPECT_EQ(pticks->size(), chunck_s*3);

    // get allowed overlapping of 1ms
    next_idx = MqltickTimeGtEqIdx(ticks, next_timebg);
    overlap = 3*chunck_s - next_idx;
    next_timebg = CppOnTicks(ticks.data() + next_idx, chunck_s + overlap, &lost_ticks);

    EXPECT_EQ(pticks->size(), chunck_s*4);   

}

TEST(MoneyBars, OnTicksSADF) {
    std::fstream fh;
    std::streampos begin, end;

    char symbol[6] = "PETR4"; // is null terminated by default - char* string literal C++
    double lost_ticks; // qc for real time 

    // load SADF indicator
    CppDataBuffersInit(0.01, 0.01, 25E6, symbol, true, 250, 200, 15, false, 200, 5);

    // calculate number of ticks on file
    std::string user_data = std::string(std::getenv("USERPROFILE")) +
        std::string("\\Projects\\stocks\\data\\PETR4_mqltick.bin");

    std::vector<MqlTick> ticks;
    
    // Read a file and simulate CopyTicksRange 

    int64_t nticks = ReadTicks(&ticks, user_data, (size_t)1e6); // 1MM
    BufferMqlTicks* pticks = GetBufferMqlTicks();
    // send in chunck of 250k ticks
    size_t chunck_s = (size_t)250e3;

    auto next_timebg = CppOnTicks(ticks.data(), chunck_s, &lost_ticks);
    EXPECT_EQ(pticks->size(), chunck_s);

    // get allowed overlapping of 1ms
    // get idx first tick with time >= next_timebg 
    auto next_idx = MqltickTimeGtEqIdx(ticks, next_timebg);
    auto overlap = chunck_s - next_idx;
    next_timebg = CppOnTicks(ticks.data() + next_idx, chunck_s + overlap, &lost_ticks);
    EXPECT_EQ(pticks->size(), chunck_s * 2);

    // get allowed overlapping of 1ms
    next_idx = MqltickTimeGtEqIdx(ticks, next_timebg);
    overlap = 2 * chunck_s - next_idx;
    next_timebg = CppOnTicks(ticks.data() + next_idx, chunck_s + overlap, &lost_ticks);
    EXPECT_EQ(pticks->size(), chunck_s * 3);

    // get allowed overlapping of 1ms
    next_idx = MqltickTimeGtEqIdx(ticks, next_timebg);
    overlap = 3 * chunck_s - next_idx;
    next_timebg = CppOnTicks(ticks.data() + next_idx, chunck_s + overlap, &lost_ticks);

    EXPECT_EQ(pticks->size(), chunck_s * 4);

}

TEST(Indicators, CSADFIndicator) {

    std::vector<float> data = { 56097., 55298., 56865., 56653., 57348., 57701., 57669., 58005.,
       56534., 56754., 57829., 59265., 59365., 58546., 58600., 59083.,
       59806., 59962., 59921., 59147., 59956., 60646., 61723., 61927.,
       62312., 62386., 62486., 62953., 62904., 62770., 63072., 64567.,
       64593., 65217., 65224., 65917., 65831., 65530., 63998., 65692.,
       65039., 65368., 66142., 66204., 65820., 65943., 65241., 65959.,
       65812., 66810., 67782., 66964., 65114., 66017., 66908., 66704.,
       66385., 68394., 68257., 67749., 67684., 67730., 67296., 66860.,
       65828., 65813., 66685., 66037., 65079., 64872., 64511., 65216.,
       64284., 63529., 63691., 62923., 61738., 61293., 63058., 62106.,
       61955., 62699., 63010., 62618., 62494., 61539., 61971., 61750.,
       62198., 61691., 61820., 62424., 62104., 60821., 61220., 60365.,
       59786., 59702., 59445., 57540., 56238., 55888., 54038., 54513.,
       56590., 55039., 54619., 54063., 54463., 55213., 54633., 53798.,
       54490., 53403., 53417., 52481., 54156., 54430., 54001., 55049.,
       55651., 55352., 56105., 56195., 57195., 57167., 55505., 55440.,
       53805., 53837., 53109., 52652., 54355., 54693., 55780., 56077.,
       56379., 55394., 53706., 53569., 53421., 54331., 53402., 53909.,
       54583., 55347., 54195., 53034., 52639., 52608. };
    std::vector<float> outsadf;
    std::vector<float> outlag;

    int maxw = 15;
    int minw = 12;
    int p = 3;
    auto iSADF = new CSADFIndicator(BUFFERSIZE);
    
    iSADF->Init(maxw, minw, p, true);
    // send in 50, 70, 30
    iSADF->Refresh(data.data(), 15);
    iSADF->Refresh(data.data()+15, 2);
    iSADF->Refresh(data.data()+17, 53);
    iSADF->Refresh(data.data()+70, 50);
    iSADF->Refresh(data.data()+120, 30);
    //sadf(, outsadf.data(), outlag.data(), data.size(), maxw, minw, p, true, 0.1, false);

    // to write assert here
    std::vector<float> pytruth = { -1.2095627e+00, -1.4402076e+00, -1.6808732e+00, -8.9917880e-01,
        -1.1911615e+00, -1.8827139e-01, -1.5914007e+00, -2.0884252e+00,
        -1.4725261e+00, -1.5353931e-01, 3.2273871e-01, 8.7458380e-02,
        -1.4796472e+00, -1.8691136e+00, -6.1389482e-01, 2.5796735e-01,
        -1.1457052e+00, -1.1822526e+00, -1.7153320e+00, -6.9972336e-01,
        -1.0656174e-01, -7.8773433e-01, -1.2573199e+00, -1.4819880e+00,
        8.5943109e-01, -5.7501823e-04, -3.0131486e-01, -6.2936342e-01,
        -1.6031090e+00, -1.6053603e+00, -1.3931369e+00, -1.4971341e+00,
        -2.1554575e+00, -1.3260804e+00, -1.8027709e+00, -2.2736332e+00,
        -2.1547198e+00, -2.4526870e+00, -2.4543948e+00, -1.9520944e+00,
        -2.4168279e+00, -1.3161443e+00, -1.3896236e+00, -1.4856383e+00,
        -1.6633970e+00, -8.6810178e-01, -1.2970024e+00, -1.2186456e+00,
        -5.4864550e-01, -2.6997941e-02, 5.0628555e-01, 1.3616739e-01,
        -1.0838879e+00, -1.4998595e+00, -1.6178182e+00, -2.2483847e+00,
        -2.3357701e+00, -3.0228729e+00, -3.1349971e+00, -3.4466634e+00,
        -4.4314680e+00, -4.4593606e+00, -7.3222607e-01, -2.3811510e-01,
        -1.4038742e+00, -1.5871313e+00, -8.8612109e-01, -5.4530567e-01,
        -1.5910079e-01, -4.0298826e-01, -9.7372913e-01, -2.1507022e+00,
        -2.5917152e-01, -4.2912278e-01, -1.1722835e+00, -1.1688999e+00,
        -1.7137966e+00, -1.0298523e+00, -1.1124365e+00, -1.6498510e+00,
        -1.3983605e+00, -5.0248623e-01, 2.0532690e-01, -3.5882545e-01,
        -8.0650300e-01, -2.4845923e-01, -2.6110181e-01, 4.8257938e-01,
        1.0190715e+00, -6.9883698e-01, -2.6373601e+00, -2.6548662e+00,
        -2.9443104e+00, -4.5649247e+00, -1.1798327e+00, -1.4723967e+00,
        -1.9377854e+00, -2.6119814e+00, -4.2328801e+00, -3.7486374e+00,
        -2.1418855e+00, -9.2513096e-01, -1.6293188e+00, -1.4976473e+00,
        -1.0545456e+00, -3.8125318e-01, -1.5899116e-01, -6.5612316e-01,
        -8.9917529e-01, -2.3258533e+00, -2.6458335e+00, -2.2123275e+00,
        -1.6656014e+00, -4.9412382e-01, 8.7294698e-01, -4.5874131e-01,
        -8.2211846e-01, -1.0594745e+00, -2.5537829e+00, -1.5482895e+00,
        -8.4954745e-01, -1.8571659e+00, -2.1523619e+00, -2.1859550e+00,
        -2.2981637e+00, -2.7850459e+00, -2.1055303e+00, -2.4616945e+00,
        -1.2686000e+00, -1.8379117e+00, -1.6301438e+00, -3.0783081e-01,
        -1.1272501e+00, -1.2535882e+00, -1.2739995e+00 };

    std::vector<float> sadf;
    sadf.resize(iSADF->nvalid());
    for (int i = iSADF->valididx(); i < iSADF->size(); i++)
        sadf[i- iSADF->valididx()] = iSADF->at(i).sadf;

    //auto dist = 0; 
    //for (size_t idx = 0; idx < iSADF->nvalid(); ++idx)
    //{ 
    //    dist += sqrt(pow(pytruth[idx] - sadf[idx], 2));
    //} 
    //EXPECT_EQ(dist <= 0.01, true) << dist << std::endl;

    EXPECT_FLOATS_AVG_DIST_NEARLY(pytruth, sadf.begin(), iSADF->nvalid(), 0.04);
}

// need to use DLL Load to Deal with 
// more reallistic enviorment of mt5