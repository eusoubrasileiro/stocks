#include "ticks.h"
#include <fstream>

#ifdef DEBUG
#include <fstream> // debugging dll load by metatrader 5 output to txt file -> located where it started
extern std::ofstream debugfile;
#else
#define debugfile std::cout
#endif

int64_t ReadTicks(std::vector<MqlTick> *mqlticks,
        std::string filename, size_t nticks=-1){

    std::fstream fh;
    std::streampos begin, end;

    fh.open(filename,
        std::fstream::in | std::fstream::binary);
    // calculate number of ticks on file
    begin = fh.tellg();
    fh.seekg(0, std::ios::end);
    end = fh.tellg();

    if (nticks == -1)
        nticks = (end - begin) / sizeof(MqlTick);
    else { // in case number of ticks to read is specified
        fh.seekg(sizeof(MqlTick)*nticks, std::ios::beg);
        end = fh.tellg();
    }

    mqlticks->resize(nticks);

    fh.seekg(0, std::ios::beg);
    fh.read((char*)mqlticks->data(), end);
    fh.close();

    return nticks;
}

void SaveTicks(BufferMqlTicks* ticks, std::string filename){
    std::fstream fh;
    std::streampos begin, end;

    fh.open(filename,
        std::fstream::out | std::fstream::binary);

    for (auto tick = ticks->begin(); tick != ticks->end(); tick++) {
        fh.write((char*) (&(*tick)), sizeof(MqlTick));
    }

    fh.close();
}

#include <iostream>

void PrintTicks(MqlTick *ticks, int size, std::ofstream &file){
    char line[256];
    file  << "       time_msc       |   last   |  r.volume |  volume  " << std::endl;
    for (int i = 0; i < size; i++) {
        sprintf(line, "%22llu|%9.1f|%11g|%10llu", ticks[i].time_msc, ticks[i].last, ticks[i].volume_real, ticks[i].volume);
        file <<  std::string(line) << std::endl;
    }
}

void PrintTicks(boost::circular_buffer<MqlTick>::iterator begin, boost::circular_buffer<MqlTick>::iterator end,
    std::ofstream &file) {
    char line[256];
    file << "       time_msc       |   last   |  r.volume |  volume  " << std::endl;
    for (auto tick = begin; tick != end; tick++) {
        sprintf(line, "%22llu|%9.1f|%11g|%10llu", (*tick).time_msc, (*tick).last, (*tick).volume_real, (*tick).volume);
        file << std::string(line) << std::endl;
    }
}

// check if the ticks are all included in
// the ticks on the specified filename
// seek file if needed
bool isInFile(BufferMqlTicks* ticks, std::string filename){
    std::vector<MqlTick> fticks;
    ReadTicks(&fticks, filename);

    // start iterator of ticks -> point to first tick on file
    auto cftick = fticks.begin();

    // seek to the start positon for the ticks if needed
    for (; cftick != fticks.end(); cftick++)
        if (cftick->time_msc == ticks->at(0).time_msc) break;

#ifdef DEBUGMORE
    debugfile << "isInFile seeked file start at: " << std::distance(fticks.begin(), cftick) << std::endl;
#endif

    auto creftick = ticks->begin();
    bool result = true;
    // compare
    for (; cftick != fticks.end() && creftick != ticks->end(); cftick++, creftick++)
        // memory comparisoin since there is no == for POD structs
        if(memcmp(&(*cftick), &(*creftick), sizeof(MqlTick))!=0) {
            result = false;
            break;
        }

#ifdef DEBUGMORE
    debugfile << "isInFile end of comparison: " << std::distance(ticks->begin(), creftick) << std::endl;
    debugfile << "isInFile total ticks compared: " << std::distance(ticks->begin(), creftick) << std::endl;
    debugfile << "isInFile : " << result << std::endl;
#endif

    return result;
}

