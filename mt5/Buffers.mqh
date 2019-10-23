#include "Util.mqh"
// Buffer single is an array when new data is added
// it deletes the oldest bigger than buffer size
// like a Queue or FIFO
// Will not use series index convention of mql5 unecessary pain
// oldest samples are in the begging of array
// only one method will cover mql5 as series convention that GetData
// Works based on ArrayCopy so pay attention that:
// Array of classes and structures containing objects that require initialization aren't copied.
// An array of structures can be copied into an array of the same type only.

template<typename Type>
class CBuffer
{
protected:    
    int m_data_total;
    int m_data_max;
    int m_step_resize;

public:
    Type m_data[];
    
    CBuffer(void) {
        ArrayResize(m_data, 16); // minimum size
        m_data_total = 0;
        m_data_max = 16;
        m_step_resize = 16;
    }
    ~CBuffer(void) { ArrayFree(m_data); }

    Type operator[](const int index) const { return m_data[index]; }

    int Size(void){ return m_data_total; }

    void RemoveLast(void){ if(m_data_total>0) m_data_total--; }

    bool Add(Type element){
      if(m_data_total >= m_data_max){ // m_data
      // copy data overwriting the oldest sample
      // overwriting the first sample
        ArrayCopy(m_data, m_data, 0, 1, m_data_total-1);
        //--- add to the end
        m_data_total--;
      }
      //--- add data in the end
      m_data[m_data_total++]=element;
      return(true);
    }

    // you may want to insert a range smaller than the full size 
    // of elements array
    void AddRange(Type &elements[], int tsize=0){
      tsize = (tsize <= 0) ? ArraySize(elements): tsize;
      //int tsize = ArraySize(elements);
      int space_needed = (m_data_total+tsize)-m_data_max;
      // 5 + 6 - 10 = 1
      if(space_needed > 0){ // m_data
        // copy data overwriting the oldest sample - overwriting the firsts
        // (shift left array)  making space for new samples in the end
        // dst, src, dst_idx_srt, src_idx_srt, count_to_copy
        ArrayCopy(m_data, m_data, 0, space_needed, m_data_total-space_needed);
        m_data_total -= space_needed;
      }
      ArrayCopy(m_data, elements, m_data_total, 0, tsize);
      m_data_total += tsize;
    }
    
    

    bool Resize(const int size)
      {
       int new_size;
    //--- check
       if(size<0)
          return(false);
    //--- resize array
       new_size=m_step_resize*(1+size/m_step_resize);
       if(m_data_max!=new_size)
         {
          if((m_data_max=ArrayResize(m_data,new_size))==-1)
            {
             m_data_max=ArraySize(m_data);
             return(false);
            }
         }
       if(m_data_total>size)
          m_data_total=size;
    //--- result
       return(m_data_max==new_size);
      }

    // Get Data At index position using Array As Series Convention
    // youngest sample is at (0)
    Type GetData(const int index) const
    {
       return(m_data[m_data_total-1-index]);
    }

    void SetData(const int index, Type value)
    {
       m_data[m_data_total-1-index] = value;
    }

    int BufferSize(){ return m_data_max; }


};

// Same as above but for classes
template<typename Type>
class CObjectBuffer
{
    int m_data_total;
    int m_data_max;
    int m_step_resize;
    Type          *m_data[];           // data array
public:
    CObjectBuffer(void) {
        ArrayResize(m_data, 16); // minimum size
        m_data_total = 0;
        m_data_max = 16;
        m_step_resize = 16;
    }
    ~CObjectBuffer(void) {
        for(int i=0; i<m_data_total; i++){
            delete m_data[i];
        }
        ArrayFree(m_data);
    }
    bool Add(Type *element){
            //--- check
      if(!CheckPointer(element))
          return(false);
      if(m_data_total >= m_data_max){ // m_data
      // copy data overwriting the oldest sample
      // overwriting the first sample
        ShiftLeftBuffer();
        //--- add to the end
        m_data[m_data_total-1] = element;
      }
      else //--- add data in the end
        m_data[m_data_total++]=element;
      return(true);
    }

    Type *operator[](const int index) const { return m_data[index]; }

