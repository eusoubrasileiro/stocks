#include "databuffers.h"
#include <fstream>

#ifdef DEBUG
std::ofstream debugfile("debug_log.txt");
#else
#define debugfile std::cout
#endif

// Base Data
std::shared_ptr<BufferMqlTicks> m_ticks; // buffer of ticks w. control to download unique ones.

std::shared_ptr<MoneyBarBuffer> m_bars; // buffer of money bars base for everything

void DataBuffersInit(double ticksize, 
                     double tickvalue,
                     double moneybar_size,  // R$ to form 1 money bar
                     char* cs_symbol,
                     float start_hour,
                     float end_hour)  // cs_symbol is a char[] null terminated string (0) value at end
{

#ifdef DEBUG
    try {
#endif
        m_ticks.reset(new BufferMqlTicks());
        m_bars.reset(new MoneyBarBuffer());
        m_ticks->Init(std::string(cs_symbol));
        m_bars->Init(tickvalue, ticksize, moneybar_size);
        // for setting all moneybars `.inday` flag
        m_bars->SetHours(start_hour, end_hour);
#ifdef  DEBUG
    }
    catch (const std::exception& ex) {
        debugfile << "c++ exception: " << std::endl;
        debugfile << ex.what() << std::endl;
    }
    catch (...) {
        debugfile << "Weird no idea exception" << std::endl;
    }
#endif //  DEBUG
}

unixtime_ms next_bgticktime = 0; // if an exception occour will


// returns the next cmpbegin_time
unixtime_ms OnTicks(MqlTick* mt5_pticks, int mt5_nticks, double* lost_ticks) {
    
#ifdef  DEBUG
    try {
#endif  //  DEBUG    

        //m.lock(); // causes
        // c++ exception: 
        // device or resource busy : device or resource busy
        int ticks_left = mt5_nticks;
        // send ticks on 'batches' of MAX_TICKS
        // changed to this to avoid needing a HUGE buffer for ticks
        while (ticks_left > 0) {

            next_bgticktime = m_ticks->Refresh(mt5_pticks, std::min(ticks_left, MAX_TICKS), lost_ticks);
            ticks_left -= MAX_TICKS;

            if (m_ticks->nNew() > 0) {
                m_bars->AddTicks(m_ticks->end() - m_ticks->nNew(), m_ticks->end());
                // m_bars call-backs all subscribed to OnNewBars
            }
        }

#ifdef  DEBUG
    }
    catch (const std::exception& ex) {
        debugfile << "c++ exception: " << std::endl;
        debugfile << ex.what() << std::endl;
        // On periods of high volatility
        // or internet connection problems
        // many many ticks are dropped (also commented on MT5 forums)
        // solved by accepting losses
    }
    catch (...) {
        debugfile << "Weird no idea exception" << std::endl;
        debugfile << "mt5_nticks " << mt5_nticks << std::endl;
        debugfile << "ticks_left " << mt5_nticks << std::endl;
    }
#endif //  DEBUG

    return next_bgticktime;
}

size_t SearchMoneyBars(buffer<MoneyBar> mbars, uint64_t uid) {
    auto iter = std::lower_bound(mbars.begin(), mbars.end(), uid, cmpMoneyBarSmallUid);
    return (iter == mbars.end()) ? -1 : iter - mbars.begin();
}


//
//
// for DEBUGGING ONLY
//
//

BufferMqlTicks* GetBufferMqlTicks() {
    return &*m_ticks;
}

void TicksToFile(char* cs_filename) {
    SaveTicks(&*m_ticks, std::string(cs_filename));
}

bool isInsideFile(char* cs_filename) {
    return isInFile(&*m_ticks, std::string(cs_filename));
}


size_t BufferSize(){ return m_bars->capacity(); }

size_t BufferTotal(){ return m_bars->size(); }