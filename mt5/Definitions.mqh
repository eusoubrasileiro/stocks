#define EXPERT_MAGIC 777707777  // MagicNumber of the expert
#define TESTINGW_FILE false // testin with predictions files

// symbols neeeded for training and prediction 
string symbols[2] = {"WIN@", "PETR4"};

// stores a prediction time and direction
struct prediction {
    datetime time; // time to apply prediction
    int direction; // positive (buy) or negative (sell) value gives quantity of order
    // for example -3 is sell -3
    // +1 buy 1
};

// array of predictions for testing only
prediction predictions[];
// index in array of predictions
int ipred = 0;
int npred = 0; // number of predictions
// last prediction
prediction plast={0};
//expected variation of price 3:1 for sl, tp
const double expect_var=0.01;
// number of open positions
//int openpositions;
// desired minprofit
const double minprofit=160;
// control of number of orders per ndt (minutes)
// 3 orders maximum allowed on the last 15 minutes
const int norders=20;
// per dt in minutes
const int perdt=15;
// maximum orders per day
const int maxorders=100;
