#property copyright "Andre"
#property description "MoneyBars"

#import "datastruct.dll"
int CppMoneyBarMt5Indicator(double &mt5_ptO[], double &mt5_ptH[], double &mt5_ptL[], double &mt5_ptC[],
        double &mt5_ptM[], datetime &mt5_ptE[], double &mt5_Color[], int mt5ptsize);

datetime CppOnTicks(MqlTick &mt5_pticks[], int mt5_nticks, double &ticks_lost);

void CppDataBuffersInit(double ticksize, double tickvalue,
    double moneybar_size,  // R$ to form 1 money bar
    char& cs_symbol[],  // cs_symbol is a char[] null terminated string (0) value at end
    // SADF part
    bool sadfindicator, // wether to load sadf indicator or not in bakground
    int maxwindow,
    int minwindow,
    int order,
    bool usedrift,
    int maxbars,
    int numcolors);

bool CppNewBars(); // are there any new bars after call of CppOnTicks

#import


//#include <Object.mqh>

//--- indicator settings
//#property indicator_chart_window
//#property indicator_separate_window
#property indicator_buffers 6
#property indicator_plots   2
#property indicator_type1   DRAW_COLOR_CANDLES
#property indicator_type2   DRAW_ARROW
#property indicator_color2  clrRed
#property indicator_width2  1
#property indicator_color1  clrRed,clrOrange,clrGray,clrGreen,clrBlue // 0,4 from bear to bull
#property indicator_width1  3

//--- input parameters
input double         InpMoneyBarSize=400;         // in MM BRL
input int            InpMaxBars=400;      // Max Bars
//--- input parameters
input bool           InpSADF=true;
input int            InpMaxWin=350;         // Maximum backward window (bars)
input int            InpMinWin=150;         // Minimum backward window (bars)
input int            InpArOrder=15;          // Order of AR model
input bool           InpModelDrift=false;     // Include Drift Term on AR model

//--- indicator buffers
double               MoneyBarsOBuffer[];
double               MoneyBarsHBuffer[];
double               MoneyBarsLBuffer[];
double               MoneyBarsCBuffer[];
double               MoneyBarsMBuffer[];
datetime             MoneyBarsETimes[];
double               MoneyBarsColor[];


// doesnt matter mql5 resizes if its necessary
 const int                Expert_Max_Tick_Copy   = 10e3; // max ticks copied at every CopyTicksRange call

int prev_calc;

// Base Data Ticks gathered with CopyTicksRange
MqlTick m_copied_ticks[]; // fixed size number of ticks copied every x seconds
int m_ncopied; // last count of ticks copied on buffer
long m_cmpbegin_time; // unix timestamp in ms begin of next copy
uchar csymbol[100];
double tick_value, tick_size;

//+------------------------------------------------------------------+
//| Custom indicator initialization function                         |
//+------------------------------------------------------------------+
string label;
string label_tlost;
bool cppstart=false;
double ticks_lost=0; // ticks lost between calls of CopyTicksRange unavoidable

void OnInit()
  {
   //EventSetMillisecondTimer(100);
//--- indicator buffers apping
   SetIndexBuffer(0,MoneyBarsOBuffer,INDICATOR_DATA);
   SetIndexBuffer(1,MoneyBarsHBuffer,INDICATOR_DATA);
   SetIndexBuffer(2,MoneyBarsLBuffer,INDICATOR_DATA);
   SetIndexBuffer(3,MoneyBarsCBuffer,INDICATOR_DATA);
   SetIndexBuffer(4,MoneyBarsColor,INDICATOR_DATA);

   PlotIndexSetInteger(5,PLOT_ARROW,158);
   SetIndexBuffer(5,MoneyBarsMBuffer,INDICATOR_DATA);

//--- indicator buffers mapping
//   SetIndexBuffer(0,SadfArrowBuffer,INDICATOR_DATA);
//   SetIndexBuffer(1,SadfColorArrowBuffer,INDICATOR_COLOR_INDEX);
//--- set accuracy
   IndicatorSetInteger(INDICATOR_DIGITS,_Digits+2);
////--- sets first bar from what index will be drawn
//   PlotIndexSetInteger(2,PLOT_DRAW_BEGIN,InpMaxWin);
//   PlotIndexSetInteger(0,PLOT_ARROW,159);
////---- line shifts when drawing
//   PlotIndexSetInteger(2,PLOT_LINE_WIDTH,1);
   string short_name = "MoneyBars("+string((int)InpMoneyBarSize)+")MM BRL";
   IndicatorSetString(INDICATOR_SHORTNAME, short_name);
    // Ticks download control
   ArrayResize(m_copied_ticks, Expert_Max_Tick_Copy);

   string symbolname = Symbol();
   StringToCharArray(symbolname, csymbol,
                      0, WHOLE_ARRAY, CP_ACP); // ANSI

   SymbolInfoDouble(Symbol(),SYMBOL_TRADE_TICK_VALUE, tick_value);
   SymbolInfoDouble(Symbol(),SYMBOL_TRADE_TICK_SIZE, tick_size);

   PlotIndexSetDouble(0,PLOT_EMPTY_VALUE, EMPTY_VALUE);

   label = short_name+"lbl";
   label_tlost = short_name+"lbl"+"lost";
   int subwindow_index = ChartWindowFind();
   // Indicator NAME
   ObjectCreate(0, label, OBJ_LABEL, subwindow_index, 0, 0);
   ObjectSetInteger(0, label,OBJPROP_CORNER, 3);
   //--- set X coordinate
   ObjectSetInteger(0, label,OBJPROP_XDISTANCE,230);
   //--- set Y coordinate
   ObjectSetInteger(0, label,OBJPROP_YDISTANCE,30);
   ObjectSetInteger(0, label,OBJPROP_FONTSIZE,10);
   ObjectSetString(0, label,OBJPROP_FONT,"Arial");
   ObjectSetString(0, label, OBJPROP_TEXT, short_name);
   // Lost ticks
   ObjectCreate(0, label_tlost, OBJ_LABEL, subwindow_index, 0, 0);
   ObjectSetInteger(0, label_tlost,OBJPROP_CORNER, 3);
   //--- set X coordinate
   ObjectSetInteger(0, label_tlost,OBJPROP_XDISTANCE,230);
   //--- set Y coordinate
   ObjectSetInteger(0, label_tlost,OBJPROP_YDISTANCE,50);
   ObjectSetInteger(0, label_tlost,OBJPROP_FONTSIZE,10);
   ObjectSetString(0, label_tlost,OBJPROP_FONT,"Arial");
   ObjectSetString(0, label_tlost, OBJPROP_TEXT, "Ticks Lost: " + string(ticks_lost));
  }


