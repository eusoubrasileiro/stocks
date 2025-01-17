#include "pch.h"
#include <array>
#include "indicators.h"

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


//#ifdef DEBUG
//#include <fstream> // debugging dll load by metatrader 5 output to txt file -> located where it started
//std::ofstream debugfile("testing_file.txt");
//#else
//#define debugfile std::cout
//#endif

// also remember that Python3 (python3.dll and others) is a dependency due
// python interpreter embedding
// so you must use `vs2019_py37.bat` script or `setpy37.bat` prior to launch vstudio
// when debugging

#define EXPECT_FLOATS_NEARLY_EQ(type, expected, actual, size, thresh) \
        for(size_t idx = 0; idx < size; ++idx) \
        { \
            auto first = expected[idx]; auto second = actual[idx]; \
            if (std::isnan<type>(second)){ \
                second = std::isnan<type>(second); \
                first = std::isnan<type>(first);  \
                EXPECT_EQ(second == first, true) <<  "at index: " << idx; \
                continue; \
            }\
            EXPECT_NEAR(first, second, thresh) << "at index: " << idx;\
        }
        

// average distance between arrays/vectors
#define EXPECT_FLOATS_AVG_DIST_NEARLY(expected, actual, size, thresh) \
        float dist=0;\
        for(size_t idx = 0; idx < size; ++idx) \
        { \
           dist += pow(expected[idx]-actual[idx], 2); \
        } \
        EXPECT_EQ((sqrt(dist)/size) <= thresh, true) << (dist/size) << std::endl;

