#property copyright "Andre"
#property description "Supremum Augmented Dickey Fuller Test - limited backward window"

#import "pytorchcpp.dll"
int sadfd_mt5(const double &signal[], double &outsadf[], double &idxadf[], int n, int maxw, int minw, int order, bool drift, double gpumem_gb, bool verbose);
#import


#include <Object.mqh>

//--- indicator settings
//#property indicator_chart_window
#property indicator_separate_window
#property indicator_buffers 3
#property indicator_plots   2 // 2 plots, arrow and line
#property indicator_type1   DRAW_COLOR_ARROW
#property indicator_color1  clrBlue, clrGreen, clrYellow, clrOrange, clrRed // 0-4 Shorter or Wider ADF window with max value
#property indicator_type2   DRAW_LINE
#property indicator_color2  clrLightGray
#property indicator_width2  1
#property indicator_applied_price PRICE_MEDIAN

//--- input parameters
input int            InpMaxWin=4*90;         // Maximum backward window (bars)
input int            InpMinWin=2*60;         // Minimum backward window (bars)
input int            InpArOrder=15;          // Order of AR model
input bool           InpModelDrift=false;     // Include Drift Term on AR model
input int            InpMaxBars=60*7;      // Max M1 Bars (1 week)

//--- indicator buffers
double               SadfLineBuffer[];
double               SadfArrowBuffer[];
double               SadfColorArrowBuffer[];

// SADF output array
double out_sadf[];
double out_idxadf[];
double in_data[];

int subwindow_index;
int prev_calc;

string idwindow_short_name = "SADF" + "("+string(InpMaxWin)+"/"+string(InpMinWin)+")";
string label = idwindow_short_name+"_lbl1";
string vline = idwindow_short_name+"_vline1";

double last_maxadfidx;

datetime before;
int adfs_count; // number of adfs per SADF(t) point

//rates_total
//[in]  Size of the price[] array or input series available to the indicator for calculation.
//In the second function type, the parameter value corresponds to the number of bars on the chart it is launched at.
//prev_calculated
//[in] Contains the value returned by the OnCalculate() function during the previous call.
//It is designed to skip the bars that have not changed since the previous launch of this function.
//begin
//[in]  Index value in the price[] array meaningful data starts from.
//It allows you to skip missing or initial data, for which there are no correct values.
//price[]
//[in]  Array of values for calculations.
//One of the price timeseries or a calculated indicator buffer can be passed as the price[] array.
//Type of data passed for calculation can be defined using the _AppliedTo predefined variable.
void CalculateSADF(int rates_total,int prev_calculated,int begin, const double &price[])
  {
   int i;
   int start; // first sample filled on indicator buffer

   // avoid uncessary re-calculation when a new bar arrives
   if(prev_calculated == 0 && prev_calc !=0){
        prev_calculated = prev_calc;
   }

   int nvalues = rates_total-prev_calculated-begin;
   start = prev_calculated;   
   begin = MathMax(rates_total-InpMaxBars,0);

//--- first calculation
   if(prev_calculated==0){
      start=begin+InpMaxWin; // first sample will be filled on indicator buffer
      //--- set empty value for first bars needed but that will be empty
      for(i=0;i<start;i++){
        SadfLineBuffer[i]=0.0;
        SadfArrowBuffer[i]=0.0;
      }
      // input data for SADF
      ArrayCopy(in_data, price, 0, begin);
      // returns number of EMPTY_VALUES forward filled
      sadfd_mt5(in_data, out_sadf, out_idxadf, rates_total-begin, InpMaxWin, InpMinWin, InpArOrder, InpModelDrift, 2.0, false);
      last_maxadfidx = out_idxadf[rates_total-begin-1];
   }
   else{
      if(nvalues > 0){ //at least one new sample compared to previous call
          start = prev_calculated;
          ArrayCopy(in_data, price, 0, (prev_calculated-InpMaxWin-1));
          sadfd_mt5(in_data, out_sadf, out_idxadf, nvalues+InpMaxWin, InpMaxWin, InpMinWin, InpArOrder, InpModelDrift, 2.0, false);
          last_maxadfidx = out_idxadf[nvalues+InpMaxWin-1];
      }
      // dont recalculate on the fly last one point
      // too many calls at the tick resolution, make GPU unavailable
      // passing to CPU Pytorch
      else{
        //if( TimeCurrent() - before > 15){ // every 15 seconds
        //  //too many calls freezes the tick movement of bars
        //  // needed to control this somehow to avoid freeze and not see last bar evolving
        //  // recalculate last 1 point
        //  // still on same bar
        //  // must be recalculating while a new bar is formed, so we can see it changing
        //     start = prev_calculated-1;
        //     ArrayCopy(in_data, price, 0, (prev_calculated-1-InpMaxWin-1));
        //     sadfd_mt5(in_data, out_sadf, out_idxadf, InpMaxWin, InpMaxWin, InpMinWin, InpArOrder, InpModelDrift, 2.0, false);
        //}
      }
   }

    // place indicator buffer values
    for(i=start; i<rates_total && !IsStopped(); i++){
        SadfLineBuffer[i]=out_sadf[i-start];
        SadfArrowBuffer[i]=out_sadf[i-start];
        SadfColorArrowBuffer[i] = (int) ( 4*((double)out_idxadf[i-start]/adfs_count) ); // 5 -> colors [0, 4] 
    }
//---
    //Print("Total bars: ", ArraySize(price), " Empty filled ", evalues);
    prev_calc = rates_total;

    double window_adfmax_length_M1 = last_maxadfidx+InpMinWin;
    ObjectSetString(0, label, OBJPROP_TEXT, StringFormat("Last ADF max (hours): %.2f", window_adfmax_length_M1/60.));
    // vline for current last value
    ObjectSetInteger(0, vline, OBJPROP_TIME,TimeCurrent()-window_adfmax_length_M1*60);
  }

