#include <stdio.h>
#include <string.h>
#include "metaEngine.h"

void sendOrder(double kind, double price, double volume,
              double sloss, double tprofit, double deviation,
              double ticket, double source){
    _orders[_iorder*N+OK] = kind;
    _orders[_iorder*N+OP] = price;
    _orders[_iorder*N+OV] = volume;
    _orders[_iorder*N+SL] = sloss;
    _orders[_iorder*N+TP] = tprofit;
    _orders[_iorder*N+DI] = deviation;
    _orders[_iorder*N+TK] = ticket;
    _orders[_iorder*N+OS] = source;
    _iorder++;
}

void updatePositionStops(int ipos, double *order){
    _positions[ipos*N+SL] = ((int) order[SL] == -1)? _positions[ipos*N+SL] : order[SL];
    _positions[ipos*N+TP] = ((int) order[TP] == -1)? _positions[ipos*N+TP] : order[TP];
}

void newPosition(double time, double price, double *order, double poskind){
    memcpy(&_positions[_ipos*N], order, sizeof(double)*7);
    _positions[_ipos*N+PT] = time; // time position opened
    _positions[_ipos*N+PP] = price;
    _positions[_ipos*N+PK] = poskind;
    _positions[_ipos*N+PV] = order[OV];
    updatePositionStops(_ipos, order);
}

void updatePositionTime(int ipos, double time){
    _positions[ipos*N+PC] = time;
}

void updatePositionPrice(int ipos, double *order, double price){
    // calculate and update average price of position (v0*p0+v1*p1)/(v0+v1)
    double wprice = _positions[ipos*N+PV]*_positions[ipos*N+PP]+order[OV]*price;
    wprice /= _positions[ipos*N+PV]+order[OV];
    _positions[ipos*N+PP] = wprice;
}

double dealResult(double price, double *order, int pos, int was_sell){
    // calculate result in money for a deal
    // resulting from an order execution
    // was_sell :
    //     0  was a buy
    //     1  was a sell
    double result = 0;
    double volume = order[OV];
    double start_price = _positions[pos*N+PP];
    if(was_sell)
        result = volume*(start_price-price)*_tick_value-_order_cost*2;
    else
        result = volume*(price-start_price)*_tick_value-_order_cost*2;
    return result;
}


void removePosition(int pos){
    // remove and rearrange book positions
    // if the book has only one position nothing needs to be done
    if(_ipos > 1) // if there are more position copy the last one to fill this gap
        memcpy(&_positions[pos*N], &_positions[(_ipos-1)*N], sizeof(double)*N);
    // less one position
    _ipos--;
}

//@jit(nopython=True)
void removeOrder(int i){
    // remove and rearrange book orders
    // if the book has only one order nothing needs to be done
    if(_iorder > 1) // if there are more orders copy the last one to fill this gap
        memcpy(&_orders[i*N], &_orders[(_iorder-1)*N], sizeof(double)*N);
    // less one position
    _iorder--;
}

// assume infinite money is easier since we are targeting derivatives
// adjust of money is done only when closing positions
//     """
//     Possible outcomes (execution) of an order in codes:

