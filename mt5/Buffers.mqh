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
    Type m_data[];
    int m_data_total;
    int m_data_max;
    int m_step_resize;

public:
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
        m_data[m_data_total-1] = element;
      }
      else //--- add data in the end
        m_data[m_data_total++]=element;
      return(true);
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

    // Get Data At index position using Array As Series Convention
    // youngest sample is at (0)
    Type *GetData(const int index) const
    {
       return(m_data[m_data_total-1-index]);
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

protected:
    // called only when buffer full
    // Move all pointers to the left
    // create space in the end for new data
    void ShiftLeftBuffer(){
        for(int i=0; i<m_data_total-1; i++){
            m_data[i] = m_data[i+1];
        }
    }

};
