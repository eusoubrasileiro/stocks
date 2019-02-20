#ifdef _WIN32
#ifndef DEBUG
#define DLL_EXPORT __declspec(dllexport)
#endif
#else
#define DLL_EXPORT
#endif

// orders
#define OK 0  // order Kind 0-sell, 1-buy, 2-change stops pendings : 3-buy stop, 4-sell stop, 5-buy limit, 6-sell limit
#define OP 1  // order price
#define VV 2  // order or position volume or deal volume
#define SL 3  // stop loss maybe -1 not set
#define TP 4  // take profit maybe -1 not set
#define DI 5  // devitation accepted maybe -1 not set
#define TK 6  // ticket position to operate on (decrese-increase volume etc) maybe -1 not set
#define OS 7  // source of order client or some event
// positions
#define PP 8  // position weighted price
#define PT 9  // position time openned - needed by algos closing by time
#define PC 10  // last time changed
// deals
#define DT 11  // deal time
#define DV 12  // deal volume
#define DR 13  // deal result + money back , - money taken or 0 nothing
#define DE 14  // deal entry - in (1) or out(0) or inout reverse (2)

#define Order_Kind_Buy   0
#define Order_Kind_Sell   1
#define Order_Kind_Change   2  // change stop loss
#define Order_Kind_BuyStop   4
#define Order_Kind_SellStop   5
#define Order_Kind_BuyLimit   6
#define Order_Kind_SellLimit   7
#define Order_Source_Client   0  // default
#define Order_Source_Stop_Loss   1
#define Order_Source_Take_Profit   2
#define Deal_Entry_In   0
#define Deal_Entry_Out   1

#define N 15 // number of collumns

DLL_EXPORT void sendOrder(double kind, double price, double volume,
              double sloss, double tprofit, double deviation,
              double ticket, double source);

DLL_EXPORT void Simulator(double init_money, double tick_value, double order_cost,
        double *ticks, int nticks, double *pmoney, void (*onTick)(double*));
        
       
DLL_EXPORT int _iorder = 0; // number of pending orders
DLL_EXPORT double _orders[N*100]; // pending waiting for entry
DLL_EXPORT int _ipos = 0; // number of open positions
DLL_EXPORT double _positions[N*100];
DLL_EXPORT int _ideals = 0; // number of executed deals
DLL_EXPORT double _deals_history[N*10000];
DLL_EXPORT double _money = 0; // money on pouch
DLL_EXPORT double _tick_value = 0.02; // ibov mini-contratc
DLL_EXPORT double _order_cost = 0.0; // order cost in $money