// Fix array of ticks so if last ask, bid or last is 0
// they get filled with previous non-zero value as should be
// volume i dont care because it will only be usefull in change if flag >= 16
void fixArrayTicks(MqlTick *ticks, size_t nticks) {
    double       bid = 0;           // (Last) Bid price
    double       ask = 0;           // (Last) Ask price
    double       last = 0;          // (Last) Price of the last deal
    for (int i = 0; i < nticks; i++) { // cannot be done in parallel?
        if (ticks[i].bid == 0)
            ticks[i].bid = bid;
        else
            bid = ticks[i].bid; // save this for use next
        if (ticks[i].ask == 0)
            ticks[i].ask = ask;
        else
            ask = ticks[i].ask; // save this for use next
        if (ticks[i].last == 0)
            ticks[i].last = last;
        else
            last = ticks[i].last; // save this for use next
        // TICK BUY or TICK SELL == most time not set
        // very predictive power information?? or Market Makers?
        if (ticks[i].last != ticks[i].ask)
            ticks[i].flags = TICK_FLAG_SELL;
        else
            ticks[i].flags = TICK_FLAG_BUY;
    }
}

void fixArrayTicks(std::vector<MqlTick> ticks) {
    size_t nticks = ticks.capacity();
    fixArrayTicks(ticks.data(), nticks);
}


//////////////////////////////////////////////
//////////////////////////////////////////////

// secury check of sync with server of ticks
// and seek to begin of new ticks
bool BufferMqlTicks::seekBeginMt5Ticks(){
    m_scheck = false;

    #ifdef DEBUGMORE
        debugfile << "BufferMqlTicks seekBeginMt5Ticks mt5ncopied: " << m_mt5ncopied << std::endl;
        debugfile << "BufferMqlTicks seekBeginMt5Ticks (Sync Check) 1st local tick: " << (*this)[m_scheck_bg_idx + 0].time_msc << std::endl;
        debugfile << "BufferMqlTicks seekBeginMt5Ticks (Sync Check) 1st  mt5  tick: " << m_mt5ticks[0].time_msc << std::endl;
    #endif


    int lost_ticks = 0; // 10 ticks can be wrong
    // before find the equals inside security check window

    int mt5i = 0; // indexer for received ticks
    // find index of first tick different
    // from what we already have using the comparision indexes
    if(this->size() == 0){ // we have nothing - will try copy everything
        m_scheck = true;
        m_nnew = m_mt5ncopied;
    }
    else{
        // get begin of new ticks from ticks sent by metatrader 5
        // comparing with last received
        // secury check of data with server - just get how many are being lost
        // CopyTicksRange(..., ..., ..., xxxx300-200, 0)
        // cannot make a request with overlap for sync check
        // because that drop is certain and must be acceptable
        for (; mt5i < m_mt5ncopied; mt5i++, m_mt5ticks++) {
            // some ticks might have been lost from previsous call
            // that cannot be solved
            // so verify where in the mt5 ticks is the last tick we got
            // return number of lost ticks
            if (m_mt5ticks->time_msc == this->back().time_msc) {
                m_scheck = true; // found begin
                mt5i++; // begin is next
                m_mt5ticks++; // walk to begin
                break; // and stop
            }
        }
        if (!m_scheck) { // did not find begin of real data - something really wrong
            #ifdef DEBUGMORE
            #define fileout debugfile
            #else
                    auto fileout = std::ofstream("wrong_ticks.txt");
                    fileout << "wrong_ticks > 10" << std::endl;
            #endif     
                    fileout << "begin check time: " << this->back().time_msc << std::endl;
                    fileout << "begin check idx: " << this->size()-1 << std::endl;
                    auto printstart = (this->size() - 10 > 0) ? this->size() - 10 : 0;
                    PrintTicks(this->begin() + printstart, this->end(), fileout); // print last 10
                    fileout << "number of ticks from mt5 " << m_mt5ncopied << std::endl;
                    fileout << "from mt5 ticks " << std::endl;
                    PrintTicks(m_mt5ticks - mt5i, std::min(200, m_mt5ncopied), fileout);
                    debugfile << "Data Without Sync With Server!" << std::endl;
                    throw new std::runtime_error("Tick data out-of-sync with server!");
        }
        m_nnew = m_mt5ncopied - mt5i;
    }

    #ifdef DEBUG
        if (mt5i > 1) // that happens 
            debugfile << "lost ticks: " << mt5i - 1 << std::endl;
    #endif

    #ifdef DEBUGMORE
        debugfile << "BufferMqlTicks seekBeginMt5Ticks  wrong ticks (100) : " << wrong_ticks << std::endl;
        debugfile << "BufferMqlTicks seekBeginMt5Ticks  mt5 begin idx : " << mt5i << std::endl;
        debugfile << "BufferMqlTicks seekBeginMt5Ticks  nnew : " << m_nnew << std::endl;
    #endif

    return mt5i-1;
}


