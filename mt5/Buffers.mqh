#include "Util.mqh"

// FIFO first in first out
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

    void MakeSpace(int space_needed){
        if(space_needed >= m_data_max){ // otherwise bellow will
            m_data_total = 0; // resize m_data array space
            return;
        }
        // copy data overwriting the oldest sample - overwriting the firsts
        // (shift left array)  making space for new samples in the end
        // dst, src, dst_idx_srt, src_idx_srt, count_to_copy
        ArrayCopy(m_data, m_data, 0, space_needed, m_data_total-space_needed);
        m_data_total -= space_needed;
    }

public:
    Type m_data[];

    CBuffer(void) {
        // ArrayResize(m_data, 16); // minimu size if specified by Resize
        m_data_total = 0;
        m_data_max = 0;
        m_step_resize = 16;
    }
    ~CBuffer(void) { ArrayFree(m_data); }

    Type operator[](const int index) const { return m_data[index]; }

    int Size(void){ return m_data_total; }

    void RemoveLast(void){ if(m_data_total>0) m_data_total--; }

    bool Add(Type element){
      if(m_data_total >= m_data_max) // m_data
        MakeSpace(1);
      //--- add data in the end
      m_data[m_data_total++]=element;
      return(true);
    }

    // you may want to insert a range smaller than the full size
    // of elements array
    void AddRange(Type &elements[], int start=0, int tsize=0){
      tsize = (tsize <= 0) ? ArraySize(elements): tsize;
      //int tsize = ArraySize(elements);
      // int space_needed = (m_data_total+tsize)-m_data_max;
      // // 5 + 6 - 10 = 1
      // if(space_needed > 0)// m_data
      //   MakeSpace(space_needed);
      // garantee not resizing m_data
      //start = (tsize > m_data_max)? tsize-m_data_max : start;
      for(int i=start; i<tsize; i++)
         Add(elements[i]);
    }

    void AddEmpty(int count){ // and count samples with EMPTY value
        int space_needed = (m_data_total+count)-m_data_max;

        if(space_needed > 0)// m_data
            MakeSpace(space_needed);

        ArrayFill(m_data, m_data_total, count, EMPTY_VALUE);
        m_data_total += count;
    }

    // this to garantee compatibility with Resize of
    // buffers of CIndicators and default mql5 libraries
    // and CExpert, CExpertBase etc.
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

     bool ResizeFixed(const int size)
     {
        m_data_max = size;
        return ArrayResize(m_data, size);
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

    // this to garantee compatibility with Resize of
    // buffers of CIndicators and default mql5 libraries
    // and CExpert, CExpertBase etc.
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
      // int space_needed = (m_data_total+tsize)-m_data_max;
      // // 5 + 6 - 10 = 1
      // if(space_needed > 0){ // m_data
      //   // copy data overwriting the oldest sample - overwriting the firsts
      //   // (shift left array)  making space for new samples in the end
      //   // src, dst, dst_idx_srt, src_idx_srt, count_to_copy
      //   ArrayCopy(m_data, m_data, 0, space_needed, m_data_total-space_needed);
      //   m_data_total -= space_needed;
      // }
      // ArrayCopy(m_data, elements, m_data_total, 0, tsize);
      //  m_data_total += tsize;
      for(int i=0; i<tsize; i++)
        Add(elements[i]);
    }

    // this to garantee compatibility with Resize of
    // buffers of CIndicators and default mql5 libraries
    // and CExpert, CExpertBase etc.
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

//////////////////////////////////
// Circular Buffers - Fixed Size//
/////////////////////////////////

// much faster because dont need to ArrayCopy
// everything once full

template<typename Type>
class CCBuffer
{
protected:
    int m_data_max;
    int m_intdiv;

public:
    Type m_data[];
    int m_cposition; // current position of end of data
    int isfull;

    CCBuffer();

    CCBuffer(int size) {
        SetSize(size);
    }

    void SetSize(int size){
      isfull = 0;
      m_data_max = size;
      m_cposition = 0;
      ArrayResize(m_data, size);
    }

    ~CCBuffer(void) { ArrayFree(m_data); }

    Type operator[](const int index) const { return m_data[index]; }

    int Count(void){ return (isfull!=0)? m_data_max: m_cposition; }

    void RemoveLast(void){
      if(m_cposition==0)
        m_cposition = m_data_max-1;
      else
        m_cposition--;
    }

    void Add(Type element){
      //--- add data in the end
      m_data[m_cposition]=element;
      m_cposition++;
      // integer division
      m_intdiv = m_cposition/m_data_max; // if bigger is gonna be 1
      m_cposition -= m_intdiv*m_data_max; // same as making (m_cposition +1)%m_data_max
      isfull = (isfull!=0)? isfull : m_intdiv;
    }