//+------------------------------------------------------------------+
//| Custom indicator initialization function                         |
//+------------------------------------------------------------------+
void OnInit(){
    
    //--- indicator buffers mapping
    // COLOR_ARROW
    SetIndexBuffer(0,SadfArrowBuffer,INDICATOR_DATA);
    SetIndexBuffer(1,SadfColorArrowBuffer,INDICATOR_COLOR_INDEX);
    // LINE
    //--- indicator buffers mapping
    SetIndexBuffer(2,SadfLineBuffer,INDICATOR_DATA);
    
    //--- set accuracy
    IndicatorSetInteger(INDICATOR_DIGITS,_Digits+3);
    //--- sets first bar from what index will be drawn
    PlotIndexSetInteger(2,PLOT_DRAW_BEGIN,InpMaxWin);
    PlotIndexSetInteger(0,PLOT_ARROW,159);
    //---- line shifts when drawing
    PlotIndexSetInteger(2,PLOT_LINE_WIDTH,1);
    
    //--- name for DataWindow
    string short_name="SADF";
    
    IndicatorSetString(INDICATOR_SHORTNAME, idwindow_short_name);
    
    //---- initialization done
    ArrayResize(out_sadf, 100000); // huge size just to make sure is enough
    ArrayResize(out_idxadf, 100000);
    ArrayResize(in_data, 100000);
    
    prev_calc = 0; // real prev calculated
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
    ObjectSetInteger(0, label,OBJPROP_COLOR,clrPink);
    ObjectSetInteger(0, vline,OBJPROP_COLOR,clrPink);
    //--- set line display style
    ObjectSetInteger(0, vline,OBJPROP_STYLE,STYLE_SOLID);
    //--- set line width
    ObjectSetInteger(0, vline,OBJPROP_WIDTH, 1 );
    //--- display in the foreground (false) or background (true)
    ObjectSetInteger(0, vline,OBJPROP_BACK, true);
    
    adfs_count = InpMaxWin - InpMinWin;
}

int OnCalculate(const int rates_total,
                const int prev_calculated,
                const int begin,
                const double &price[])
  {
//--- check for bars count
   if(rates_total-begin<InpMaxWin)
      return(0);// not enough bars for calculation
//--- first calculation or number of bars was changed
   if(prev_calculated==0 && prev_calc ==0){
      ArrayInitialize(SadfLineBuffer,0);
      ArrayInitialize(SadfArrowBuffer,0);
      ArrayInitialize(SadfColorArrowBuffer,0);
   }
//--- sets first bar from what index will be draw
   PlotIndexSetInteger(2,PLOT_DRAW_BEGIN,InpMaxWin-1+begin);
   //--- calculation
   CalculateSADF(rates_total,prev_calculated,begin,price);
//--- return value of prev_calculated for next call
     
   return(rates_total);
  }
  
//+------------------------------------------------------------------+
void OnDeinit(const int reason)
{
    ObjectDelete(0, label);
    ObjectDelete(0, vline);
}