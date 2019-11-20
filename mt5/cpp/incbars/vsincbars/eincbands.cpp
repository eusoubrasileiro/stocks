#include "eincbands.h"

ExpertIncBands::ExpertIncBands(void) {
    m_bbwindow_inc = 0.5;
    //m_model = new sklearnModel;
    m_xypair_count = 0;
    m_model_last_training = 0;
    m_last_raw_signal_index = -1;
    m_last_positions = 0; // number of 'positions' openned
    m_last_volume = 0;
    m_volume = 0;
}

void ExpertIncBands::Initialize(int nbands, int bbwindow,
    int batch_size, int ntraining,
    double ordersize, double stoploss, double targetprofit,
    double run_stoploss, double run_targetprofit, bool recursive,
    double ticksize, double tickvalue,
    int max_positions)
{
    // create money bars
    m_ticks.SetSize(Expert_BufferSize);
    m_bars = MoneyBarBuffer(tickvalue,
        ticksize, Expert_MoneyBar_Size);
    m_bars.SetSize(Expert_BufferSize);
    m_times.SetSize(Expert_BufferSize);

    m_recursive = recursive;
    m_nbands = nbands;
    m_bbwindow = bbwindow;
    // ordersize is in $$
    m_ordersize = ordersize;
    // for training
    m_stoploss = stoploss; // value in $$$
    m_targetprofit = targetprofit;
    // for execution
    m_run_stoploss = run_stoploss; // value in $$$
    m_run_targetprofit = run_stoploss;
    // training
    m_batch_size = batch_size;
    m_ntraining = ntraining; // minimum number of X, y training pairs
    // total of signal features
    // m_nbands band signals
    // indicar features is 1
    // 1 - fracdiff on prices
    m_nsignal_features = m_nbands + 1;
    m_xtrain_dim = m_nsignal_features * m_batch_size;

    // allocate array of bbands indicators
    m_bands.resize(m_nbands);

    // group of samples and signal vectors to train the model
    // resize buffer of training pairs
    m_xypairs.Resize(m_ntraining);
    // resize x feature vector inside it
    // but they do not exist cant resize something that doesnt exist
    // only the space available for them
    //m_model_refresh = model_refresh;

    m_raw_signal.resize(m_nbands); // one for each band
    m_last_raw_signal.resize(m_nbands);

    CreateBBands();
    CreateOtherFeatureIndicators();
}

void ExpertIncBands::CreateBBands() {
    // raw signal storage
    for (int j = 0; j < m_nbands; j++) {
        m_raw_signal[j] = CCBuffer<int>(Expert_BufferSize);
    }

    // ctalib bbands
    double inc = m_bbwindow_inc;
    for (int i = 0; i < m_nbands; i++) { // create multiple increasing bollinger bands
        m_bands[i] = CTaBBANDS(m_bbwindow * inc, 2.5, 1); // 1-Exponential type
        m_bands[i].SetSize(Expert_BufferSize);
        inc += m_bbwindow_inc;
    }
}

void ExpertIncBands::CreateOtherFeatureIndicators() {
    // only fracdiff on prices
    m_fd_mbarp = CFracDiffIndicator(Expert_Fracdif_Window, Expert_Fracdif);
    m_fd_mbarp.SetSize(Expert_BufferSize);
}

// can be called every < 1 second
void ExpertIncBands::AddTicks(MqlTick *cticks, int size)
{
    if (m_ticks.Refresh(cticks, size) > 0) {

        if (m_bars.AddTicks(m_ticks) > 0)
        {
            // some new ticks arrived and new bars created
            // at least one new money bar created
            // refresh whatever possible with new bars
            if (Refresh()) {
                // sucess of updating everything w. new data
                verifyEntry();
            }
        }
    }
    // Metatrader Part 
    // update indicators for trailing stop
    // check to see if we should close any open position
    // 1. by time 
    // 2. and by trailing stop    
}

// Since all buffers must be alligned
// Same new bars will come for all indicators and else 

int ExpertIncBands::BufferSize() { return m_bars.Size(); }
int ExpertIncBands::BufferTotal() { return m_bars.Count(); }
int ExpertIncBands::BeginNewData() { return m_bars.Count() - m_bars.m_nnew; }

