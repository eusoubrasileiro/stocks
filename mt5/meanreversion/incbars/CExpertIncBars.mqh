#include "..\..\CExpertMain.mqh"

// number of samples needed/used for training 5*days?
const int                Expert_BufferSize      = 100e3; // indicators buffer needed
const int                Expert_MaxFeatures     = 100; // max features allowed - not used
const double             Expert_MoneyBar_Size   = 100e3; // R$ to form 1 money bar
const double             Expert_Fracdif         = 0.6; // fraction for fractional difference
const double             Expert_Fracdif_Window  = 512; // window size fraction fracdif
const int                Expert_Max_Tick_Copy   = 10e3; // max ticks copied at every CopyTicksRange call

// Using Dolar/Real Bars and consequently tick data
// only for tick time-frame
// Net Mode Only!
class CExpertIncBars : public CExpertMain
{
protected:
#ifdef _DEBUG
    int file_io_hnd;
#endif
    // Base Data Ticks gathered with CopyTicksRange
    MqlTick m_copied_ticks[]; // fixed size number of ticks copied every x seconds
    int m_ncopied; // last count of ticks copied on buffer
    long m_cmpbegin_time; // unix timestamp in ms begin of next copy

    // upper and down and middle
    int m_nbands; // number of bands
    int m_bbwindow; // reference size of bollinger band indicator others
    double m_bbwindow_inc;   // are multiples of m_bbwindow_inc

    // execution of positions and orders
    // profit or stop loss calculation for training
    double m_ordersize;
    double m_stoploss; // stop loss value in $$$ per order
    double m_targetprofit; //targetprofit in $$$ per order
    // max number of orders
    // position being a execution of one order or increment
    // on the same direction
    // number of maximum 'positions'
    int m_max_positions; // maximum number of 'positions'
    int m_last_positions; // last number of 'positions'
    double m_last_volume; // last volume + (buy) or - (sell)
    double m_volume; // current volume of open or not positions

  public:

  CExpertIncBars(void);
  ~CExpertIncBars(); // dont care for now memory leaks on expert execution

  void Initialize(int nbands, int bbwindow,
     int batch_size, int ntraining,
     double ordersize, double stoploss, double targetprofit,
     double run_stoploss, double run_targetprofit, bool recursive,
     int max_positions=-1);

  void Deinit(void);
  // start index and count of new bars that just arrived
  bool Refresh();

  // will be called every < 1 second
  // OnTick + OnTimer garantee a better refresh rate
  // than 1 second
  // Since OnTick not enquees calls
  // OnTick is not called again if the first OnTick
  // has not being processed yet
  void CheckTicks(void);
  virtual void verifyEntry();

  protected:
  void BuySell(int sign);
  // events
  bool TradeEventPositionOpened();
  bool TradeEventPositionClosed();
  bool TradeEventPositionVolumeChanged();
};
