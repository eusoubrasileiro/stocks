//struct MqlTick
//  {
//   datetime     time;          // Time of the last prices update
//   double       bid;           // Current Bid price
//   double       ask;           // Current Ask price
//   double       last;          // Price of the last deal (Last)
//   ulong        volume;        // Volume for the current Last price
//   long         time_msc;      // Time of a price last update in milliseconds
//   uint         flags;         // Tick flags
//   double       volume_real;   // Volume for the current Last price with greater accuracy
//  };

// Fix array of ticks so if last ask, bid or last is 0
// they get filled with previous non-zero value as should be
void FixArrayTicks(MqlTick &ticks[]){
  int nticks = ArraySize(ticks);
  double       bid = 0;           // Current Bid price
  double       ask = 0;           // Current Ask price
  double       last = 0;          // Price of the last deal (Last)
  for(int i=0; i<nticks;i++){ // cannot be done in parallel?
    if(ticks[i].bid == 0)
       ticks[i].bid = bid;
    else
       bid = ticks[i].bid; // save this for use next
    if(ticks[i].ask == 0)
       ticks[i].ask = ask;
    else
       ask = ticks[i].ask; // save this for use next
    if(ticks[i].last == 0)
       ticks[i].last = last;
    else
       last = ticks[i].last; // save this for use next
  }
}
