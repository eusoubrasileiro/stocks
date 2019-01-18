#include "BbandsDefinitions.mqh"


double volumeToClose(long positionid)
  {
   ulong ticket;
   double volumeout=0; // volume of sells realized on the last expiretime period
   double volumein=0; // first buy realized on the period
   double volumeoutbefore=0;
   double volume=0; // total volume to sell
   datetime now = TimeCurrent();
   datetime dayBegin=dayBegin(now);
//--- request trade history for day
   HistorySelect(dayBegin,now);
   uint  total=HistoryDealsTotal(); // total deals
   for(uint i=0;i<total;i++)
     { // get the first deal buy of this position
      ticket=HistoryDealGetTicket(i);
      if(HistoryDealGetInteger(ticket,DEAL_TYPE)==DEAL_TYPE_BUY && 
         HistoryDealGetInteger(ticket,DEAL_MAGIC)==EXPERT_MAGIC && 
         HistoryDealGetInteger(ticket,DEAL_POSITION_ID)==positionid)
        {
         volumein=HistoryDealGetDouble(ticket,DEAL_VOLUME);
         datetime opentime=HistoryDealGetInteger(ticket,DEAL_TIME);
         if(now>opentime+expiretime)
           { // need to expire this order
            // get how much was sold on the expired time period
            volumeout=0;
            for(uint j=i;j<total;j++)
              { // get the sell volume on the period
               ticket=HistoryDealGetTicket(j); //--- try to get deals ticket
               if(HistoryDealGetInteger(ticket,DEAL_TYPE)==DEAL_TYPE_SELL && 
                  HistoryDealGetInteger(ticket,DEAL_MAGIC)==EXPERT_MAGIC && 
                  HistoryDealGetInteger(ticket,DEAL_POSITION_ID)==positionid)
                 { // only a sell (sell out not)
                  volumeout+=HistoryDealGetDouble(ticket,DEAL_VOLUME);
                 }
              }
            volumeout-=volumeoutbefore;
            if(volumein-volumeout>0)
              {
               volume+=(volumein-volumeout);
               volumeoutbefore+=volumeout; // the volume deduced
              }
           }
        }
     }
   return volume;
  }
//+------------------------------------------------------------------+
//|                                                                  |
//+------------------------------------------------------------------+
void ClosePositionbyTime(){
   MqlTradeRequest request;
   MqlTradeResult  result;
   double volume=0;
   datetime timenow= TimeCurrent();
   datetime dayend = dayEnd(timenow);
   datetime daybegin=dayBegin(timenow);
// NET MODE only ONE buy or ONE sell at once
   int total=PositionsTotal(); // number of open positions
   if(total<1) // nothing to do
      return;
   ulong  position_ticket=PositionGetTicket(0);  // ticket of the position
   //--- if the MagicNumber matches MagicNumber of the position
   if(PositionGetInteger(POSITION_MAGIC)!=EXPERT_MAGIC)
      return;
   if(timenow>=dayend)
     { // close whatever volume is open
      volume=PositionGetDouble(POSITION_VOLUME);
     }
   else 
     { // check expire time to see how much and if should close volume
      long positionid=PositionGetInteger(POSITION_IDENTIFIER);
      volume=volumeToClose(positionid);
      if(volume<=0)
         return;
     }
//--- zeroing the request and result values
   ZeroMemory(request);
   ZeroMemory(result);
//--- setting the operation parameters
   request.action   = TRADE_ACTION_DEAL;        // type of trade operation
   request.position = position_ticket;          // ticket of the position
   request.symbol=sname;          // symbol
   request.volume=volume;                   // volume of the position
   request.deviation=deviation*ticksize;                        // 7*0.01 tick size : 7 cents
   request.magic=EXPERT_MAGIC;             // MagicNumber of the position
   request.price   = SymbolInfoDouble(sname, SYMBOL_BID);
   request.type    =  ORDER_TYPE_SELL;
   if(!OrderSend(request,result))
      Print("OrderSend error ",GetLastError());
//--- information about the operation
   Print("closed by time - retcode ",result.retcode," deal ",result.deal);
}