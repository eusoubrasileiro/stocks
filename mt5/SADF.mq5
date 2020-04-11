#property copyright "Andre"
#property description "Supremum Augmented Dickey Fuller Test - limited backward window"

#import "pytorchcpp.dll"
int sadfd_mt5(const double &signal[], double &out[], int n, int maxw, int minw, int p, double gpumem_gb, bool verbose);
#import

//--- indicator settings
//#property indicator_chart_window
#property indicator_separate_window
#property indicator_buffers 2
#property indicator_plots   2
#property indicator_type1   DRAW_ARROW
#property indicator_type2   DRAW_LINE
#property indicator_color1  clrBlue
#property indicator_color2  clrLightGray
#property indicator_width1  1

//--- input parameters
input int            InpMaxWin=2*90;         // Maximum backward window (bars)
input int            InpMinWin=2*60;         // Minimum backward window (bars)
input int            InpMaxLagP=15;          // Order of AR model

//--- indicator buffers
double               SadfLineBuffer[];
double               SadfHistBuffer[];
// SADF output array
double out_sadf[];
double in_data[];

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
   int evalues; // number of EMPTY_VALUE's forward filled
   
   int nvalues = rates_total-prev_calculated-begin;
   
//--- first calculation or number of bars was changed
   if(prev_calculated==0){// first calculation
      start=begin+InpMaxWin; // first sample will be filled on indicator buffer
      //--- set empty value for first bars needed but that will be empty
      for(i=0;i<start;i++){
        SadfLineBuffer[i]=0.0;      
        SadfHistBuffer[i]=0.0; 
      }
      // input data for SADF
      ArrayCopy(in_data, price, 0, begin);      
      // returns number of EMPTY_VALUES forward filled
      evalues = sadfd_mt5(in_data, out_sadf, rates_total-begin, InpMaxWin, InpMinWin, InpMaxLagP, 2.0, false);
   }
   else{   
      if(nvalues > 0){ //at least one new sample compared to previous call
          start = prev_calculated;
          ArrayCopy(in_data, price, 0, (prev_calculated-InpMaxWin));
          evalues = sadfd_mt5(in_data, out_sadf, rates_total-prev_calculated+InpMaxWin, InpMaxWin, InpMinWin, InpMaxLagP, 2.0, false);
      }
      else{
          // recalculate last 1 point
          // still on same bar
          // must be recalculating while a new bar is formed, so we can see it changing
          start = prev_calculated-1;
          ArrayCopy(in_data, price, 0, (prev_calculated-1-InpMaxWin));
          evalues = sadfd_mt5(in_data, out_sadf, InpMaxWin, InpMaxWin, InpMinWin, InpMaxLagP, 2.0, false);          
      }
   }
   
    // place indicator buffer values
    for(i=start; i<rates_total && !IsStopped(); i++){
        SadfLineBuffer[i]=out_sadf[i-start];
        SadfHistBuffer[i]=out_sadf[i-start];
    }
//---
    //Print("Total bars: ", ArraySize(price), " Empty filled ", evalues);
    
  }

//+------------------------------------------------------------------+
//| Custom indicator initialization function                         |
//+------------------------------------------------------------------+
void OnInit()
  {
//--- indicator buffers mapping
   SetIndexBuffer(0,SadfLineBuffer,INDICATOR_DATA);
//--- indicator buffers mapping
   SetIndexBuffer(1,SadfHistBuffer,INDICATOR_DATA);
//--- set accuracy
   IndicatorSetInteger(INDICATOR_DIGITS,_Digits+3);
//--- sets first bar from what index will be drawn
   PlotIndexSetInteger(1,PLOT_DRAW_BEGIN,InpMaxWin);
   PlotIndexSetInteger(0,PLOT_ARROW,159);
//---- line shifts when drawing
   PlotIndexSetInteger(1,PLOT_LINE_WIDTH,1);
 
   
//--- name for DataWindow
   string short_name="SADF";

   IndicatorSetString(INDICATOR_SHORTNAME,short_name+"("+string(InpMaxWin)+")");

//---- initialization done

   ArrayResize(out_sadf, 100000); // huge size just to make sure is enough
   ArrayResize(in_data, 100000);
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
   if(prev_calculated==0){
      ArrayInitialize(SadfLineBuffer,0);
      ArrayInitialize(SadfHistBuffer,0);
   }
//--- sets first bar from what index will be draw 
   PlotIndexSetInteger(1,PLOT_DRAW_BEGIN,InpMaxWin-1+begin);   
   //--- calculation
   CalculateSADF(rates_total,prev_calculated,begin,price);
//--- return value of prev_calculated for next call
   return(rates_total);
  }
//+------------------------------------------------------------------+
