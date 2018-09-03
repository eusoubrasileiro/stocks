#define EXPERT_MAGIC 777707777  // MagicNumber of the expert
#define TESTINGW_FILE false // testin with predictions files

// symbols neeeded for training and prediction on PETR4
string symbols[9] = {"BBDC4", "DOL$", "VALE3", "BBAS3", "PETR4",  "ABEV3", "B3SA3", "ITUB4", "WIN@"};

// stores a prediction time and direction
struct prediction {
    datetime time; // time to apply prediction
    int direction; // 1 or -1 : {buy or sell}
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
const double minprofit=300;
// control of number of orders per ndt (minutes)
// 3 orders maximum allowed on the last 15 minutes
const int norders=3;
// per dt in minutes
const int perdt=15;
// maximum orders per day
const int maxorders=12;