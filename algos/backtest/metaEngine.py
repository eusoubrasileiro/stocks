import numpy as np
from numba import jit, njit, prange
import random

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
DK=14 # deal kind - same from order Order_Kind_Buy and Order_Kind_Sell
DE=15 # deal entry - in (1) or out(0) or inout reverse (2)

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
Deal_Entry_InOut = 2

booksdict = {"EP" : 0, "QT" : 1, "DR" : 2, "TP": 3, "SL" : 4,
                      "OT" : 5, "CP" : 6, "SS" : 7, "CT" : 8, "MB" : 9, "DY": 10}

_iorder = 0 # number of pending orders
_orders = np.ones((1000, 8))*np.nan  # pending waiting for entry
_ipos = 0 # number of open positions
_positions = np.ones((1000, 8))*np.nan
_ideals = 0 # number of executed deals
_deals_history = np.ones((1000, 1))*np.nan
_money = 1000 # money on pouch
_tick_value = 0.02 # ibov mini-contratc

#######################################################################
########################## level 4 ####################################

@jit(nopython=True)
def sendOrder(kind, price=-1,  volume=-1,
              sloss=-1, tprofit=-1, deviation=-1,
              ticket=-1, source=Order_Source_Client):
    _orders[iorder, :] = kind, price, volume, \
        sloss, tprofit, deviation, ticket, source
    _iorder+=1

def newPosition(time, order):
    _positions[_ipos, :] =  order
    _positions[_ipos, PT] =  time # time position opened
    updatePositionStops(_ipos, order)

def updatePositionTime(ipos, time):
    _positions[ipos, PC] = time

def updatePositionStops(ipos, order):
    position = _positions[ipos]
    _positions[ipos, SL] = position[SL] if order[SL] == -1 else order[SL]
    _positions[ipos, TP] = position[TP] if order[TP] == -1 else order[TP]

def updatePositionPrice(ipos, order, price):
    position = _positions[ipos]
    # calculate and update average price of position (v0*p0+v1*p1)/(v0+v1)
    wprice = position[VV]*position[PP]+order[VV]*price
    wprice /= position[VV]+order[VV]
    _positions[ipos, PP] = wprice

# def decreasePosition(ipos, order, price):
#     """calculate result in money for this deal"""
#     # buy closed with sell
#     #if
#     signal = 1 if _positions[ipos, OK] == Order_Kind_Buy else -1
#     # sell closed with buy

#     # sell increase or buy increase

# assume infinite money is easier since we are targeting derivatives
# adjust of money is done only when closing positions
def executeOrder(time, price, order, code, pos=-1):

    result = 0
    if code==1: # new position
        newPosition(time, order)
        _deals_history[_ideals, :] = _positions[_ipos]
        _deals_history[_ideals, -6:] = time, order[VV], result, kind, \
        entry, source
        _ipos += 1
        _ideals += 1
    else: # modifying existing position
        position = _positions[pos]
        if ((kind == Deal_Kind_Buy and entry == Deal_Entry_In) or
            (kind == Deal_Kind_Sell and entry == Deal_Entry_In)): # increasing position
            _deals_history[_ideals, :] = _positions[pos]
            updatePositionTime(pos, time)
            updatePositionStops(pos, order)
            updatePositionPrice(pos, order, price)
            _positions[pos, VV] += order[VV]
            _deals_history[_ideals, -6:] = time, order[VV], result, kind, \
                entry, source
            _ideals += 1
        elif((kind == Deal_Kind_Buy or kind == Deal_Kind_Sell) and
            (entry == Deal_Entry_Out)): # decreasing or closing position
                volume_balance  = _positions[pos, VV] - order[OV]
                if(order[OV] < _positions[pos, VV]): # decreasing position
                    result = price*order[OV]
                    _positions[ipos, VV] -= order[VV] # increase volume
                    result = -price*order[OV]

"""
Possible outcomes (execution) of an order in codes:

0. new position buy
1. new position sell
2. increase a buy
3. increase a sell
4. decrease a buy
5. decrease a sell
6. close a buy
7. close a sell
--------------------------
8. reverse a buy = 5. + 2.
9. reverse a sell = 6. + 1.

"""
@jit(nopython=True)
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

"""
Flux of an order:

Evaluate -> Process -> Execute
"""
@jit(nopython=True)
def evaluateOrders(tick, ptick):
    time, price = tick[:2]
    for i in range(_iorder):
        if ((_orders[i, OT] == Order_Type_Buy) or # buy or sell
            (_orders[iorder, OT] == Order_Type_Sell)):
            deviation = _orders[i, DV] # acceptable tolerance
            if (_orders[i, OP] < (price + deviation) or
                _orders[i, OP] > (price - deviation)):
                processOrder(tick, _orders[i], _orders[i, TK])
            removeOrder(i)
        elif _orders[iorder, OT] == Order_Type_Change: # change stops
            processOrder(tick, _orders[i], _orders[i, TK])
            removeOrder(i)
        else: # pending orders
            pprice = ptick[1] # previous tick price
            if order[OK] == Order_Kind_BuyStop:
                if(_orders[i, OP] > pprice and
                   _orders[i, OP] <= price): # buy stop order triggered
                    deviation = _orders[i, DV]
                    if (_orders[i, OP] < (price + deviation) or
                        _orders[i, OP] > (price - deviation)):
                        processOrder(tick, _orders[i], _orders[i, TK])
                        removeOrder(i)
            elif order[OK] == Order_Kind_SellStop:
                if(_orders[i, OP] < pprice and
                   _orders[i, OP] >= price): # sell stop order triggered
                    deviation = _orders[i, DV]
                    if (_orders[i, OP] < (price + deviation) or
                        _orders[i, OP] > (price - deviation)):
                        processOrder(tick, _orders[i], _orders[i, TK])
                        removeOrder(i)

@jit(nopython=True, parallel=False)
def Simulator(ticks, onTick):
    # copy first as previous tick for buy/sell stop orders
    ticks = np.concatenate(ticks[0], ticks)
    nticks = ticks.shape[0]
    for i in range(1, nticks):
        # second arg previous tick for buy/sell stop orders
        evaluateOrders(ticks[i], ticks[i-1])
        checkStops(ticks[i])
        updateMoney(ticks[i]) # hedge first - net mode future
        onTick(ticks[i])
