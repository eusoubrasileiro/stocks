#include <stdio.h>
#include <string.h>
#include "metaEngine.h"

void sendOrder(double kind, double price, double volume,
              double sloss, double tprofit, double deviation,
              double ticket, double source){
    _orders[_iorder*N+0] = kind;
    _orders[_iorder*N+1] = price;
    _orders[_iorder*N+2] = volume;
    _orders[_iorder*N+3] = sloss;
    _orders[_iorder*N+4] = tprofit;
    _orders[_iorder*N+5] = deviation;
    _orders[_iorder*N+6] = ticket;
    _orders[_iorder*N+7] = source;
    _iorder++;
}

void updatePositionStops(int ipos, double *order){
    _positions[ipos*N+SL] = ((int) order[SL] == -1)? _positions[ipos*N+SL] : order[SL];
    _positions[ipos*N+TP] = ((int) order[TP] == -1)? _positions[ipos*N+TP] : order[TP];
}

void newPosition(double time, double price, double *order){
    memcpy(&_positions[_ipos*N], order, sizeof(double)*7);
    _positions[_ipos*N+PT] = time; // time position opened
    _positions[_ipos*N+PP] = price; 
    updatePositionStops(_ipos, order);
}

void updatePositionTime(int ipos, double time){
    _positions[ipos*N+PC] = time;
}

void updatePositionPrice(int ipos, double *order, double price){
    // calculate and update average price of position (v0*p0+v1*p1)/(v0+v1)
    double wprice = _positions[ipos*N+VV]*_positions[ipos*N+PP]+order[VV]*price;
    wprice /= _positions[ipos*N+VV]+order[VV];
    _positions[ipos*N+PP] = wprice;
}

