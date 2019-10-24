#include "..\Buffers.mqh"

// time equivalent to _ftime64	<sys/types.h> and <sys/timeb.h>

//  mine   _ftime64
//         dstflag	--- ignored
//  ms     millitm	Fraction of a second in milliseconds.
//  time   time	Time in seconds since midnight (00:00:00), January 1, 1970, coordinated universal time (UTC).
//         timezone --- ignored


// minimum needed for identify day of a tick timestamp in milliseconds
// can be inserted on MoneyBar struct
// and C++ could create array of timedays from
// array of moneybar structs 
struct timeday {
  int day; // day of year from MqlDateTime.day_of_year
  long ms;
};

// Same as CStructBuffer but for timeday struct
// adding QuickSearch functinality
class CTimeDayBuffer : public CStructBuffer<timeday>
{
  MqlDateTime m_last_mqldtime;  

public:
    timeday     m_last;
  // code from clong array
  // quick search for a sorted array
 int QuickSearch(const timeday &element) const
  {
   int  i,j,m=-1;
   long t_long;
//--- search
   i=0;
   j=m_data_total-1;
   while(j>=i)
     {
      //--- ">>1" is quick division by 2
      m=(j+i)>>1;
      if(m<0 || m>=m_data_total)
         break;
      t_long=m_data[m].ms;
      if(t_long==element.ms)
         break;
      if(t_long>element.ms)
         j=m-1;
      else
         i=m+1;
     }
//--- position
   return(m);
  }

  void Add(long &time_ms){
    // convert ms timestamp to linux epoch (time-zone is already accounted for)
    TimeToStruct((datetime) time_ms/1000, m_last_mqldtime);
    m_last.ms = time_ms;
    m_last.day = m_last_mqldtime.day_of_year;
    CStructBuffer<timeday>::Add(m_last);
  }

  // you may want to insert a range smaller than the full size of elements array
  void AddRange(long &time_ms[], int tsize=0){
    for(int i=0; i<tsize; i++)
      Add(time_ms[i]);
  }

};


// Same as CStructBuffer but for MqlDateTime
// adding QuickSearch functinality
class CMqlDateTimeBuffer : public CStructBuffer<MqlDateTime>
{

public:
   // code from clong array
  // quick search for a sorted array
  int QuickSearch(MqlDateTime &element) const
    {
     int  i,j,m=-1;
     MqlDateTime mqldtcurrent;
     datetime d_mqldtcurrent;
     datetime d_element = StructToTime(element);
  //--- search
     i=0;
     j=m_data_total-1;
     while(j>=i)
       {
        //--- ">>1" is quick division by 2
        m=(j+i)>>1;
        if(m<0 || m>=m_data_total)
           break;
        mqldtcurrent=m_data[m];
        d_mqldtcurrent = StructToTime(mqldtcurrent);
        // if(mqldtcurrent==element)
        if(IsEqualMqldt(mqldtcurrent, element))
           break;
        if(d_mqldtcurrent>d_element)
           j=m-1;
        else
           i=m+1;
       }
  //--- position
     return(m);
    }

};
