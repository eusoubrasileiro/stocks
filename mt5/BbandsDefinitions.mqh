#define EXPERT_MAGIC 777707777  // MagicNumber of the expert
//#define BACKTESTING // testing with predictions files

// symbols neeeded for training and prediction
#ifdef BACKTESTING
string symbols[2] = {"PETR4", "WIN@"};
string sname = "WIN@";
#else // real operation
string symbols[2] = {"WING19", "WIN@"};
//string sname = "WING19";
string sname = "WIN@"; // backtesting live
#endif

// stores a prediction time and direction
struct prediction {
    datetime time; // time to apply prediction
    int direction; // positive (buy) or negative (sell) value gives quantity of order
    // for example -3 is sell -3
    // +1 buy 1
};
//////////////////////////////////
///////// Real-Time Operation
//////////////////////////////////
// number of minute bars needed by the python code
const int ntrainingbars = 5000;
// time to expire a position (close it)
const int expiretime=46*60;
// number of contracts to buy for each direction/quantity
int quantity = 5;
// tick-size
const double ticksize=5; // minicontratos ibovespa 5 points price variation
// deviation accept by price
const double deviation=3;
// control of number of deals per ndt (minutes)
// maximum allowed on the last 15 minutes (perdt)
const int dtndeals=8;
// per dt in minutes
const int perdt=15;
// maximum deals per day
const int maxdealsday=12;
// number of deals counter
int ndeals=0;
// expert operations begin
const int starthour=9;
const int startminute=30;
// expert operations end (no further sells or buys)
const int endhour=17;
const int endminute=30;
// tolerance in minutes for an order readed and its execution (seconds)
const int exectolerance = 3*60;
/////////// Predictions
prediction read_predictions[]; // latest predictions read from file (python created)
// executed/sent predictions per day, not sure how many python create, 10k will sufice certainly
prediction sent_predictions[10000];
int nsent = 0; // count executed_predictions (dont like resize)
// handle for Stdev indicator
int    hstdev=0;
const int  windowstdev=60; // last hour
double laststdev=5*ticksize;  // default value in case of shit
const double trailing_value = 1*ticksize; // independent stoploss for trailing
// handle for EMA of trailing stop smooth
// the price variation on this EMA controls to change on the trailling stop
int    hema=0;
const int  windowema=5; // 10 minutes
double lastema=1; // last value of the EMA 1 minute
double lastemachange=0; // last EMA value when there was a change on stop loss positive
//////////////////////////////////
///////// Back-Testing
//////////////////////////////////
const uint npredictionfile=399;
prediction test_predictions[];
// index in array of predictions
int ipred = 0;
int npred = 0; // number of test predictions