    int Size(void){ return m_data_total; }

    void RemoveLast(void){
        if(m_data_total>0){
          delete m_data[m_data_total-1];
          m_data_total--;
        }
    }

    void Remove(const int index)
    {
        delete m_data[index];
        ShiftLeftBuffer(index);
        m_data[m_data_total-1] = NULL;
        m_data_total--;
    }

    Type *Last() // last added
    {
       return GetData(0);
    }

     bool Resize(const int size)
     {
       int new_size;
    //--- check
       if(size<0)
          return(false);
    //--- resize array
       new_size=m_step_resize*(1+size/m_step_resize);
       if(m_data_max!=new_size)
         {
          if((m_data_max=ArrayResize(m_data,new_size))==-1)
            {
             m_data_max=ArraySize(m_data);
             return(false);
            }
         }
       if(m_data_total>size)
          m_data_total=size;
    //--- result
       return(m_data_max==new_size);
    }

    int BufferSize(){ return m_data_max; }

    // following use
    // using Array As Series Convention
    // youngest sample is at (0)
    // Remove data at index position using Array As Series Convention
    void RemoveData(const int index){
        Remove(m_data_total-1-index);
    }

    // Get Data At index position using Array As Series Convention
    Type *GetData(const int index) const
    {
       return(m_data[m_data_total-1-index]);
    }


protected:
    // called only when buffer full
    // Move all pointers to the left
    // create space in the end for new data
    void ShiftLeftBuffer(int start=0){
        for(int i=start; i<m_data_total-1; i++){
            m_data[i] = m_data[i+1];
        }
    }

};

// Same as above but for structs
template<typename Type>
class CStructBuffer
{
protected:

    int m_data_total;
    int m_data_max;
    int m_step_resize;

public:

    Type          m_data[];           // data array

    CStructBuffer(void) {
        ArrayResize(m_data, 16); // minimum size
        m_data_total = 0;
        m_data_max = 16;
        m_step_resize = 16;
    }
    ~CStructBuffer(void) { ArrayFree(m_data); }

    Type operator[](const int index) const { return m_data[index]; }

    int Size(void){ return m_data_total; }

    void RemoveLast(void){ if(m_data_total>0) m_data_total--; }

    void Add(Type &element){
      if(m_data_total >= m_data_max){ // m_data
      // copy data overwriting the oldest sample
      // overwriting the first sample
        ArrayCopy(m_data, m_data, 0, 1, m_data_total-1);
        //--- add to the end
        m_data_total--;
      }
      //--- add data in the end
      m_data[m_data_total++]=element;
    }

    // you may want to insert a range smaller than the full size of elements array
    void AddRange(Type &elements[], int tsize=0){
      tsize = (tsize <= 0) ? ArraySize(elements): tsize;
      //int tsize = ArraySize(elements);
      int space_needed = (m_data_total+tsize)-m_data_max;
      // 5 + 6 - 10 = 1
      if(space_needed > 0){ // m_data
        // copy data overwriting the oldest sample - overwriting the firsts
        // (shift left array)  making space for new samples in the end
        // src, dst, dst_idx_srt, src_idx_srt, count_to_copy
        ArrayCopy(m_data, m_data, 0, space_needed, m_data_total-space_needed);
        m_data_total -= space_needed;
      }
      ArrayCopy(m_data, elements, m_data_total, 0, tsize);
      m_data_total += tsize;
    }

    bool Resize(const int size)
      {
       int new_size;
    //--- check
       if(size<0)
          return(false);
    //--- resize array
       new_size=m_step_resize*(1+size/m_step_resize);
       if(m_data_max!=new_size)
         {
          if((m_data_max=ArrayResize(m_data,new_size))==-1)
            {
             m_data_max=ArraySize(m_data);
             return(false);
            }
         }
       if(m_data_total>size)
          m_data_total=size;
    //--- result
       return(m_data_max==new_size);
      }

    // Get Data At index position using Array As Series Convention
    // youngest sample is at (0)
    Type GetData(const int index) const
    {
       return(m_data[m_data_total-1-index]);
    }

    int BufferSize(){ return m_data_max; }

};


// Same as above but for MqlDateTime
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
