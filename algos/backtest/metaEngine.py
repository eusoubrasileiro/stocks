import numpy as np
from numpy.ctypeslib import ndpointer
import os
import ctypes as c
from numba import jit, cfunc, types, carray
# hedge mode buy default

if os.name == 'nt':
    # WINFUNCTYPE for stdcall (standard call is c++ normally)
    _lib = c.cdll.LoadLibrary('./metaengine.dll')
else:
    _lib = c.cdll.LoadLibrary('./metaengine.so')

Order_Kind_Buy = 0.
Order_Kind_Sell = 1.
Order_Kind_Change = 2. # change stop loss
Order_Kind_BuyStop = 4.
Order_Kind_SellStop = 5.
Order_Kind_BuyLimit = 6.
Order_Kind_SellLimit = 7.
Order_Source_Client = 0. # default
Order_Source_Stop_Loss = 1.
Order_Source_Take_Profit = 2.
Deal_Entry_In = 0.
Deal_Entry_Out = 1.

#### Ctypes + Numba interface Awesome!!
### https://numba.pydata.org/numba-doc/dev/user/cfunc.html

_simulator = _lib.Simulator
_simulator.argtypes = [c.c_double, c.c_double, c.c_double,
                          c.c_void_p, c.c_int32, c.c_void_p, c.c_void_p]
_simulator.restype = None

# double init_money, double tick_value, double order_cost,
#        double *ticks, int nticks, double *pmoney, void (*onTick)(double*))
def Simulator(ticks, onTick, money=5000, tick_value=0.2, order_cost=4.0, cnumba=True):
    nticks = len(ticks)
    pmoney = np.empty(nticks, dtype=np.float64)
    if cnumba: # Numba callback 1000x faster, but call back print BUG
        cOnTick = onTick.ctypes
    else:
        cfunc = c.CFUNCTYPE(None, c.POINTER(c.c_double))
        cOnTick = cfunc(onTick)
    _simulator(money, tick_value, order_cost, ticks.ctypes.data, len(ticks),
                  pmoney.ctypes.data, cOnTick)
    return pmoney

_sendorder = _lib.sendOrder
_sendorder.argtypes = [c.c_double]*8
_sendorder.restype = None

_norders = _lib.nOrders
_norders.restype = c.c_int

_npositions = _lib.nPositions
_npositions.restype = c.c_int

_ndeals = _lib.nDeals
_ndeals.restype = c.c_int

_orders = _lib.Orders
_orders.argtypes = [c.POINTER(c.c_double)]
_orders.restype = None

_positions = _lib.Positions
_positions.argtypes = [c.POINTER(c.c_double)]
_positions.restype = None

_deals = _lib.Deals
_deals.argtypes = [c.POINTER(c.c_double)]
_deals.restype = None
#lib.function.restype = ndpointer(dtype=ctypes.c_double, shape=(10000,15))

def __sendorder(kind, price=-1.,  volume=-1.,
              sloss=-1., tprofit=-1., deviation=-1.,
              ticket=-1., source=Order_Source_Client):
    _sendorder(kind, price, volume, sloss, tprofit, deviation,
                 ticket, source)

def __ndeals():
    return _ndeals()

def __npositions():
    return _npositions()

def __norders():
    return _norders()

def __deals():
    deals = c.c_double*150000
    deals = deals() # c_double_Array_150000
    _deals(deals)
    # deals = (c.c_double*150000).from_address(
    #     c.addressof(deals.contents))
    return np.ctypeslib.as_array(deals).reshape(15,10000)

def __positions():
    positions = c.POINTER(c.c_double) # this is a real c pointer
    _positions(positions)
    positions = (c.c_double*1500).from_address(
        c.addressof(positions.contents))
    return np.ctypeslib.as_array(positions, shape=(15,100))

def __orders():
    orders = c.POINTER(c.c_double)
    _orders(orders)
    orders = (c.c_double*1500).from_address(
        c.addressof(orders.contents))
    return np.ctypeslib.as_array(orders, shape=(15,100))

sendOrder = c.CFUNCTYPE(None, c.c_double, c.c_double, c.c_double, c.c_double, \
                     c.c_double, c.c_double, c.c_double, c.c_double)(__sendorder)

nDeals = c.CFUNCTYPE(c.c_int32)(__ndeals)

nPositions = c.CFUNCTYPE(c.c_int32)(__npositions)

nOrders = c.CFUNCTYPE(c.c_int32)(__norders)

Deals = __deals
#Deals = c.CFUNCTYPE(None, ndpointer(c.c_double, shape=(15, 10000)))(__deals)

Positions = __positions
# Positions = c.CFUNCTYPE(c.c_void_p, c.c_int32)(__positions)

Orders = __orders
# Orders = c.CFUNCTYPE(c.c_void_p, c.c_int32)(__orders)

# orders
# OK=0 # order Kind 0-sell, 1-buy, 2-change stops pendings : 3-buy stop, 4-sell stop, 5-buy limit, 6-sell limit
# OP=1 # order price
# VV=2 # order or position volume or deal volume
# SL=3 # stop loss maybe -1 not set
# TP=4 # take profit maybe -1 not set
# DI=5 # devitation accepted maybe -1 not set
# TK=6 # ticket position to operate on (decrese-increase volume etc) maybe -1 not set
# OS=7 # source of order client or some event
# # positions
# PP=8 # position weighted price
# PT=9 # position time openned - needed by algos closing by time
# PC=10 # last time changed
# # deals
# DT=11 # deal time
# DV=12 # deal volume
# DR=13 # deal result + money back , - money taken or 0 nothing
# DE=14 # deal entry - in (1) or out(0) or inout reverse (2)

bookvalues = "OK OP VV SL TP DI TK OS PP PT PC DT DV DR DE".split(' ')
N = len(bookvalues)

#### Call-back function jitted with **Numba** > 1000x faster than non-jitted version
### All-ticks 9 seconds to 55 ms
### Altough:
###  AssertionError: Keyword arguments are not supported, yet
# c_sig = types.void(types.CPointer(types.double))
#
# @cfunc(c_sig)
# def onTickFunc(tick):
#     #tick = np.ctypeslib.as_array(tick, shape=(4,))
#     tick = carray(tick, (4))
#     time, price = tick[:2]
#     if time == 1388657280:
#         sendOrder(Order_Kind_Buy, price, 3., -1., -1., 10, -1, 0)
#     if time == 1388663280:
#         sendOrder(Order_Kind_Sell, price, 6, -1, -1, 10, -1, 0)
#     if time > 1388663280+60*10 and time < 1388663280+60*11: # 60 minutes after
#         sendOrder(Order_Kind_Buy, price, 6, -1, -1, 10, 1, 0)

# __all__ = [ Simulator, sendOrder,
#            nDeals, nPositions, nOrders ]
