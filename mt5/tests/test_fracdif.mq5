#include "..\Util.mqh"
#include "..\indicators\CSpecialIndicators.mqh"
#include "Tests.mqh"

// from Python
double in[100] = {0.23575223, 0.89348613, 0.43196633, 0.86018474, 0.59765737, // a
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
               0.77023781, 0.30574534, 0.74073346, 0.62654889, 0.64183077}; // d

double pyfractruth[91] = {-0.35131172,
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
                0.11830923};

// void test_fracdif(){
//     double pyfcoefs[10] = {-0.00949793, -0.01148944, -0.01427259, -0.01836547, -0.0248182 ,
//                     -0.03607296, -0.059136  , -0.1232    , -0.56      ,  1.       }; // size 10
//     int fsize = 10; // or window size
//     int insize = 100;
//     double eps = 0.0001; // error tolerance in comparison
//     double frac = 0.56; // fractional derivative
//
//     double mql5fcoefs[];
//     FracDifCoefs(frac, fsize, mql5fcoefs);
//
//     if(!almostEqual(mql5fcoefs, pyfcoefs, fsize, eps))
//         Print("Passed - Test FracDifCoefs");
//     else
//         Print("Passed - Test FracDifCoefs");
//
//     double mql5filtered[];
//     int outsize = FracDifApply(in, insize, pyfcoefs, fsize, mql5filtered);
//
//     if((!almostEqual(mql5filtered, pyfractruth, outsize, eps)) || outsize != 91)
//         Print("Failed - Test FracDifApply");
//     else
//         Print("Passed - Test FracDifApply");
// }

void test_fracdf_indicator(){
    double eps = 0.0001; // error tolerance in comparison
    int fsize = 10;
  	CFracDiffIndicator c_fdiff(fsize, 0.56);
  	c_fdiff.SetSize(200); // whatever buffer size just need to be enough

  	// partial calls until total array calculated
  	double a[7];
    // dst, src, dst_idx_srt, src_idx_srt, count_to_copy
    ArrayCopy(a, in, 0, 0, 7);
  	double b[33];
    ArrayCopy(b, in, 0, 7, 33); // 7:40 [left open - no included]
  	double c[11];
    ArrayCopy(c, in, 0, 40, 11); // 40:51
  	double d[49];
    ArrayCopy(d, in, 0, 51, 49); // 51:100 = 49

  	c_fdiff.Refresh(a, 0, 7);
  	c_fdiff.Refresh(b, 0, 33);
  	c_fdiff.Refresh(c, 0, 11);
  	c_fdiff.Refresh(d, 0, 49);

    double pyfractruth_indicator[100]; // prepend EMPTY_VALUES of indicator
    ArrayFill(pyfractruth_indicator, 0, fsize-1, EMPTY_VALUE);
    ArrayCopy(pyfractruth_indicator, pyfractruth, fsize-1);

  	if(!almostEqual(pyfractruth_indicator, c_fdiff.m_data, c_fdiff.Count(), 1e-6))
  	    Print("Failed - Test CFracDiffIndicator");
  	else
  	    Print("Passed - Test CFracDiffIndicator");
}


void OnStart(){
    //test_fracdif();
    test_fracdf_indicator();
}
