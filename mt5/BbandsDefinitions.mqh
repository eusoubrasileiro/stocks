#define EXPERT_MAGIC 777707777  // MagicNumber of the expert
#define TESTINGW_FILE false // testin with predictions files

// symbols neeeded for training and prediction
string symbols[2] = {"WING19", "WIN@"};
string sname = "WING19"; //"WING19";
int nv = 2; // number of contracts to buy for each direction/quantity
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
const int expire_time=45*60;
//expected variation of price 3:1 for sl, tp
const double expect_var=0.0025;
// desired minprofit
const double minprofit=160;
// control of number of orders per ndt (minutes)
// maximum allowed on the last 15 minutes (perdt)
const int dtnorders=8;
// per dt in minutes
const int perdt=15;
// maximum orders per day
const int maxorders=8;
/////////// Predictions
prediction read_predictions[]; // latest predictions read
// executed predictions per day maxorders*2 (buy and sell)
prediction executed_predictions[16];
int nexec = 0; // count executed_predictions (dont like resize)
//////////////////////////////////
///////// Back-Testing
//////////////////////////////////
prediction test_predictions[];
// index in array of predictions
int ipred = 0;
int npred = 0; // number of test predictions
