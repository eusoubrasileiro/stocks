#include "ticks.h"
#include <fstream>

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


// Fix array of ticks so if last ask, bid or last is 0
// they get filled with previous non-zero value as should be
// volume i dont care because it will only be usefull in change if flag >= 16
void fixArrayTicks(MqlTick *ticks, size_t nticks) {
    double       bid = 0;           // Current Bid price
    double       ask = 0;           // Current Ask price
    double       last = 0;          // Price of the last deal (Last)
    double       volume = 0;
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
    }
}

void fixArrayTicks(std::vector<MqlTick> ticks) {
    size_t nticks = ticks.capacity();
    fixArrayTicks(ticks.data(), nticks);
}


bool BufferMqlTicks::correctMt5UnrealTicks(){
  int real_nnew = 0;
  int begin_refpos = m_refpos;
  for(int i=0; i<m_nnew && m_refpos < m_refsize; i++){
    if(m_mt5ticks[i].time_msc
        == m_refticks[m_refpos].time_msc){ // could and should be an iterator 
      real_nnew++; m_refpos++;
    }
  }
  // testing needs boundaries of data chuncks
  m_bound_ticks[ib_tick++] = m_refpos;

  /////// add correct ticks from file
  addrange(m_refticks.data() + begin_refpos, real_nnew);

  m_nnew = real_nnew;
  if(m_nnew==0){ // number of trash ticks created by Metatrader
    m_trash_count++;
    return false;
  }
  if(m_trash_count > 0){
      std::tm ctime;
      time_t timet = m_cmpbegin_time/1000;
      gmtime_s(&ctime, &timet);

      char buffer[32];
      // Format: Mo, 15.06.2009 20:20:00
      strftime(buffer, 32, "%a, %d.%m.%Y %H:%M:%S", &ctime);

      // can be a in a log file certainly!!! if verbose set
      debugfile << "Ignored metatrader 5 created unreal ticks : " << m_trash_count 
          << " at " << buffer << std::endl;

      m_trash_count = 0;
  }
  return true;
}

void BufferMqlTicks::loadCorrectTicks(std::string symbol)
{
    // must load specific file with ticks e.g. PETR4_ticks.bin
    // located user profile \stocks\data folder
    // if ticks on this file are not compatible with
    // your tick history. Problems will come
    // advised to use a custom symbol to avoid that
    
    m_refcticks_file = std::string(std::getenv("USERPROFILE")) + 
        std::string("\\Projects\\stocks\\data\\") +
        symbol + std::string("_mqltick.bin");    
    m_refsize = ReadTicks(&m_refticks, m_refcticks_file);

    // seek to the start positon of ticks for the 
    // current back test
    for(m_refpos = 0; m_refpos<m_refsize; m_refpos++)
        if(m_refticks[m_refpos].time_msc >= m_cmpbegin_time) break;
    m_trash_count = 0;
    
    // array of boundary of chuncks of ticks
    m_bound_ticks.resize(m_refsize);    
    ib_tick = 0;    // first boundary will come 
}

//////////////////////////////////////////////
//////////////////////////////////////////////

  bool BufferMqlTicks::beginNewTicks(){
    scheck = false;

    // find index of first tick different
    // from what we already have using the comparision indexes
    if(this->size() == 0){ // we have nothing - copy everything
      m_bgidx = 0; scheck = true;
    }
    else{ // security check 
      for(int i=0; i<m_mt5ncopied; i++)
        if(m_mt5ticks[i].time_msc == (*this)[m_cmpbegin+i].time_msc){
          scheck=true; // must have at least one sample equal
          // for security check of sync with server
        }
        else{ // found begin new data
          m_bgidx = i;
          break;
        }
    }
    // the difference is the number of new ticks
    m_nnew = m_mt5ncopied - m_bgidx;
    return (scheck);
  }

BufferMqlTicks::BufferMqlTicks()
{
    m_isbacktest = false;
}

void BufferMqlTicks::Init(std::string symbol, bool isbacktest, time_t timenow){
    m_isbacktest = isbacktest;
    m_symbol = symbol;
    m_nnew = 0;

    // time_t same as int64_t unixtime stamp
    // real time operations get current time now
    if (!isbacktest) {
        // get raw time than convert to struct tm and them get 
        // first 1 hour of day 1:00 am 
        time(&timenow);
    } 
    // time now in ms will work 'coz next tick.ms value will be bigger
    // and will take its place
    tm tm_time;
    gmtime_s(&tm_time, &timenow); // unixtime timestamp to tm_struct
    // begin of day at 1:00 am
    m_cmpbegin_time = timenow - (tm_time.tm_hour*3600+tm_time.tm_min*60+tm_time.tm_sec) + 3600;
    m_cmpbegin_time *= 1000;// turn in ms
    m_cmpbegin = 0;    

    if(isbacktest)
        loadCorrectTicks(symbol);
}

// int64_t same as long for Mql5
// will be called by Metatrader 5 after 
// m_ncopied = CopyTicksRange(m_symbol, m_copied_ticks,
// COPY_TICKS_ALL, m_cmpbegin_time, 0)
// passing m_copied_ticks as *pmt5_mqlticks
// will compare with ticks 
int64_t BufferMqlTicks::Refresh(MqlTick *mt5_pmqlticks, int mt5_ncopied){

    m_mt5ticks = mt5_pmqlticks;
    m_mt5ncopied = mt5_ncopied;

    if(!beginNewTicks()){
        debugfile << "Data Without Sync With Server!" << std::endl;
        throw new std::runtime_error("Tick data out-of-sync with server!");
    }

    // allign array of new ticks to zero
    // ArrayCopy(m_copied_ticks, m_copied_ticks, 0, m_bgidx, m_nnew);
    m_mt5ticks = m_mt5ticks + m_bgidx;
    // fix nonsense last, ask, bid empty
    fixArrayTicks(m_mt5ticks, m_nnew);

    // if on back-test
    // dont use ticks from metatrader, only times!
    if(m_isbacktest){
        if(!correctMt5UnrealTicks())
            return 0; // no new ticks
    }
    else{ // real time operation
        addrange(m_mt5ticks, m_nnew);
    }

    // get begin time of comparison to check for new ticks
    // last tick ms-1 on buffer
    // get index of first tick with time >= (last tick ms) - (1 ms)
    // begin time of comparison is tick with time >= [last copied tick - 1 ms)]
    m_cmpbegin_time = m_mt5ticks[m_nnew-1].time_msc-1;
    m_cmpbegin = 0;
    for(int64_t i=size()-1; i>=0; i--)
        if((*this)[i].time_msc < m_cmpbegin_time){
            m_cmpbegin = i+1; 
            break;
        }

    return m_cmpbegin_time;
}


// we will use this comparator
// compare a MqlTick's to a time_ms to see if it comes before it or not
inline bool cmpTickSmallTimeMs(const MqlTick& a, int64_t time) {
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
size_t MqltickTimeGtEqIdx(std::vector<MqlTick> ticks, int64_t time)
{
    auto iter = std::lower_bound(ticks.begin(), ticks.end(), time, cmpTickSmallTimeMs);
    return (iter == ticks.end()) ? -1 : iter - ticks.begin();
}


