from numba import jit, njit, prange
# hedge mode buy default

# orders
OK=0 # order Kind 0-sell, 1-buy, 2-change stops pendings : 3-buy stop, 4-sell stop, 5-buy limit, 6-sell limit
OP=1 # order price
VV=2 # order or position volume or deal volume
SL=3 # stop loss maybe -1 not set
TP=4 # take profit maybe -1 not set
DV=5 # devitation accepted maybe -1 not set
TK=6 # ticket position to operate on (decrese-increase volume etc) maybe -1 not set
OS=7 # source of order client or some event
# positions
PP=8 # position weighted price
PT=9 # position time openned - needed by algos closing by time
PC=10 # last time changed
# deals
DT=11 # deal time
DV=12 # deal volume
DP=13 # deal result + money back , - money taken or 0 nothing
DE=14 # deal entry - in (1) or out(0) or inout reverse (2)

Order_Kind_Buy = 0
Order_Kind_Sell = 1
Order_Kind_Change = 2 # change stop loss
Order_Kind_BuyStop = 4
Order_Kind_SellStop = 5
Order_Kind_BuyLimit = 6
Order_Kind_SellLimit = 7
Order_Source_Client = 0 # default
Order_Source_Stop_Loss = 1
Order_Source_Take_Profit = 2
Deal_Entry_In = 0
Deal_Entry_Out = 1

# booksdict = {"EP" : 0, "QT" : 1, "DR" : 2, "TP": 3, "SL" : 4,
#                       "OK" : 5, "CP" : 6, "SS" : 7, "CT" : 8, "MB" : 9, "DY": 10}