//     0. new position buy
//     1. new position sell
//     2. increase a buy
//     3. increase a sell
//     4. decrease a buy
//     5. decrease a sell
//     6. close a buy
//     7. close a sell
//     --------------------------
//     8. reverse a buy = 6. + 1.
//     9. reverse a sell = 7. + 0.
void executeOrder(double time, double price, double *order,
        int code, int pos){
    double result = 0; // profit or loss of deal
    double reverse_volume=0;
    switch(code){
        case 0: case 1: // new position
            newPosition(time, price, order, code-0);
            memcpy(&_deals_history[_ideals*N], &_positions[_ipos*N], sizeof(double)*N);
            _deals_history[_ideals*N+DT] = time;
            _deals_history[_ideals*N+DV] = order[OV];
            _deals_history[_ideals*N+DR] = result;
            _deals_history[_ideals*N+DE] = Deal_Entry_In;
            _ipos++;
            _ideals++;
            break;
        case 2: case 3: // increasing hand
            // modifying existing position
            updatePositionTime(pos, time);
            updatePositionStops(pos, order);
            updatePositionPrice(pos, order, price);
            _positions[pos*N+PV] += order[OV];
            memcpy(&_deals_history[_ideals*N], &_positions[pos*N], sizeof(double)*N);
            _deals_history[_ideals*N+DT] = time;
            _deals_history[_ideals*N+DV] = order[OV];
            _deals_history[_ideals*N+DR] = result;
            _deals_history[_ideals*N+DE] = Deal_Entry_In;
            _ideals++;
            break;
        case 4: case 5: // decrease a position
            result = dealResult(price, order, pos, (code%6));
            _money += result;
            updatePositionTime(pos, time);
            _positions[pos*N+PV] -= order[OV]; // decrease volume
            memcpy(&_deals_history[_ideals*N], &_positions[pos*N], sizeof(double)*N);
            _deals_history[_ideals*N+DT] = time;
            _deals_history[_ideals*N+DV] = order[OV];
            _deals_history[_ideals*N+DR] = result;
            _deals_history[_ideals*N+DE] = Deal_Entry_Out;
            _ideals++;
            break;
        case 6: case 7: // close a position
            result = dealResult(price, order, pos, (code%8));
            _money += result;
            updatePositionTime(pos, time);
            memcpy(&_deals_history[_ideals*N], &_positions[pos*N], sizeof(double)*N);
            _deals_history[_ideals*N+DT] = time;
            _deals_history[_ideals*N+DV] = order[OV];
            _deals_history[_ideals*N+DR] = result;
            _deals_history[_ideals*N+DE] = Deal_Entry_Out;
            _ideals++;
            removePosition(pos);
            break;
        case 8: // code 8 reverse a buy
            reverse_volume = order[OV]-_positions[pos*N+PV];
            order[OV] = _positions[pos*N+PV];
            executeOrder(time, price, order, 6, pos); // close
            order[OV] = reverse_volume;
            executeOrder(time, price, order, 1, -1); // sell-in
            break;
        case 9:// code 9 reverse a sell
            reverse_volume = order[OV]-_positions[pos*N+PV];
            order[OV] = _positions[pos*N+PV];
            executeOrder(time, price, order, 7, pos); // close
            order[OV] = reverse_volume;
            executeOrder(time, price, order, 0, -1); // buy-in
            break;
    }
}

void processOrder(double *tick, double *order, int pos){
    //does:
    //1. create a new position
    //2. modify/close an existing position
    //3. future:
    //    modify an existing order
    double time = tick[0];
    double price = tick[1];
    // replace buystop and sellstop by buy and sell for a exec_kind
    double difvolume, exec_code = -1;
    int exec_kind = ((int) order[OK] == Order_Kind_BuyStop ||
      (int) order[OK] == Order_Kind_BuyLimit )? Order_Kind_Buy  : (int) order[OK];
    exec_kind = ( exec_kind == Order_Kind_SellStop ||
      exec_kind == Order_Kind_SellLimit)? Order_Kind_Sell : exec_kind;

	if(pos < 0){ // new position
        exec_kind = 0 + exec_kind;
        executeOrder(time, price, order, exec_kind, -1);
    }
    else{ //modify/close an existing position
      	if(pos >= _ipos ) // not a valid position
      		return;
  		 // modify/close existing position
        if(_positions[pos*N+PK] == exec_kind){ // same direction
            //exec_code = 2 if exec_kind == Order_Kind_Buy else 3
            exec_kind = 2 + exec_kind;
            executeOrder(time, price, order, exec_kind, pos);
        }
        else {
            if(( ((int) _positions[pos*N+PK]) == Position_Kind_Buy && exec_kind == Order_Kind_Sell) || // was bought and now is selling
             ( ((int) _positions[pos*N+PK]) == Position_Kind_Sell && exec_kind == Order_Kind_Buy)){ // was selling and now is buying
                difvolume = _positions[pos*N+PV] - order[OV];
                difvolume = (difvolume>0)? 1: (difvolume==0)? 0: -1;
                switch((int) difvolume){
                    case 0: // closing
                    exec_kind = 6 + _positions[pos*N+PK];
                    executeOrder(time, price, order, exec_kind, pos);
                    break;
                    case 1: //  decreasing position
                    exec_kind = 4 + _positions[pos*N+PK];
                    executeOrder(time, price, order, exec_kind, pos);
                    break;
                    case -1: // reversing position
                    exec_kind = 8 + _positions[pos*N+PK];
                    executeOrder(time, price, order, exec_kind, pos);
                    break;
                }
            }
            //order[OK] == Order_Kind_Change: // change stops
      }
    }
}

int insideLimits(double value, double base, double deviation){
    if (value < (base + deviation) &&
        value > (base - deviation))
        return 1;
    return 0;
}

