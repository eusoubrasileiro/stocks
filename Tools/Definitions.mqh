#define EXPERT_MAGIC 777707777  // MagicNumber of the expert
#define TESTING true

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
// last prediction
prediction plast={0};