double dealResult(double price, double *order, int pos, int was_sell){
    // calculate result in money for a deal
    // resulting from an order execution
    // was_sell :
    //     0  was a buy
    //     1  was a sell
    double result = 0;
    double volume = order[VV];
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
    double order_volume=0;
    switch(code){
        case 0: case 1: // new position
            newPosition(time, price, order);
            memcpy(&_deals_history[_ideals*N], &_positions[_ipos*N], sizeof(double)*N);
            _deals_history[_ideals*N+DT] = time;
            _deals_history[_ideals*N+DV] = order[VV];
            _deals_history[_ideals*N+DR] = result;
            _deals_history[_ideals*N+DE] = Deal_Entry_In;
            _ipos++;
            _ideals++;
            break;
        case 2: case 3: // increasing hand
            // modifying existing position
            memcpy(&_deals_history[_ideals*N], &_positions[_ipos*N], sizeof(double)*N);
            updatePositionTime(pos, time);
            updatePositionStops(pos, order);
            updatePositionPrice(pos, order, price);
            _positions[pos*N+VV] += order[VV];
            _deals_history[_ideals*N+DT] = time;
            _deals_history[_ideals*N+DV] = order[VV];
            _deals_history[_ideals*N+DR] = result;
            _deals_history[_ideals*N+DE] = Deal_Entry_In;
            _ideals++;
            break;
        case 4: case 5: // decrease a position
            result = dealResult(price, order, pos, (code%6));
            _money += result;
            updatePositionTime(pos, time);
            _positions[pos*N*VV] -= order[VV]; // decrease volume
            _positions[pos*N+VV] += order[VV];
            _deals_history[_ideals*N+DT] = time;
            _deals_history[_ideals*N+DV] = order[VV];
            _deals_history[_ideals*N+DR] = result;
            _deals_history[_ideals*N+DE] = Deal_Entry_Out;
            _ideals++;
            break;
        case 6: case 7: // close a position
            result = dealResult(price, order, pos, (code%8));
            _money += result;
            updatePositionTime(pos, time);
            removePosition(pos);
            _deals_history[_ideals*N+DT] = time;
            _deals_history[_ideals*N+DV] = order[VV];
            _deals_history[_ideals*N+DR] = result;
            _deals_history[_ideals*N+DE] = Deal_Entry_Out;
            _ideals++;
            break;
        case 8: // code 8 reverse a buy
            order_volume = order[VV]-_positions[pos*N+VV];
            order[VV] = _positions[pos*N+VV];
            executeOrder(time, price, order, 6, -1);
            order[VV] = order_volume;
            executeOrder(time, price, order, 1, -1);
            break;
        case 9:// code 9 reverse a sell
            order_volume = order[VV]-_positions[pos*N+VV];
            order[VV] = _positions[pos*N+VV];
            executeOrder(time, price, order, 7, -1);
            order[VV] = order_volume;
            executeOrder(time, price, order, 0, -1);
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
    // replace buystop and sellstop by buy and sell for a execution kind
    double difvolume, exec_code = -1;
    int exec_kind = ((int) order[OK] == Order_Kind_BuyStop)? Order_Kind_Buy  : order[OK];
    exec_kind = ((int) exec_kind == Order_Kind_SellStop)? Order_Kind_Sell : exec_kind;

	if(pos < 0){ // new position
        exec_code = 0 + exec_kind;
        executeOrder(time, price, order, exec_code, -1);
    }
    else{ //modify/close an existing position
    	if(pos >= _ipos ) // not a valid position
    		return;
		 // modify/close existing position
        if(_positions[pos*N+OK] == exec_kind){ // same direction
            //exec_code = 2 if exec_kind == Order_Kind_Buy else 3
            exec_code = 2 + exec_kind;
            executeOrder(time, price, order, exec_code, pos);
        }
        else {
            if(( ((int) _positions[pos*N+OK]) == Order_Kind_Buy && exec_kind == Order_Kind_Sell) || // was bought and now is selling
             ( ((int) _positions[pos*N+OK]) == Order_Kind_Sell && exec_kind == Order_Kind_Buy)){ // was selling and now is buying
                difvolume = _positions[pos*N+VV] - order[VV];
                difvolume = (difvolume>0)? 1: (difvolume==0)? 0: -1;
                switch((int) difvolume){
                    case 0: // closing
                    exec_code = 6 + _positions[pos*N+OK];
                    executeOrder(time, price, order, exec_code, pos);
                    break;
                    case 1: //  decreasing position
                    exec_code = 4 + _positions[pos*N+OK];
                    executeOrder(time, price, order, exec_code, pos);
                    break;
                    case -1: // reversing position
                    exec_code = 8 + _positions[pos*N+OK];
                    executeOrder(time, price, order, exec_code, pos);
                    break;
                }
            }
            //order[OK] == Order_Kind_Change: // change stops
        }
    }
}

void evaluateOrders(double *tick, double *ptick){
        // Flux of an order:
        // Evaluate -> Process -> Execute
        double deviation;
        double pprice, price = tick[1];
        for(int i=0; i<_iorder; i++){
            deviation = _orders[i*N+DI]; // acceptable tolerance
            if(( (int) _orders[i*N+OK] == Order_Kind_Buy) || // buy or sell
                ( (int) _orders[i*N+OK] == Order_Kind_Sell))
                if( (_orders[i*N+OP] < (price + deviation)) ||
                    (_orders[i*N+OP] > (price - deviation))){
                    processOrder(tick, &_orders[i*N], (int) _orders[i*N+TK]);
                    removeOrder(i);
                }
            else
            if( (int) _orders[i*N+OK] == Order_Kind_Change){ // change stops
                processOrder(tick, &_orders[i*N], (int) _orders[i*N+TK]);
                removeOrder(i);
            }
            else { // pending orders
                pprice = ptick[1]; // previous tick price
                if( ((int) _orders[i*N+OK] == Order_Kind_BuyStop) &&
                (_orders[i*N+OP] > pprice) && (_orders[i*N+OP] <= price) ) // buy stop order triggered
                    if (_orders[i*N+OP] < (price + deviation) ||
                        _orders[i*N+OP] > (price - deviation)){
                        processOrder(tick, &_orders[i*N], (int) _orders[i*N+TK]);
                        removeOrder(i);
                    }
                else
                if( ( (int) _orders[i*N+OK] == Order_Kind_SellStop) &&
                    _orders[i*N+OP] < pprice && _orders[i*N+OP] >= price)// sell stop order triggered
                    if (_orders[i*N+OP] < (price + deviation) ||
                        _orders[i*N+OP] > (price - deviation)){
                        processOrder(tick, &_orders[i*N], (int) _orders[i*N+TK]);
                        removeOrder(i);
                    }
            }
        }
}


void evaluateStops(double *tick){
    double price = tick[1];
    for(int i=0; i<_ipos; i++){
        if( (int) _positions[i*N+OK] == Order_Kind_Buy){
            if( _positions[i*N+SL] != -1 &&
               price <= _positions[i*N+SL]){
                // deviation high to make sure it`s executed
                sendOrder(Order_Kind_Sell, price, _positions[i*N+VV], -1, -1,
                          1000, i, Order_Source_Stop_Loss);
            }
            else
            if( _positions[i*N+TP] != -1 &&
                price >= _positions[i*N+TP])
                sendOrder(Order_Kind_Sell, price, _positions[i*N+VV], -1, -1,
                          1000, i, Order_Source_Take_Profit);
        }
        else { // sell position
            if( _positions[i*N+SL] != -1 &&
               price >= _positions[i*N+SL]){
                // deviation high to make sure it`s executed
                sendOrder(Order_Kind_Buy, price, _positions[i*N+VV], -1, -1,
                          1000, i, Order_Source_Stop_Loss);
            }
            if( _positions[i*N+TP] != -1 &&
                price <= _positions[i*N+TP])
                // deviation high to make sure it`s executed
                sendOrder(Order_Kind_Buy, price, _positions[i*N+VV], -1, -1,
                          1000, i, Order_Source_Take_Profit);
        }
    }
}


double positionsValue(double *tick){
    double price = tick[1];
    double volume, start_price, value = 0;
    // possible parallel
    for(int i=0; i<_ipos;i++){
        volume = _positions[i*N+VV];
        start_price = _positions[i*N+PP];
        if( (int) _positions[i*N+OK] == Order_Kind_Buy)
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
    pmoney[0] = _money;
    for(int i=1; i<nticks; i++){
        // second arg previous tick for buy/sell stop orders
        evaluateOrders(&ticks[i*4], &ticks[(i-1)*4]);
        evaluateStops(&ticks[i*4]);
        pmoney[i-1] = _money + positionsValue(&ticks[i*4]); // hedge first - net mode future
        onTick(&ticks[i*4]);
    }
}