    // you may want to insert a range smaller than the full size
    // of elements array
    void AddRange(Type &elements[], int start=0, int tsize=0){
      tsize = (tsize <= 0) ? ArraySize(elements): tsize;
      for(int i=start; i<tsize; i++)
         Add(elements[i]);
    }

    void AddEmpty(int count){ // and count samples with EMPTY value
      for(int i=0; i<count;i++)
        Add(EMPTY_VALUE);
    }

    int Size(){ return m_data_max; }

    // given begin
    // -- index based on start of data
    // and count
    // -- number of elements to copy
    // returns indexes of data to performs copy
    // in two parts or one part
    // start and end of two parts
    // depending on the m_cposition
    // return 1 or 2 depending if two parts
    // or only 1 part needed to be copied
    int indexesData(int  begin,  int count,
                    int &start1, int &end1,
                    int &start2, int &end2){
      int tmpcount = Count();
      if(count > m_data_max | count > tmpcount | tmpcount == 0)
        return 0;
      if(m_cposition!=0 && isfull!=0){
        // buffer has two parts full filled
        start1 = (m_cposition + begin);
        if(start1 > m_data_max){
          start1 %= m_data_max;
          end1 = start1 + count; // cannot go around again
          // otherwise count would be bigger than m_data_max
          start2 = 0;
          end2 = 0;
          return 1;
        }
        end1 = start1 + count;
        if(end1 > m_data_max){
          start2 = 0;
          end2 = end1%m_data_max;
          end1 = m_data_max;
          return 2;
        }
        start2 = 0;
        end2 = 0;
        return 1;
      }
      // one part only
      start1 = begin;
      end1 = begin+count;
      start2 = 0;
      end2 = 0;
      return 1;
    }

};

// Same as above but for structs
// without  AddEmpty
template<typename Type>
class CCStructBuffer
{
protected:
    int m_data_max;
    int m_intdiv;

public:
    Type m_data[];
    int m_cposition; // current position of end of data
    int isfull;

    CCStructBuffer(){isfull=0;m_cposition=0;};

    CCStructBuffer(int size) {
        SetSize(size);
    }

    void SetSize(int size){
      isfull = 0;
      m_data_max = size;
      m_cposition = 0;
      ArrayResize(m_data, size);
    }

    ~CCStructBuffer(void) { ArrayFree(m_data); }

    Type operator[](const int index) const { return m_data[index]; }

    int Count(void){ return (isfull!=0)? m_data_max: m_cposition; }

    void RemoveLast(void){
      if(m_cposition==0)
        m_cposition = m_data_max-1;
      else
        m_cposition--;
    }

    void Add(Type &element){
      //--- add data in the end
      m_data[m_cposition]=element;
      m_cposition++;
      // integer division
      m_intdiv = m_cposition/m_data_max; // if bigger is gonna be 1
      m_cposition -= m_intdiv*m_data_max; // same as making (m_cposition +1)%m_data_max
      isfull = (isfull!=0)? isfull : m_intdiv;
    }

    // you may want to insert a range smaller than the full size
    // of elements array
    void AddRange(Type &elements[], int start=0, int tsize=0){
      tsize = (tsize <= 0) ? ArraySize(elements): tsize;
      for(int i=start; i<tsize; i++)
         Add(elements[i]);
    }

    int Size(){ return m_data_max; }

    // given begin
    // -- index based on start of data
    // and count
    // -- number of elements to copy
    // returns indexes of data to performs copy
    // in two parts or one part
    // start and end of two parts
    // depending on the m_cposition
    // return 1 or 2 depending if two parts
    // or only 1 part needed to be copied
    int indexesData(int  begin,  int count,
                    int &start1, int &end1,
                    int &start2, int &end2){
      int tmpcount = Count();
      if(count > m_data_max | count > tmpcount | tmpcount == 0)
        return 0;
      if(m_cposition!=0 && isfull!=0){
        // buffer has two parts full filled
        start1 = (m_cposition + begin);
        if(start1 > m_data_max){
          start1 %= m_data_max;
          end1 = start1 + count; // cannot go around again
          // otherwise count would be bigger than m_data_max
          start2 = 0;
          end2 = 0;
          return 1;
        }
        end1 = start1 + count;
        if(end1 > m_data_max){
          start2 = 0;
          end2 = end1%m_data_max;
          end1 = m_data_max;
          return 2;
        }
        start2 = 0;
        end2 = 0;
        return 1;
      }
      // one part only
      start1 = begin;
      end1 = begin+count;
      start2 = 0;
      end2 = 0;
      return 1;
    }

};