void evaluateOrders(double *tick){
        // Flux of an order:
        // Evaluate -> Process -> Execute
        double deviation;
        double price = tick[1];

        for(int i=0; i<_iorder; i++){
            deviation = _orders[i*N+DI]; // acceptable tolerance
            switch ((int) _orders[i*N+OK]){

              case  Order_Kind_Buy: case Order_Kind_Sell:  // buy or sell
                if(insideLimits(_orders[i*N+OP], price, deviation)){
                  processOrder(tick, &_orders[i*N], (int) _orders[i*N+TK]);
                  removeOrder(i);
                }
              break;
              case Order_Kind_Change: // change stops
                processOrder(tick, &_orders[i*N], (int) _orders[i*N+TK]);
                removeOrder(i);
              break;
              // pending orders
              case  Order_Kind_BuyStop:
                // buy stop order triggered going up
                if (price >= _orders[i*N+OP]
                    && insideLimits(_orders[i*N+OP], price, deviation) ){
                      processOrder(tick, &_orders[i*N], (int) _orders[i*N+TK]);
                      removeOrder(i);
                  }
              break;
              case Order_Kind_SellStop:
                // sell stop order triggered going down
                if(price <= _orders[i*N+OP]
                  && insideLimits(_orders[i*N+OP], price, deviation)){
                    processOrder(tick, &_orders[i*N], (int) _orders[i*N+TK]);
                    removeOrder(i);
                  }
              break;
              case Order_Kind_BuyLimit:
              // buy limit  order triggered going down
                if(price <= _orders[i*N+OP]
                  && insideLimits(_orders[i*N+OP], price, deviation)){
                  processOrder(tick, &_orders[i*N], (int) _orders[i*N+TK]);
                  removeOrder(i);
                }
              break;
              case Order_Kind_SellLimit:
              // sell limit  order triggered going up
                if(price >= _orders[i*N+OP]
                  && insideLimits(_orders[i*N+OP], price, deviation)){
                  processOrder(tick, &_orders[i*N], (int) _orders[i*N+TK]);
                  removeOrder(i);
                }
              break;
              default: // not a valid order
              break;
            }
        }
}


// need to implement Order_Kind_BuyStop Order_Kind_BuyLimit etc.
// maybe should implement position direction
void evaluateStops(double *tick){
    double price = tick[1];
    for(int i=0; i<_ipos; i++){
        if( (int) _positions[i*N+PK] == Position_Kind_Buy){
            if( _positions[i*N+SL] != -1 &&
               price <= _positions[i*N+SL]){
                // deviation high to make sure it`s executed
                sendOrder(Order_Kind_Sell, price, _positions[i*N+PV], -1, -1,
                          1000, i, Order_Source_Stop_Loss);
            }
            else
            if( _positions[i*N+TP] != -1 &&
                price >= _positions[i*N+TP])
                sendOrder(Order_Kind_Sell, price, _positions[i*N+PV], -1, -1,
                          1000, i, Order_Source_Take_Profit);
        }
        else { // sell position
            if( _positions[i*N+SL] != -1 &&
               price >= _positions[i*N+SL]){
                // deviation high to make sure it`s executed
                sendOrder(Order_Kind_Buy, price, _positions[i*N+PV], -1, -1,
                          1000, i, Order_Source_Stop_Loss);
            }
            if( _positions[i*N+TP] != -1 &&
                price <= _positions[i*N+TP])
                // deviation high to make sure it`s executed
                sendOrder(Order_Kind_Buy, price, _positions[i*N+PV], -1, -1,
                          1000, i, Order_Source_Take_Profit);
        }
    }
}


double positionsValue(double *tick){
    double price = tick[1];
    double volume, start_price, value = 0;
    // possible parallel
    for(int i=0; i<_ipos;i++){
        volume = _positions[i*N+PV];
        start_price = _positions[i*N+PP];
        if( (int) _positions[i*N+PK] == Position_Kind_Buy)
            value += volume*(price-start_price)*_tick_value-_order_cost*2;
        else
            value += volume*(start_price-price)*_tick_value-_order_cost*2;
    }
    return value;
}

void Simulator(double init_money, double tick_value, double order_cost,
        double *ticks, int nticks, double *pmoney, void (*onTick)(double*)){
    // first tick will be ignore, used for buy/sell stop orders
    // ticks 4 columns - datetime, price, volume, real-volume
    // pmoney is just same size as nticks for recording money progress
    _ideals = 0;
    _iorder = 0;
    _ipos = 0;
    _money = init_money;
    _tick_value = tick_value;
    _order_cost = order_cost;

    for(int i=0; i<nticks; i++){
        evaluateOrders(&ticks[i*4]);
        evaluateStops(&ticks[i*4]);
        pmoney[i] = _money + positionsValue(&ticks[i*4]); // hedge first - net mode future
        onTick(&ticks[i*4]);
    }

}