#@jit(nopython=True)
def Simulator(ticks, onTick):

    _iorder = 0 # number of pending orders
    _orders = np.ones((100, 14))*np.nan  # pending waiting for entry
    _ipos = 0 # number of open positions
    _positions = np.ones((100, 14))*np.nan
    _ideals = 0 # number of executed deals
    _deals_history = np.ones((90000, 14))*np.nan
    _money = 1000 # money on pouch
    _tick_value = 0.02 # ibov mini-contratc
    _order_cost = 0.0 # order cost in $money


    #@jit(nopython=True)
    def sendOrder(kind, price=-1,  volume=-1,
                  sloss=-1, tprofit=-1, deviation=-1,
                  ticket=-1, source=Order_Source_Client):
        _orders[_iorder, :] = kind, price, volume, \
            sloss, tprofit, deviation, ticket, source
        _iorder+=1

    #@jit(nopython=True)
    def updatePositionStops(ipos, order):
        position = _positions[ipos]
        _positions[ipos, SL] = position[SL] if order[SL] == -1 else order[SL]
        _positions[ipos, TP] = position[TP] if order[TP] == -1 else order[TP]

    #@jit(nopython=True)
    def newPosition(time, order):
        _positions[_ipos, :] =  order
        _positions[_ipos, PT] =  time # time position opened
        updatePositionStops(_ipos, order)

    #@jit(nopython=True)
    def updatePositionTime(ipos, time):
        _positions[ipos, PC] = time


    #@jit(nopython=True)
    def updatePositionPrice(ipos, order, price):
        position = _positions[ipos]
        # calculate and update average price of position (v0*p0+v1*p1)/(v0+v1)
        wprice = position[VV]*position[PP]+order[VV]*price
        wprice /= position[VV]+order[VV]
        _positions[ipos, PP] = wprice

    #@jit(nopython=True)
    def dealResult(price, order, pos, was_sell=0):
        """
        calculate result in money for a deal
        resulting from an order execution
        was_sell :
            0  was a buy
            1  was a sell
        """
        result = 0
        position = _positions[pos]
        volume = order[VV]
        start_price = _positions[PP]
        if was_sell:
            result = volume*(start_price-price)*_tick_value-_order_cost*2
        else:
            result = volume*(price-start_price)*_tick_value-_order_cost*2
        return result

    #@jit(nopython=True)
    def removePosition(pos):
        # remove and rearrange book positions
        # if the book has only one position nothing needs to be done
        if _ipos > 1: # if there are more position copy the last one to fill this gap
            _positions[pos, :] = _positions[_ipos-1, :]
        # less one position
        _ipos -= 1

    #@jit(nopython=True)
    def removeOrder(i):
        # remove and rearrange book orders
        # if the book has only one order nothing needs to be done
        if _ipos > 1: # if there are more orders copy the last one to fill this gap
            _orders[i, :] = _orders[_iorder-1, :]
        # less one position
        _iorder -= 1

    # assume infinite money is easier since we are targeting derivatives
    # adjust of money is done only when closing positions
    #     """
    #     Possible outcomes (execution) of an order in codes:

    #     0. new position buy
    #     1. new position sell
    #     2. increase a buy
    #     3. increase a sell
    #     4. decrease a buy
    #     5. decrease a sell
    #     6. close a buy
    #     7. close a sell
    #     --------------------------
    #     8. reverse a buy = 6. + 1.
    #     9. reverse a sell = 7. + 0.

    #     """
    #@jit(nopython=True)
    def executeOrder(time, price, order, code, pos=-1):

        result = 0 # profit or loss of deal
        if code < 2 and code > -1: # new position
            newPosition(time, order)
            _deals_history[_ideals, :] = _positions[_ipos]
            _deals_history[_ideals, -4:] = time, order[VV], \
                result, Deal_Entry_In
            _ipos += 1
            _ideals += 1
        elif code < 4 and code > 1: # increasing hand
            # modifying existing position
            position = _positions[pos]
            _deals_history[_ideals, :] = _positions[pos]
            updatePositionTime(pos, time)
            updatePositionStops(pos, order)
            updatePositionPrice(pos, order, price)
            _positions[pos, VV] += order[VV]
            _deals_history[_ideals, -4:] = time, order[VV], \
                result, Deal_Entry_In
            _ideals += 1
        elif code < 6 and code > 3: # decrease a position
            result = dealResult(price, order, pos, (code%6))
            money += result
            updatePositionTime(pos, time)
            _positions[ipos, VV] -= order[VV] # decrease volume
            _deals_history[_ideals, -4:] = time, order[VV], \
                result, Deal_Entry_Out
            _ideals += 1
        elif code < 8 and code > 5: # close a position
            result = dealResult(price, order, pos, (code%8))
            money += result
            updatePositionTime(pos, time)
            removePosition(pos)
            _deals_history[_ideals, -4:] = time, _order[VV], \
                result, Deal_Entry_Out
            _ideals += 1
        elif code < 9: # code 8 reverse a buy
            order_volume = order[VV]-_positions[VV]
            order[VV] = _positions[VV]
            executeOrder(time, price, order, 6)
            order[VV] = order_volume
            executeOrder(time, price, order, 1)
        else: # code 9 reverse a sell
            order_volume = order[VV]-_positions[VV]
            order[VV] = _positions[VV]
            executeOrder(time, price, order, 7)
            order[VV] = order_volume
            executeOrder(time, price, order, 0)

    #@jit(nopython=True)
    def processOrder(tick, order, pos):
        """
        does:
        1. create a new position
        2. modify/close an existing position
        3. future:
            modify an existing order
        """
        time, price = tick[:2]
        # replace buystop and sellstop by buy and sell for a execution kind
        exec_kind = Order_Kind_Buy if order[OK] == Order_Kind_BuyStop else order[OK]
        exec_kind = Order_Kind_Sell if exec_kind == Order_Kind_SellStop else exec_kind

        if pos == -1: # new position
            exec_code = 0 + exec_kind
            executeOrder(time, price, order, exec_code)
        else: # modify/close existing position
            position = _positions[pos]
            if position[OK] == exec_kind: # same direction
                #exec_code = 2 if exec_kind == Order_Kind_Buy else 3
                exec_code = 2 + exec_kind
                executeOrder(time, price, order, exec_code, pos)
            elif((position[OK] == Order_Kind_Buy and exec_kind == Order_Kind_Sell) or # was bought and now is selling
                 (position[OK] == Order_Kind_Sell and exec_kind == Order_Kind_Buy)): # was selling and now is buying
                difvolume = position[VV] - order[VV]
                if difvolume == 0: # closing
                    exec_code = 6 + position[OK]
                    executeOrder(time, price, order, exec_code, pos)
                elif difvolume > 0: #  decreasing position
                    exec_code = 4 + position[OK]
                    executeOrder(time, price, order, exec_code, pos)
                elif difvolume < 0: # reversing position
                    exec_code = 8 + position[OK]
                    executeOrder(time, price, order, exec_code, pos)
            elif order[OK] == Order_Kind_Change: # change stops
                pass

    #@jit(nopython=True)
    def evaluateOrders(tick, ptick):
        """
        Flux of an order:

        Evaluate -> Process -> Execute
        """
        time, price = tick[:2]
        for i in range(_iorder):
            if ((_orders[i, OK] == Order_Kind_Buy) or # buy or sell
                (_orders[i, OK] == Order_Kind_Sell)):
                deviation = _orders[i, DV] # acceptable tolerance
                if (_orders[i, OP] < (price + deviation) or
                    _orders[i, OP] > (price - deviation)):
                    processOrder(tick, _orders[i], _orders[i, TK])
                    removeOrder(i)
            elif _orders[i, OK] == Order_Kind_Change: # change stops
                processOrder(tick, _orders[i], _orders[i, TK])
                removeOrder(i)
            else: # pending orders
                pprice = ptick[1] # previous tick price
                if _orders[i, OK] == Order_Kind_BuyStop:
                    if(_orders[i, OP] > pprice and
                       _orders[i, OP] <= price): # buy stop order triggered
                        deviation = _orders[i, DV]
                        if (_orders[i, OP] < (price + deviation) or
                            _orders[i, OP] > (price - deviation)):
                            processOrder(tick, _orders[i], _orders[i, TK])
                            removeOrder(i)
                elif _orders[i, OK] == Order_Kind_SellStop:
                    if(_orders[i, OP] < pprice and
                       _orders[i, OP] >= price): # sell stop order triggered
                        deviation = _orders[i, DV]
                        if (_orders[i, OP] < (price + deviation) or
                            _orders[i, OP] > (price - deviation)):
                            processOrder(tick, _orders[i], _orders[i, TK])
                            removeOrder(i)

    #@jit(nopython=True)
    def evaluateStops(tick):
        price = tick[1]
        for i in prange(_ipos):
            if _positions[i, OK] == Order_Kind_Buy:
                if( _positions[i, SL] != -1 and
                   price <= _positions[i, SL]):
                    # deviation high to make sure it`s executed
                    sendOrder(Order_Kind_Sell, price, _positions[i, VV],
                              deviation=1000, ticket=i,
                              source=Order_Source_Stop_Loss)
                if( _positions[i, TP] != -1 and
                    price >= _positions[i, TP]):
                    # deviation high to make sure it`s executed
                    sendOrder(Order_Kind_Sell, price, _positions[i, VV],
                              deviation=1000, ticket=i,
                              source=Order_Source_Take_Profit)
            else: # sell position
                if( _positions[i, SL] != -1 and
                   price >= _positions[i, SL]):
                    # deviation high to make sure it`s executed
                    sendOrder(Order_Kind_Buy, price, _positions[i, VV],
                              deviation=1000, ticket=i,
                              source=Order_Source_Stop_Loss)
                if( _positions[i, TP] != -1 and
                    price <= _positions[i, TP]):
                    # deviation high to make sure it`s executed
                    sendOrder(Order_Kind_Buy, price, _positions[i, VV],
                              deviation=1000, ticket=i,
                              source=Order_Source_Take_Profit)


    #@jit(nopython=True, parallel=True)
    def positionsValue(tick):
        price = tick[1]
        value = 0
        for i in prange(_ipos):
            position = _positions[pos]
            volume = _positions[VV]
            start_price = _positions[PP]
            if _positions[i, OK] == Order_Kind_Buy:
                value += volume*(start_price-price)*_tick_value-_order_cost*2
            else:
                value += volume*(price-start_price)*_tick_value-_order_cost*2
        return value


    # first tick will be ignore, used for buy/sell stop orders
    nticks = ticks.shape[0]
    pmoney = np.zeros(nticks)
    pmoney[0] = _money
    for i in range(1, 10000):
        # second arg previous tick for buy/sell stop orders
        evaluateOrders(ticks[i], ticks[i-1])
        evaluateStops(ticks[i])
        pmoney[i] = _money + positionsValue(ticks[i]) # hedge first - net mode future
        onTick(ticks[i])
    return pmoney
