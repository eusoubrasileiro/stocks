#property copyright "Andre"
#property description "Supremum Augmented Dickey Fuller Test - limited backward window"

#import "indicators.dll"
void CppSADFMoneyBarsInit(int maxwindow, int minwindow, int order, bool usedrift, int maxbars, int numcolors);

int CppSADFMoneyBars(double& mt5_SADFline[], double& mt5_SADFdots[], double& mt5_imaxadfcolor[],double &mt5_imaxadflast, int mt5ptsize);
#import


#include <Object.mqh>

//--- indicator settings
#property indicator_separate_window
#property indicator_buffers 3
#property indicator_plots   2
#property indicator_type1   DRAW_COLOR_ARROW
#property indicator_color1  C'255,255,102',C'157,226,79',C'135,206,250',C'255,189,85',C'255,102,102'
#property indicator_type2   DRAW_LINE
#property indicator_color2  clrLightGray
#property indicator_width2  1

//--- input parameters
input int            InpMaxWin=2*90;         // Maximum backward window (bars)
input int            InpMinWin=2*60;         // Minimum backward window (bars)
input int            InpArOrder=15;          // Order of AR model
input bool           InpModelDrift=false;     // Include Drift Term on AR model
input int            InpMaxBars=60*7*21;      // Max M1 Bars (1 month)

//--- indicator buffers
double               SadfLineBuffer[];
double               SadfArrowBuffer[];
double               SadfColorArrowBuffer[];

//--- An array for storing colors contains 14 elements
color colors[]= // rainbow
  {
    C'255,255,102', // yellow  - closer to current sample
    C'157,226,79', // green
    C'135,206,250', // blue
    C'255,189,85', // orange
    C'255,102,102' // red - farther from current sample
  };

int subwindow_index;

string idwindow_short_name = "SADF_MB" + "("+string(InpMaxWin)+"/"+string(InpMinWin)+")";
string label = idwindow_short_name+"_lbl1";
string vline = idwindow_short_name+"_vline1";

//+------------------------------------------------------------------+
//| Custom indicator initialization function                         |
//+------------------------------------------------------------------+
void OnInit(){
  //--- indicator buffers mapping
  SetIndexBuffer(2,SadfLineBuffer,INDICATOR_DATA);
  //--- indicator buffers mapping
  SetIndexBuffer(0,SadfArrowBuffer,INDICATOR_DATA);
  SetIndexBuffer(1,SadfColorArrowBuffer,INDICATOR_COLOR_INDEX);
  //--- set accuracy
  IndicatorSetInteger(INDICATOR_DIGITS,_Digits+3);
  //--- sets first bar from what index will be drawn
  PlotIndexSetInteger(2,PLOT_DRAW_BEGIN,InpMaxWin);
  PlotIndexSetInteger(0,PLOT_ARROW,159);
  //---- line shifts when drawing
  PlotIndexSetInteger(2,PLOT_LINE_WIDTH,1);

  IndicatorSetString(INDICATOR_SHORTNAME, idwindow_short_name);

  //---- initialization done

  // colors definition by integer mapping
  // index of max adf will go from 0 to InpMaxWin-InpMinWin
  // divide colors uniformily
  int color_span = (InpMaxWin-InpMinWin)/5;
  for(int i=0; i<5; i++){
     PlotIndexSetInteger(0,             //  The number of a graphical style
                  PLOT_LINE_COLOR,      //  Property identifier
                  i,                    //  The index of the color, where we write the color
                  colors[i]);             //  A new color
  }

  subwindow_index = ChartWindowFind();
  // max length of maximum ADF found
  ObjectCreate(0, label, OBJ_LABEL, subwindow_index, 0,0);
  ObjectSetInteger(0, label,OBJPROP_CORNER, 3);
  //--- set X coordinate
  ObjectSetInteger(0, label,OBJPROP_XDISTANCE,230);
  //--- set Y coordinate
  ObjectSetInteger(0, label,OBJPROP_YDISTANCE,10);
  ObjectSetInteger(0, label,OBJPROP_FONTSIZE,10);
  ObjectSetString(0, label,OBJPROP_FONT,"Arial");

  ObjectCreate(0, vline, OBJ_VLINE, subwindow_index, 0, 0);

  ObjectSetInteger(0, vline,OBJPROP_COLOR,clrAntiqueWhite);
  //--- set line display style
  ObjectSetInteger(0, vline,OBJPROP_STYLE,STYLE_DOT);
  //--- set line width
  ObjectSetInteger(0, vline,OBJPROP_WIDTH, 1 );
  //--- display in the foreground (false) or background (true)
  ObjectSetInteger(0, vline,OBJPROP_BACK, true);

  PlotIndexSetDouble(0,PLOT_EMPTY_VALUE, EMPTY_VALUE);
  PlotIndexSetDouble(1,PLOT_EMPTY_VALUE, EMPTY_VALUE);
  PlotIndexSetDouble(2,PLOT_EMPTY_VALUE, EMPTY_VALUE);

  CppSADFMoneyBarsInit(InpMaxWin, InpMinWin, InpArOrder, InpModelDrift, InpMaxBars, 5);
}

int OnCalculate(const int rates_total,
                const int prev_calculated,
                const int begin,
                const double &price[]){
  //--- first calculation or number of bars was changed
  if(prev_calculated==0){
    ArrayInitialize(SadfLineBuffer,0);
    ArrayInitialize(SadfArrowBuffer,0);
    ArrayInitialize(SadfColorArrowBuffer,0);
  }
  double last_maxadfidx;
  //--- calculation
  int ncalculated = CppSADFMoneyBars(SadfLineBuffer, SadfArrowBuffer, SadfColorArrowBuffer, last_maxadfidx, rates_total);

  double window_adfmax_length = last_maxadfidx+InpMinWin;
  ObjectSetString(0, label, OBJPROP_TEXT, StringFormat("Last ADF max (hours): %.2f", window_adfmax_length));
  // vline for current last value
  ObjectSetInteger(0, vline, OBJPROP_TIME, TimeCurrent()-window_adfmax_length);

  return(rates_total);
}
//+------------------------------------------------------------------+
