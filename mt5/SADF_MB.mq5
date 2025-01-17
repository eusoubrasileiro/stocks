#property copyright "Andre"
#property description "Supremum Augmented Dickey Fuller Test - limited backward window"

#import "mt5indicators.dll"
int CppSADFMoneyBars(double& mt5_SADFline[], double& mt5_SADFdots[], double& mt5_imaxadfcolor[],double &mt5_imaxadflast, int mt5ptsize);
void CppGetSADFWindows(int &minwin, int &maxwin);
#import


#include <Object.mqh>

//--- indicator settings
#property indicator_separate_window
#property indicator_buffers 3
#property indicator_plots   2
#property indicator_type1   DRAW_COLOR_ARROW
#property indicator_color1  clrBlue, clrGreen, clrYellow, clrOrange, clrRed // 0-4 Shorter or Wider ADF window with max value
#property indicator_type2   DRAW_LINE
#property indicator_color2  clrLightGray
#property indicator_width2  1


//--- indicator buffers
double               SadfLineBuffer[];
double               SadfArrowBuffer[];
double               SadfColorArrowBuffer[];

int subwindow_index;

string idwindow_short_name;
string label;
string vline;

int SADFminWin, SADFmaxWin;


//+------------------------------------------------------------------+
//| Custom indicator initialization function                         |
//+------------------------------------------------------------------+
void OnInit(){


  CppGetSADFWindows(SADFminWin, SADFmaxWin);
  idwindow_short_name  = "SADF_MB"+ "("+string(SADFmaxWin)+"/"+string(SADFminWin)+")";
  label = idwindow_short_name+"_lbl1";
  vline = idwindow_short_name+"_vline1";

  //--- indicator buffers mapping
  SetIndexBuffer(2,SadfLineBuffer,INDICATOR_DATA);
  //--- indicator buffers mapping
  SetIndexBuffer(0,SadfArrowBuffer,INDICATOR_DATA);
  SetIndexBuffer(1,SadfColorArrowBuffer,INDICATOR_COLOR_INDEX);
  //--- set accuracy
  IndicatorSetInteger(INDICATOR_DIGITS,_Digits+3);
  //--- sets first bar from what index will be drawn
  PlotIndexSetInteger(2,PLOT_DRAW_BEGIN,SADFmaxWin);
  PlotIndexSetInteger(0,PLOT_ARROW,159);
  //---- line shifts when drawing
  PlotIndexSetInteger(2,PLOT_LINE_WIDTH,1);

  IndicatorSetString(INDICATOR_SHORTNAME, idwindow_short_name);

  //---- initialization done
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
  ObjectSetInteger(0, label,OBJPROP_COLOR,clrGreenYellow);
  ObjectSetInteger(0, vline,OBJPROP_COLOR,clrGreenYellow);
  //--- set line display style
  ObjectSetInteger(0, vline,OBJPROP_STYLE,STYLE_SOLID);
  //--- set line width
  ObjectSetInteger(0, vline,OBJPROP_WIDTH, 1 );
  //--- display in the foreground (false) or background (true)
  ObjectSetInteger(0, vline,OBJPROP_BACK, true);

  PlotIndexSetDouble(0,PLOT_EMPTY_VALUE, EMPTY_VALUE);
  PlotIndexSetDouble(1,PLOT_EMPTY_VALUE, EMPTY_VALUE);
  PlotIndexSetDouble(2,PLOT_EMPTY_VALUE, EMPTY_VALUE);
}

int OnCalculate(const int rates_total,
                const int prev_calculated,
                const int begin,
                const double &price[]){
  //--- first calculation or number of bars was changed
  if(prev_calculated==0){
    ArrayInitialize(SadfLineBuffer,EMPTY_VALUE);
    ArrayInitialize(SadfArrowBuffer,EMPTY_VALUE);
    ArrayInitialize(SadfColorArrowBuffer,EMPTY_VALUE);
  }
  double last_maxadfidx=0;
  //--- calculation
  int ncalculated = CppSADFMoneyBars(SadfLineBuffer, SadfArrowBuffer, SadfColorArrowBuffer, last_maxadfidx, rates_total);

  double window_adfmax_length = last_maxadfidx+SADFminWin;
  ObjectSetString(0, label, OBJPROP_TEXT, StringFormat("Last ADF_MB max: %.2f", window_adfmax_length));
  // vline for current last value
  ObjectSetInteger(0, vline, OBJPROP_TIME, TimeCurrent()-window_adfmax_length*60);

  return(rates_total);
}

//+------------------------------------------------------------------+
void OnDeinit(const int reason)
{
    ObjectDelete(0, label);
    ObjectDelete(0, vline);
}