// start index and count of new bars that just arrived
bool ExpertIncBands::Refresh()
{
    m_bars.RefreshArrays(); // update internal latest arrays of times, prices
    m_times.AddRangeTimeMs(m_bars.m_times.data(), 0, m_bars.m_nnew);
    bool result = true;

    // all bellow must be called to maintain alligment of buffers
    // by creating empty samples

    // features indicators
    // fracdif on bar prices order of && matter
    // first is what you want to do and second what you want to try
    result = m_fd_mbarp.Refresh(m_bars.m_last.data(), 0, m_bars.m_nnew) && result;

    // update all bollinger bands indicators
    for (int j = 0; j < m_nbands; j++) {
        result = m_bands[j].Refresh(m_bars.m_last.data(), 0, m_bars.m_nnew) && result;
    }

    // called after m_ticks.Refresh() and Refreshs() above
    // garantes not called twice and bband indicators available
    RefreshRawBandSignals(m_bars.m_last.data(), m_bars.m_nnew, result);

    return result;
}

void ExpertIncBands::RefreshRawBandSignals(double last[], int count, int empty) {
    // should be called only once per refresh
    // use the last added samples
    empty = (empty) ? 1 : 0;
    int start_new = BeginNewData();
    for (int j = 0; j < m_nbands; j++) {
        //    Based on a bollinger band defined by upper-band and lower-band
        //    return signal:
        //        buy   1 : crossing down-outside it's buy
        //        sell -1 : crossing up-outside it's sell
        //        hold  0 : nothing usefull happend
        for (int i = 0; i < count; i++) {
            if (last[i] >= m_bands[j].m_upper[start_new + i])
                m_raw_signal[j].Add(-1 * empty); // sell
            else
                if (last[i] <= m_bands[j].m_down[start_new + i])
                    m_raw_signal[j].Add(+1 * empty); // buy
                else
                    m_raw_signal[j].Add(0); // nothing
        }
    }
}

void ExpertIncBands::verifyEntry() {

    // has an entry signal in any band? on the lastest
    // raw signals added bars
    if (lastRawSignals() != -1)
    {
        int m_xypair_count_old = m_xypairs.Count();
        // only when a new entry signal recalc classes
        // only when needed
        //LabelClasses();
        //CreateXFeatureVectors(m_xypairs);
        // add number of xypairs added (cannot decrease)
        m_xypair_count += (m_xypairs.Count() - m_xypair_count_old);
        // update/train model if needed/possible
        //if (m_xypairs.Count() >= m_ntraining) {
        //    // first time training
        //    if (!m_model.isready) {
        //        // time to train the model for the first time
        //        m_model.isready = PythonTrainModel();
        //    }
        //    //else // time to update the model?
        //    //if((m_xypair_count - m_model_last_training) >= m_model_refresh){
        //    // if diference in training samples to the last training is bigger than
        //    // refresh criteria - time to train again
        //    //  m_model.isready = PythonTrainModel();
        //    //}
        //    // record when last training happened
        //    m_model_last_training = m_xypair_count;
        //}
        //// Call Python if model is already trainned
        //if (m_model.isready) {
        //    // x forecast vector
        //    XyPair Xforecast = new XyPair;
        //    Xforecast.time = m_times[m_last_raw_signal_index];
        //    CreateXFeatureVector(Xforecast);
        //    int y_pred = PythonPredict(Xforecast);
        //    BuySell(y_pred);
        //    if (m_recursive && (y_pred == 0 || y_pred == 1 || y_pred == -1)) {
        //        // modify the band raw signals by the ones predicted
        //        for (int i = 0; i < m_nbands; i++)
        //            // change from -1 to 1 or to 0
        //            m_raw_signal[i].m_data[m_raw_signal[i].Count() - 1] = y_pred;
        //    }
        //}
    }
}


// verify an entry signal in any band
// on the last m_added bars
// get the latest signal
// return index on buffer of existent entry signal
// only if more than one signal
// all in the same direction
// otherwise returns -1
// only call when there's at least one signal
// stored in the buffer of raw signal bands
int ExpertIncBands::lastRawSignals() {
    int direction = 0;
    m_last_raw_signal_index = -1;
    // m_last_raw_signal_index is index based
    // on circular buffers index alligned on all 
    for (int j = 0; j < m_nbands; j++) {
        for (int i = BeginNewData(); i < BufferTotal(); i++) {
            m_last_raw_signal[j] = m_raw_signal[j][i];
            if (m_last_raw_signal[j] != 0) {
                if (direction == 0) {
                    direction = m_last_raw_signal[j];
                    m_last_raw_signal_index = i;
                }
                else
                    // two signals with oposite directions on last added bars
                    // lets keep it simple now - lets ignore them
                    if (m_last_raw_signal[j] != direction) {
                        m_last_raw_signal_index = -1;
                        return -1;
                    }
                    else
                        // same direction only update index to get last
                        m_last_raw_signal_index = i;
            }
        }
    }
    return m_last_raw_signal_index;
}