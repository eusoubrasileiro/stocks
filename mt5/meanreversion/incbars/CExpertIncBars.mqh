#include "..\..\CExpertMain.mqh"

// Using Dolar/Real Bars and consequently tick data
// only for tick time-frame
// Net Mode Only!
class CExpertIncBars : public CExpertMain
{
protected:
    // Base Data Ticks gathered with CopyTicksRange
    MqlTick m_copied_ticks[]; // fixed size number of ticks copied every x seconds
    int m_ncopied; // last count of ticks copied on buffer
    long m_cmpbegin_time; // unix timestamp in ms begin of next copy

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
    int m_count_positions; // last number of 'positions'
    double m_last_volume; // last volume + (buy) or - (sell)
    double m_volume; // current volume of open or not positions

    // py model re-training control
    //double m_money; // actual money balance
    int m_model_npos; // number of positions executed by this model
    double m_model_accrate; // actual sucess rate of currente python model

  public:

  CExpertIncBars(void);

  void Initialize(int nbands, int bbwindow,
     int batch_size, int ntraining,
     double ordersize, double stoploss, double targetprofit,
     int max_positions);

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
  void DealAdd(ulong  deal);
  
  protected:
  void BuySell(int sign);
  // events
  bool TradeEventPositionOpened();
  bool TradeEventPositionClosed();
  bool TradeEventPositionVolumeChanged();

};