//ENUM_DEFINE(TA_MAType_SMA, Sma) = 0,
//ENUM_DEFINE(TA_MAType_EMA, Ema) = 1,
//ENUM_DEFINE(TA_MAType_WMA, Wma) = 2,
//ENUM_DEFINE(TA_MAType_DEMA, Dema) = 3,
//ENUM_DEFINE(TA_MAType_TEMA, Tema) = 4,
//ENUM_DEFINE(TA_MAType_TRIMA, Trima) = 5,
//ENUM_DEFINE(TA_MAType_KAMA, Kama) = 6,
//ENUM_DEFINE(TA_MAType_MAMA, Mama) = 7,
//ENUM_DEFINE(TA_MAType_T3, T3) = 8
TEST(Indicators, CFracDiff){
    // from Python size 100
    std::vector<double> in = { 0.23575223, 0.89348613, 0.43196633, 0.86018474, 0.59765737, // a
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
    // size 91
    std::vector<double> pyfractruth = { -0.35131172,
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
        CFracDiff c_fdiff;
        c_fdiff.Init(fsize, 0.56);

        // partial calls until total array calculated
        std::vector<double> a(7), b(33), c(11), d(49);
        // dst, src, dst_idx_srt, src_idx_srt, count_to_copy
        std::copy_n(in.begin(), 7, a.begin());
        std::copy_n(in.begin()+7, 33, b.begin()); // 7:40 [left open - not included
        std::copy_n(in.begin() + 40, 11, c.begin()); // 40:51
        std::copy_n(in.begin() + 51, 49, d.begin());  // 51:100 = 49

        c_fdiff.Refresh<vec_iterator<double>>(a.begin(), a.end());
        c_fdiff.Refresh<vec_iterator<double>>(b.begin(), b.end());
        c_fdiff.Refresh<vec_iterator<double>>(c.begin(), c.end());
        c_fdiff.Refresh<vec_iterator<double>>(d.begin(), d.end());

        std::vector<double> pyfractruth_indicator;
        pyfractruth_indicator.resize(100);
        // prepend (size - 1) EMPTY_VALUES of indicator
        std::fill(pyfractruth_indicator.begin(), pyfractruth_indicator.end(), DBL_NAN);

        std::copy_n(pyfractruth.begin(), 91, pyfractruth_indicator.begin() + fsize - 1);

        EXPECT_FLOATS_NEARLY_EQ(double, pyfractruth_indicator, c_fdiff.Begin(0), 100, 0.001);
 }

TEST(Indicators, CTaMA){
    double in[] = { 1, 1, 2, 3, 4, 5, 5, 3, 1 };
    int size = 8;
    int window = 2;
    CTaMA cta_MA;
    cta_MA.Init(window, 0);	 // simple MA

    // partial calls until total array calculated
    std::vector<double> a = { 1 };
    std::vector<double> b = { 1 };
    std::vector<double> c = { 2, 3, 4, 5, 5 };
    std::vector<double> d = { 3, 1 };
    cta_MA.Refresh<vec_iterator<double>>(a.begin(), a.end());
    cta_MA.Refresh<vec_iterator<double>>(b.begin(), b.end());
    cta_MA.Refresh<vec_iterator<double>>(c.begin(), c.end());
    cta_MA.Refresh<vec_iterator<double>>(d.begin(), d.end());

    std::vector<double> ta_truth = { DBL_NAN, 1. , 1.5, 2.5, 3.5, 4.5, 5. , 4. , 2. };

    EXPECT_FLOATS_NEARLY_EQ(double, ta_truth, cta_MA.Begin(0), 9, 0.01);
}


// rewrite to use vBegin(0)
TEST(Indicators, vBegin) {
    int window = 2;
    CTaMA cta_MA;
    cta_MA.Init(window, 0);	 // simple MA

    std::vector<double> a = { 1 };
    std::vector<double> c = { 3, 4, 5, 5 };
    cta_MA.Refresh<vec_iterator<double>>(a.begin(), a.end());
    //EXPECT_EQ(cta_MA.valididx(), -1);
    cta_MA.Refresh<vec_iterator<double>>(a.begin(), a.end());
    //EXPECT_EQ(cta_MA.valididx(), 1);
    cta_MA.Refresh<vec_iterator<double>>(c.begin(), c.end());
    //EXPECT_EQ(cta_MA.valididx(), 1);

    // fills the buffer
    for(int i=0; cta_MA.Count()<cta_MA.BufferSize(); i++)
        cta_MA.Refresh<vec_iterator<double>>(c.begin(), c.begin()+1);

    double last = *(cta_MA.End(0)-1);

    // EXPECT_EQ(cta_MA.valididx(), 1);

    // overwrites the first
    for (int i = 0; i < window-1; i++)
        cta_MA.Refresh<vec_iterator<double>>(a.begin(), a.end());

    // EXPECT_EQ(cta_MA.valididx(), 0);
}


TEST(Indicators, CTaBBANDS){
    double in[] = { 1, 1, 2, 3, 4, 5, 5, 3, 1 };
    std::vector<double> ta_truth_upper = { DBL_NAN,  1.  , 2.75, 3.75, 4.75, 5.75, 5.  , 6.5 , 4.5 };
    std::vector<double> ta_truth_down = { DBL_NAN, 1.  ,  0.25,  1.25,  2.25,  3.25,  5.  ,  1.5 , -0.5 };

    int size = 8;
    int window = 2;
    CTaBBANDS cta_BBANDS;
    cta_BBANDS.Init(window, 2.5, 0); // simple MA + 2.5 deviations

    // partial calls until total array calculated
    std::vector<double> a = { 1 };
    std::vector<double> b = { 1 };
    std::vector<double> c = { 2, 3, 4, 5, 5 };
    std::vector<double> d = { 3, 1 };
    cta_BBANDS.Refresh<vec_iterator<double>>(a.begin(), a.end());
    cta_BBANDS.Refresh(b.begin(), b.end());
    cta_BBANDS.Refresh(c.begin(), c.end());
    cta_BBANDS.Refresh(d.begin(), d.end());

    std::vector<double> up(cta_BBANDS.Count());
    std::vector<double> down(cta_BBANDS.Count());    
    for (int i = 0; i < cta_BBANDS.Count(); i++) {
        up[i] = cta_BBANDS.Up(i);
        down[i] = cta_BBANDS.Down(i);
    }

    EXPECT_FLOATS_NEARLY_EQ(double, ta_truth_upper, up, 9, 0.001);
    EXPECT_FLOATS_NEARLY_EQ(double, ta_truth_down, down, 9, 0.001);
}

#include <iostream>
#include <fstream>
#include <databuffers.h>

TEST(MoneyBars, InitializeNoSADF) {
    char symbol[6] = "PETR4"; // is null terminated by default - char* string literal C++

    DataBuffersInit(0.01, 0.01, 25E6, symbol);
    // false, 250, 200, 15, false, 200, 5);
}

// TODO
// re-write since 
// tick losses are acceptable
TEST(MoneyBars, OnTicks) {
    std::fstream fh;
    std::streampos begin, end;

    // calculate number of ticks on file
    std::string user_data = std::string(std::getenv("USERPROFILE")) +
        std::string("\\Projects\\stocks\\data\\PETR4_mqltick.bin");

    std::vector<MqlTick> ticks;
    double lost_ticks;

    char symbol[6] = "PETR4"; // is null terminated by default - char* string literal C++
    DataBuffersInit(0.01, 0.01, 25E6, symbol);

    // Read a file and simulate CopyTicksRange
    int64_t nticks = ReadTicks(&ticks, user_data, (size_t) 1e6); // 1MM
    BufferMqlTicks* pticks = GetBufferMqlTicks();
    // send in chunck of 250k ticks
    size_t chunck_s = (size_t)250e3;

    auto next_timebg = OnTicks(ticks.data(), chunck_s, &lost_ticks);
    EXPECT_EQ(pticks->size(), chunck_s);

    // get allowed overlapping of 1ms
    // get idx first tick with time >= next_timebg
    auto next_idx = MqltickTimeGtEqIdx(ticks, next_timebg);
    auto overlap = chunck_s - next_idx;
    next_timebg = OnTicks(ticks.data()+ next_idx, chunck_s + overlap, &lost_ticks);
    EXPECT_EQ(pticks->size(), chunck_s*2);

    // get allowed overlapping of 1ms
    next_idx = MqltickTimeGtEqIdx(ticks, next_timebg);
    overlap = 2*chunck_s - next_idx;
    next_timebg = OnTicks(ticks.data()+ next_idx, chunck_s + overlap, &lost_ticks);
    EXPECT_EQ(pticks->size(), chunck_s*3);

    // get allowed overlapping of 1ms
    next_idx = MqltickTimeGtEqIdx(ticks, next_timebg);
    overlap = 3*chunck_s - next_idx;
    next_timebg = OnTicks(ticks.data() + next_idx, chunck_s + overlap, &lost_ticks);

    EXPECT_EQ(pticks->size(), chunck_s*4);

}

// TODO
// re-write since 
// tick losses are acceptable
TEST(MoneyBars, OnTicksSADF) {
    std::fstream fh;
    std::streampos begin, end;

    char symbol[6] = "PETR4"; // is null terminated by default - char* string literal C++
    double lost_ticks; // qc for real time

    // load SADF indicator
    DataBuffersInit(0.01, 0.01, 25E6, symbol);
    IndicatorsInit(250, 200, 15, false);

    // calculate number of ticks on file
    std::string user_data = std::string(std::getenv("USERPROFILE")) +
        std::string("\\Projects\\stocks\\data\\PETR4_mqltick.bin");

    std::vector<MqlTick> ticks;

    // Read a file and simulate CopyTicksRange
    int64_t nticks = ReadTicks(&ticks, user_data, (size_t)1e6); // 1MM
    BufferMqlTicks* pticks = GetBufferMqlTicks();
    // send in chunck of 250k ticks
    size_t chunck_s = (size_t)250e3;

    auto next_timebg = OnTicks(ticks.data(), chunck_s, &lost_ticks);
    EXPECT_EQ(pticks->size(), chunck_s);

    // get allowed overlapping of 1ms
    // get idx first tick with time >= next_timebg
    auto next_idx = MqltickTimeGtEqIdx(ticks, next_timebg);
    auto overlap = chunck_s - next_idx;
    next_timebg = OnTicks(ticks.data() + next_idx, chunck_s + overlap, &lost_ticks);
    EXPECT_EQ(pticks->size(), chunck_s * 2);

    // get allowed overlapping of 1ms
    next_idx = MqltickTimeGtEqIdx(ticks, next_timebg);
    overlap = 2 * chunck_s - next_idx;
    next_timebg = OnTicks(ticks.data() + next_idx, chunck_s + overlap, &lost_ticks);
    EXPECT_EQ(pticks->size(), chunck_s * 3);

    // get allowed overlapping of 1ms
    next_idx = MqltickTimeGtEqIdx(ticks, next_timebg);
    overlap = 3 * chunck_s - next_idx;
    next_timebg = OnTicks(ticks.data() + next_idx, chunck_s + overlap, &lost_ticks);

    EXPECT_EQ(pticks->size(), chunck_s * 4);

}

// TODO
// re-write since 
// tick losses are acceptable
TEST(MoneyBars, OnTicksMbReturns) {
    std::fstream fh;
    std::streampos begin, end;

    char symbol[6] = "PETR4"; // is null terminated by default - char* string literal C++
    double lost_ticks; // qc for real time

    // load SADF indicator
    DataBuffersInit(0.01, 0.01, 25E6, symbol);
    IndicatorsInit(250, 200, 15, false);

    // calculate number of ticks on file
    std::string user_data = std::string(std::getenv("USERPROFILE")) +
        std::string("\\Projects\\stocks\\data\\PETR4_mqltick.bin");

    std::vector<MqlTick> ticks;

    // Read a file and simulate CopyTicksRange
    int64_t nticks = ReadTicks(&ticks, user_data, (size_t)1e6); // 1MM
    BufferMqlTicks* pticks = GetBufferMqlTicks();
    // send in chunck of 250k ticks
    size_t chunck_s = (size_t)250e3;

    auto next_timebg = OnTicks(ticks.data(), chunck_s, &lost_ticks);
    EXPECT_EQ(pticks->size(), chunck_s);

    for (auto it = m_MbReturn->Begin(0); it < m_MbReturn->End(0); it++) {
        std::cout << " " << *it;
        std::cout << std::endl;
    }  


}

TEST(Indicators, CSADF) {

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
    auto iSADF = new CSADF(BUFFERSIZE);

    iSADF->Init(maxw, minw, p, true);
    // send in 50, 70, 30
    // consider that the results are different
    // from it was sent entirely 150 at once
    // small batches have same degrees of freedom
    // but matrices solved on GPU are assemblied differently
    auto idata = data.begin();
    iSADF->Refresh<vec_iterator<float>>(idata, idata + 15); idata += 15;
    iSADF->Refresh<vec_iterator<float>>(idata, idata + 2); idata += 2;
    iSADF->Refresh<vec_iterator<float>>(idata, idata + 53); idata += 53;
    iSADF->Refresh<vec_iterator<float>>(idata, idata + 50); idata += 50;
    iSADF->Refresh<vec_iterator<float>>(idata, idata + 30);
    //sadf(, outsadf.data(), outlag.data(), data.size(), maxw, minw, p, true, 0.1, false);

    // to write assert here
    std::vector<float> pytruth =
        {   -1.21135693E+00, -1.43946329E+00, -1.68232564E+00, -9.06640237E-01,
            -1.22108498E+00, -1.89698401E-01, -1.61136884E+00, -2.07922055E+00,
            -1.47889062E+00, -1.54500897E-01,  3.19911696E-01,  8.78689220E-02,
            -1.47921599E+00, -1.86779454E+00, -6.06307399E-01,  2.57244375E-01,
            -1.14334838E+00, -1.18730249E+00, -1.71506619E+00, -6.47773148E-01,
            -1.04807511E-01, -7.87479853E-01, -1.25817874E+00, -1.49762514E+00,
             8.60342121E-01, -1.47008814E-04, -3.01985088E-01, -6.28367474E-01,
            -1.60387453E+00, -1.60768983E+00, -1.39444175E+00, -1.49552296E+00,
            -2.15359997E+00, -1.33201221E+00, -1.81103167E+00, -2.28677615E+00,
            -2.14815579E+00, -2.44999544E+00, -2.46165201E+00, -1.94116550E+00,
            -2.40810728E+00, -1.33043296E+00, -1.38970916E+00, -1.48910285E+00,
            -1.66342974E+00, -8.67434354E-01, -1.29622790E+00, -1.21606999E+00,
            -5.45470444E-01, -2.69971280E-02,  5.05580571E-01,  1.36449876E-01,
            -1.08418823E+00, -1.49865579E+00, -1.61797853E+00, -2.24710713E+00,
            -2.34719591E+00, -3.07444659E+00, -3.18648781E+00, -3.59555532E+00,
            -4.32738401E+00, -4.40975161E+00, -7.45215460E-01, -2.38555156E-01,
            -1.40379290E+00, -1.58752257E+00, -8.80635844E-01, -5.50424848E-01,
            -1.58852384E-01, -4.03429076E-01, -9.74086686E-01, -2.14892913E+00,
            -2.59347470E-01, -4.26148831E-01, -1.17506539E+00, -1.16422279E+00,
            -1.70821469E+00, -1.02379456E+00, -1.11480813E+00, -1.64664715E+00,
            -1.40974294E+00, -4.96134128E-01,  2.04860811E-01, -3.63199833E-01,
            -8.08995754E-01, -2.49130799E-01, -2.62068776E-01,  4.83203541E-01,
             1.02040844E+00, -7.01105404E-01, -2.63805879E+00, -2.64598113E+00,
            -2.95255525E+00, -4.56142359E+00, -1.17970784E+00, -1.47220169E+00,
            -1.93733325E+00, -2.61218996E+00, -4.28997290E+00, -3.82756132E+00,
            -2.19426689E+00, -9.33211928E-01, -1.63946240E+00, -1.49839818E+00,
            -1.05623747E+00, -3.83544039E-01, -1.58982535E-01, -6.55848188E-01,
            -8.98149304E-01, -2.32552512E+00, -2.64714236E+00, -2.23722131E+00,
            -1.62003934E+00, -4.81141227E-01,  8.62839722E-01, -4.58048141E-01,
            -8.21320403E-01, -1.05867790E+00, -2.55349116E+00, -1.54699690E+00,
            -8.48539009E-01, -1.85618615E+00, -2.15178528E+00, -2.18635148E+00,
            -2.29839341E+00, -2.78491273E+00, -2.10545655E+00, -2.46073525E+00,
            -1.26924919E+00, -1.83844315E+00, -1.63325120E+00, -3.09571763E-01,
            -1.12712250E+00, -1.25302114E+00, -1.27289258E+00, -5.46270441E-01
        };

    std::vector<float> sadf;
    sadf.resize(iSADF->vCount());
    sadf.assign(iSADF->vBegin(0), iSADF->End(0)); // first buffer is sadf values

    // due model aproximation zeroing rows there is an error
    float distx = 0; // 1.22525799 - real distance th.dist
    for (size_t idx = 0; idx < sadf.size(); ++idx)
        distx += sqrt(pow(pytruth[idx] - sadf[idx], 2));
    distx = sqrt(distx);
    // avg distance (CPU) 0.00xxxxxxxxxxxxxx ~ avg. error <0.9%
    // avg distance (GPU) 0.0090092499264706 ~ avg. error 0.9%
    EXPECT_FLOATS_AVG_DIST_NEARLY(pytruth, sadf.begin(), iSADF->vCount(), 0.01);
}


// Pytho code for test bellow
//import numpy
//
//@jit(nopython=True)
//def cum_sum_entries(y, cum_reset, valid):
//    cumsum = np.zeros(len(y)-1)
//    sumup = 0.
//    sumdw = 0.    
//    for i in range(len(y)-1):        
//        if valid[i+1] and valid[i]:            
//            if np.isnan(y[i]) or np.isnan(y[i+1]):
//                cumsum[i] = 0 # set to np.nan to make it like cpp code
//            else:
//                diff = y[i+1]-y[i]
//                sumup = max(0, sumup + diff)
//                sumdw = min(0, sumdw + diff)
//                if sumup > cum_reset:
//                    sumup = 0
//                    cumsum[i] = 1
//                elif sumdw < -cum_reset:
//                    sumdw = 0
//                    cumsum[i]= -1
//                else:
//                    cumsum[i] = 0
//        else:
//            cumsum[i] = 0
//            sumup = 0
//            sumdw = 0
//    return cumsum

TEST(Indicators, CCumSumPair) {

    std::vector<float> data = { 1.29018739e-01, -3.72868106e-02,  8.03665072e-02,  1.13388896e-01,
        2.11405158e-01, -1.87389302e+00, -1.49298251e+00, -3.67588133e-01,
        1.85023010e-01,  1.37755489e+00,  1.94471419e+00,  2.02779365e+00,
        7.79438198e-01,  8.75312984e-01, -8.91449675e-02, -7.51767457e-01,
       -2.68092918e+00, -1.24713015e-02, -8.06056380e-01,  1.11172998e+00,
        1.50093794e+00,  1.06444263e+00,  6.12410843e-01, -4.11053836e-01,
       -1.16459882e+00, -9.15333807e-01, -1.59840739e+00, -1.95039451e+00,
       -1.77875793e+00, -1.48031354e+00, -1.43630040e+00, -1.08606124e+00,
       -1.35706651e+00, -1.29983497e+00, -1.34431458e+00, -1.09004354e+00,
       -2.36512214e-01, -8.22312891e-01, -1.58157325e+00, -9.23010170e-01,
       -1.04412234e+00, -1.25493813e+00, -1.41527200e+00, -9.57051098e-01,
       -9.96711016e-01, -1.90293401e-01, -2.74390996e-01, -5.56135952e-01,
       -8.43288779e-01, -1.51556408e+00, -8.95729721e-01, -1.02896595e+00,
       -1.08858490e+00, -5.15372515e-01,  2.27763373e-02,  1.25832170e-01,
        3.26305062e-01, -3.77074569e-01, -4.83075708e-01,  1.08749524e-01,
        2.94588685e-01,  1.06928301e+00,  1.70731950e+00,  1.29427803e+00,
        2.49867584e-03,  4.28516775e-01, -5.38754463e-01, -9.09344018e-01,
       -1.67661929e+00, -1.83686483e+00, -2.01971841e+00, -2.11905456e+00,
       -1.26595163e+00, -8.61423373e-01, -1.05872786e+00, -9.03169811e-01,
       -5.58466256e-01, -1.28801656e+00, -1.26653564e+00, -7.97035813e-01,
       -1.05585408e+00,  2.44350776e-01,  9.81386125e-01,  5.81563473e-01,
        4.13042367e-01,  1.43541145e+00,  1.77149022e+00,  1.82967317e+00,
        2.26690030e+00,  4.83456701e-01, -1.44588661e+00, -9.57962990e-01,
       -1.20314801e+00, -9.91502404e-01, -1.55784547e+00, -2.15295100e+00,
       -2.38276124e+00, -1.00244188e+00, -6.05904102e-01,  6.41168980e-03,
        2.03695863e-01,  4.15354103e-01, -3.80860180e-01, -7.26542890e-01,
       -8.11517417e-01, -1.23697770e+00, -6.92515969e-01, -7.93053448e-01,
       -1.41065717e-01, -5.08833416e-02,  4.70857143e-01,  4.71359283e-01,
       -1.10454559e+00, -1.36302245e+00, -1.26677775e+00, -1.39578390e+00,
       -1.35328066e+00, -9.65200126e-01, -2.13306904e+00, -2.48041272e+00,
       -2.66801286e+00, -2.98874855e+00, -2.97862577e+00, -2.77002382e+00,
       -2.92133570e+00, -3.24340367e+00, -3.59914637e+00, -2.41213465e+00,
       -1.13178456e+00, -1.09764194e+00, -1.41129303e+00, -1.51124787e+00,
       -1.39209521e+00, -1.41107309e+00, -1.35771084e+00, -9.28762376e-01};

    std::vector<float> pytruth = { FLT_NAN, 0.,  0.,  0.,  0., -1.,  0.,  1.,  0.,  1.,  0.,  0.,  0.,  0.,
       -1.,  0., -1.,  1.,  0.,  1.,  0.,  0.,  0., -1.,  0.,  0.,  0.,
       -1.,  0.,  0.,  0.,  0.,  0.,  0.,  0.,  0.,  1.,  0.,  0.,  0.,
        0.,  0.,  0.,  0.,  0.,  0.,  0.,  0.,  0.,  0.,  0.,  0.,  0.,
        0.,  1.,  0.,  0.,  0.,  0.,  0.,  0.,  0.,  0.,  0.,  0.,  0.,
        0.,  0.,  0.,  0.,  0.,  0.,  0.,  0.,  0.,  0.,  0.,  0.,  0.,
        0.,  0.,  0.,  0.,  0.,  0.,  0.,  0.,  0.,  0.,  0.,  0.,  0.,
        0.,  0.,  0.,  0.,  0.,  0.,  1.,  0.,  0.,  0.,  0.,  0.,  0.,
       -1.,  0.,  0.,  0.,  0.,  1.,  0., -1.,  0.,  0.,  0.,  0.,  0.,
        0., -1.,  0.,  0.,  0.,  0.,  0.,  0.,  0.,  0.,  1.,  0.,  0.,
        0.,  0.,  0.,  0.,  0. };

    CCumSumPair iCumSum(BUFFERSIZE);

    std::vector<std::pair<float,int>> pairs(data.size());

    for (int i = 0; i < data.size(); i++) {
        pairs[i].first = data[i];
        pairs[i].second = 1;
    }
    for (int i = 55; i < 95; i++) // no calculation region
        pairs[i].second = 0;

    iCumSum.Init(1.5);

    iCumSum.Refresh<std::vector<std::pair<float,int>>::iterator>(pairs.begin(), pairs.end());

    std::vector<float> cumsum(iCumSum.Begin(0), iCumSum.End(0));
    EXPECT_FLOATS_NEARLY_EQ(float, cumsum, pytruth, 9, 1e-9);

}


TEST(Indicators, CCumSumSADF) {

    // different from above since 
    // the valid region is set on the input data
    // not on the input for the CCumSum
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

    std::vector<float> pytruth = { FLT_NAN, FLT_NAN, FLT_NAN, FLT_NAN, 
        FLT_NAN, FLT_NAN, FLT_NAN, FLT_NAN, FLT_NAN, FLT_NAN, FLT_NAN, 
        FLT_NAN, FLT_NAN, FLT_NAN, FLT_NAN,  
        0.,  0.,  0.,  0., -1.,  0.,  1.,  0.,  1.,  0.,  0., 0.,   0., 
       -1.,  0., -1.,  1.,  0.,  1.,  0.,  0.,  0., -1.,  0.,
        0.,  0., -1.,  0.,  0.,  0.,  0.,  0.,  0.,  0.,  0.,  1.,  0.,
        0.,  0.,  0.,  0.,  0.,  0.,  0.,  0.,  0.,  0.,  0.,  0.,  0.,
        0.,  0.,  0.,  0.,  0.,  0.,  0.,  0.,  0.,  0.,  0.,  0.,  0.,
        0.,  0.,  0.,  0.,  0.,  0.,  0.,  0.,  0.,  0.,  0.,  0.,  0.,
        0.,  0.,  0.,  0.,  0.,  0.,  0.,  0.,  0.,  1.,  0.,  0., -1.,
       -1.,  0.,  0.,  0.,  0.,  0.,  0.,  0.,  1.,  0.,  0.,  0.,  0.,
        0.,  0., -1.,  0.,  0.,  0.,  0.,  1.,  0., -1.,  0.,  0.,  0.,
        0.,  0.,  0., -1.,  0.,  0.,  0.,  0.,  0.,  0.,  0.,  0.,  1.,
        0.,  0.,  0.,  0.,  0.,  0.,  0. };

    MoneyBarBuffer mbars(BUFFERSIZE);
    CSADF iSADF(BUFFERSIZE);
    CCumSumSADFPair iCumSum(BUFFERSIZE);
    int maxw = 15;
    int minw = 12;
    int p = 3;
    
    auto sadf_refresh = [pSADF = &iSADF, pbars = &mbars]() {
        auto bar_wprices = vecMoneyBarBufferLast<double, float>(&MoneyBar::wprice, pbars);
        // altough nice and cool, If I need multiple members so... I loop is imperative
        pSADF->Refresh<vec_iterator<float>>(bar_wprices.begin(), bar_wprices.end());
    };

    std::vector<MoneyBar> fake_bars(data.size());
    
    for (int i = 0; i < data.size(); i++) {
        fake_bars[i].wprice = data[i];
        fake_bars[i].inday = 1;
    }
    for (int i = 55; i < 95; i++) // no calculation region
        fake_bars[i].inday = 0;

    iSADF.Init(maxw, minw, p, false);       
    mbars.AddOnNewBars(sadf_refresh);
    // automatically refreshed when m_SADFi is refreshed
    iCumSum.Init(1.5, &(iSADF), &(mbars));
    mbars.AddBars<vec_iterator<MoneyBar>>(fake_bars.begin(), fake_bars.begin()+50);
    mbars.AddBars<vec_iterator<MoneyBar>>(fake_bars.begin()+50, fake_bars.end());

    std::vector<float> cumsum(iCumSum.Begin(0), iCumSum.End(0));
    EXPECT_FLOATS_NEARLY_EQ(float, cumsum, pytruth, 9, 1e-9);

}



TEST(StdBarBuffer, AddTicks) {
    buffer<MqlTick> ticks; // toy data
    std::vector<double> last = { 100, 120, 110, 100, 130, 120, 80, 100, 90};
#define SECS12H (unixtime) 12*3600
    std::vector<unixtime> time = { 0, 30, 55, 60, 180, 200, SECS12H, SECS12H + 60, SECS12H + 70};

    MqlTick tick;
    for (auto i = 0; i<9; i++) {
        tick.time = time[i];
        tick.last = last[i];
        tick.volume = 10; // must be non-zero
        ticks.push_back(tick);
    }

    StdBarBuffer bars(BUFFERSIZE);

    bars.AddTicks(ticks.begin(), ticks.end());
    bars.AddLast(); // close the last bar - like sending a tick w. time greater >>

    // TODO finish assert of test
    // use a tupple instead of StdBar?
    // equallity builtin implemented 
    //for (auto it = bars.begin(); it < bars.end(); it++) {
    //    std::cout << " " << (*it).high;
    //    std::cout << " " << (*it).low;
    //    std::cout << " " << (*it).time;
    //    std::cout << std::endl;
    //}
    // correct expected
    // expected output 
    //  H   L   BARTIME
    // 120 100       0
    // 100 100      60
    // 130 120     180
    //  80  80   43200
    // 100  90   43260
    
}


// need to use DLL Load to Deal with
// more reallistic enviorment of mt5

TEST(Labelling, LabelEvents) {
    std::fstream fh;
    std::streampos begin, end;

    char symbol[6] = "PETR4"; // is null terminated by default - char* string literal C++
    double lost_ticks; // qc for real time

    // load SADF indicator
    DataBuffersInit(0.01, 0.01, 25E6, symbol);
    IndicatorsInit(250, 200, 15, false);

    // calculate number of ticks on file
    std::string user_data = std::string(std::getenv("USERPROFILE")) +
        std::string("\\Projects\\stocks\\data\\PETR4_mqltick.bin");

    std::vector<MqlTick> ticks;

    // Read a file and simulate CopyTicksRange
    int64_t nticks = ReadTicks(&ticks, user_data, (size_t)1e6); // 1MM
    BufferMqlTicks* pticks = GetBufferMqlTicks();
    // send in chunck of 250k ticks
    size_t chunck_s = (size_t)550e3;

    auto next_timebg = OnTicks(ticks.data(), chunck_s, &lost_ticks);
    EXPECT_EQ(pticks->size(), chunck_s);

    LabelEvents(*m_Events, 10., 10, 2, 50, 5);

}