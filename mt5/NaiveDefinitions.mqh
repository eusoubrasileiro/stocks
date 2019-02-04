#define EXPERT_MAGIC 123456789  // MagicNumber of the expert

string isname = "WING19"; // symbol for orders
string sname = "WIN@"; // symbol for indicators

// time to expire a position (close it)
const int expiretime=2*60+30;
// number of contracts to buy for each direction/quantity
int quantity = 5;
// tick-size
const double ticksize=5; // minicontratos ibovespa 5 points price variation
// deviation accept by price
const double deviation=3;
// control of number of deals per ndt (minutes)
// maximum allowed on the last 15 minutes (perdt)
const int dtndeals=1;
// per dt in minutes
const int perdt=35;
// maximum deals per day
const int maxdealsday=6;
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
// handle for Stdev indicators
int    hstdevh=0;
int    hstdevl=0;
// handle for High and Low emas
int    hemah=0;
int    hemal=0;