// number of new ticks added on last call to Refresh(...)
int BufferMqlTicks::nNew(){ // you may add more data than the buffer
    return (m_nnew > BUFFERSIZE) ? BUFFERSIZE : m_nnew;
}

void BufferMqlTicks::Init(std::string symbol){
    m_symbol = symbol;
    m_nnew = 0;

#ifdef DEBUGMORE
    debugfile << "BufferMqlTicks symbol: " << m_symbol << std::endl;
    debugfile << "BufferMqlTicks timenow: " << timenow << std::endl;
    debugfile << "BufferMqlTicks scheck_bg_time: " << m_scheck_bg_time << std::endl;
#endif

}

// int64_t same as long for Mql5
// will be called by Metatrader 5 after
// m_ncopied = CopyTicksRange(m_symbol, m_copied_ticks,
// COPY_TICKS_ALL, m_cmpbegin_time, 0)
// passing m_copied_ticks as *pmt5_mqlticks
// will compare with ticks
unixtime_ms BufferMqlTicks::Refresh(MqlTick *mt5_pmqlticks, int mt5_ncopied){

    m_mt5ticks = mt5_pmqlticks;
    m_mt5ncopied = mt5_ncopied;
    m_nnew = 0;

    if (mt5_ncopied < 1) // no data
        return this->back().time_msc;

    // secury check and get begin
    seekBeginMt5Ticks();

    // fix nonsense last, ask, bid empty
    fixArrayTicks(m_mt5ticks, m_nnew);

    // if on back-test use every tick based on real ticks
    addrange(m_mt5ticks, m_nnew);

#ifdef DEBUGMORE
        debugfile << "BufferMqlTicks Refresh nnew : " << m_nnew << std::endl;
        debugfile << "BufferMqlTicks Refresh mt5ticks last new : " << m_mt5ticks[m_nnew - 1].time_msc << std::endl;
        debugfile << "BufferMqlTicks Refresh scheck_bg_time : " << m_scheck_bg_time << std::endl;
#endif
        // get begin time for next call by Metatrader 5 to CopyTicks
    return this->back().time_msc;
}


// we will use this comparator
// compare a MqlTick's to a time_ms to see if it comes before it or not
inline bool cmpTickSmallTimeMs(const MqlTick& a, unixtime_ms time) {
    return a.time_msc < time;
}

// std::lower_bound
// Returns an iterator pointing to the first element in the range [first,last)
// which does not compare less than val.
// The elements are compared using operator< or comparator function (cmpMoneyBarSmallUid)
// since uid
// 1. does not repeat
// 2. are sorted
// above will get the index of the uid on the buffer of bars

// return index on ring buffer for
// for first tick with time_ms bigger or equal than or -1 not finding
size_t MqltickTimeGtEqIdx(std::vector<MqlTick> ticks, unixtime_ms time)
{
    auto iter = std::lower_bound(ticks.begin(), ticks.end(), time, cmpTickSmallTimeMs);
    return (iter == ticks.end()) ? -1 : iter - ticks.begin();
}
