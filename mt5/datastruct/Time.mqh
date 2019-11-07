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
     return(IsEqualMqldt(mqldtcurrent, element)? m : -1);
    }

};


// Circular Buffer Version
// Same as CStructBuffer but for timeday struct
// adding QuickSearch functinality
class CCTimeDayBuffer : public CCStructBuffer<timeday>
{
  MqlDateTime m_last_mqldtime;  

  // code from clong array
  // quick search for a sorted array
 int m_QuickSearch(long element, int start, int end) 
  {
   int  i,j,m=-1;
   long t_long;
//--- search
   i=start;
   j=end-1;
   while(j>=i)
     {
      //--- ">>1" is quick division by 2
      m=(j+i)>>1;
      if(m<start || m>=end)
         break;
      t_long=m_data[m].ms;
      if(t_long==element)
         break;
      if(t_long>element)
         j=m-1;
      else
         i=m+1;
     }
//--- position   
   return (t_long==element)? m: -1;
  }
  
public:
   timeday     m_last;
  
  int QuickSearch(long element_ms) 
  {
    int nparts, start, end, ifound, i, parts[4];
    nparts = indexesData(0, Count(), parts);
    for(i=0; i<nparts;i++){
      start = parts[i*2];
      end = parts[1+i*2];
      ifound = m_QuickSearch(element_ms, start, end);
      if(ifound != -1)
        break;
    }
    // convert to start based index              
    return toIndex(ifound);     
  }

  int QuickSearch(timeday &element){
     return QuickSearch(element.ms);
  } 


  void AddTimeMs(long time_ms){
    // convert ms timestamp to linux epoch (time-zone is already accounted for)
    TimeToStruct((datetime) time_ms/1000, m_last_mqldtime);
    m_last.ms = time_ms;
    m_last.day = m_last_mqldtime.day_of_year;
    CCStructBuffer<timeday>::Add(m_last);
  }

  // you may want to insert a range smaller than the full size of elements array
  void AddRangeTimeMs(long &time_ms[], int start=0, int tsize=0){
    tsize = (tsize <= 0) ? ArraySize(time_ms): tsize;
    for(int i=start; i<tsize; i++)
      AddTimeMs(time_ms[i]);
  }
  


};