void GetTicks()
{
  // copy all ticks from last copy time - 1 milisecond to now
  // to avoid missing ticks on same ms)
  m_ncopied = CopyTicksRange(Symbol(), m_copied_ticks,
       COPY_TICKS_ALL, m_cmpbegin_time, 0);

  if(m_ncopied < 1){
     int error = GetLastError();
     if(error == ERR_HISTORY_TIMEOUT)
       // ticks synchronization waiting time is up, the function has sent all it had.
       Print("ERR_HISTORY_TIMEOUT");
     else
     if(error == ERR_HISTORY_SMALL_BUFFER)
       //static buffer is too small. Only the amount the array can store has been sent.
       Print("ERR_HISTORY_SMALL_BUFFER");
     else
     if(error == ERR_NOT_ENOUGH_MEMORY)
       // insufficient memory for receiving a history from the specified range to the dynamic
       // tick array. Failed to allocate enough memory for the tick array.
       Print("ERR_NOT_ENOUGH_MEMORY");
     // better threatment here ??...better ignore and wait for next call
     return;
  }

  // call C++ passing ticks
    m_cmpbegin_time = CppOnTicks(m_copied_ticks, m_ncopied, ticks_lost);
    ObjectSetString(0, label_tlost, OBJPROP_TEXT, "Ticks Lost: " + string(ticks_lost));
}

//void OnTimer(){
//    GetTicks();
//}

int OnCalculate(const int rates_total,
                const int prev_calculated,
                const datetime &time[],
                const double &open[],
                const double &high[],
                const double &low[],
                const double &close[],
                const long &tick_volume[],
                const long &volume[],
                const int &spread[])
{

////--- first calculation or number of bars was changed
    PlotIndexSetInteger(0, PLOT_DRAW_BEGIN, 0);
    PlotIndexSetInteger(1, PLOT_DRAW_BEGIN, 0);
    PlotIndexSetInteger(2, PLOT_DRAW_BEGIN, 0);
    PlotIndexSetInteger(3, PLOT_DRAW_BEGIN, 0);
    PlotIndexSetInteger(4, PLOT_DRAW_BEGIN, 0);
    PlotIndexSetInteger(5, PLOT_DRAW_BEGIN, 0);
    // must clean up entire buffer every time
    // since bars are allways moving backward
    ArrayInitialize(MoneyBarsOBuffer,EMPTY_VALUE);
    ArrayInitialize(MoneyBarsHBuffer,EMPTY_VALUE);
    ArrayInitialize(MoneyBarsLBuffer,EMPTY_VALUE);
    ArrayInitialize(MoneyBarsCBuffer,EMPTY_VALUE);
    ArrayInitialize(MoneyBarsMBuffer,EMPTY_VALUE);
    ArrayInitialize(MoneyBarsColor,EMPTY_VALUE);

   if(!cppstart && rates_total > InpMaxBars+1){ // starting
        // first m_cmpbegin_time must be done here others
        // will come from C++
        // time now is in seconds unix timestamp
        m_cmpbegin_time = time[rates_total-InpMaxBars-1];
        CppDataBuffersInit(tick_size, tick_value, InpMoneyBarSize*1E6, csymbol,
            InpSADF, InpMaxWin, InpMinWin, InpArOrder, InpModelDrift, InpMaxBars, 5);
        m_cmpbegin_time*=1000; // to ms next CopyTicksRange call
        m_ncopied = 0;
        Print("Last Bar Open Time: ", time[rates_total-1]);
        cppstart = true;
   }
   GetTicks();
   //--- sets first bar from what index will be draw
   ArrayResize(MoneyBarsETimes, rates_total);
   int totalbars = CppMoneyBarMt5Indicator(MoneyBarsOBuffer,MoneyBarsHBuffer, MoneyBarsLBuffer,
                MoneyBarsCBuffer, MoneyBarsMBuffer, MoneyBarsETimes, MoneyBarsColor, rates_total);
   return(rates_total);
  }
//+------------------------------------------------------------------+


void OnDeinit(const int reason)
{
    cppstart = false;
    ObjectDelete(0, label);
    ObjectDelete(0, label_tlost);
    EventKillTimer();
}
