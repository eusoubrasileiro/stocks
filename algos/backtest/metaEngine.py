import numpy as np
import os
import ctypes as c
from numba import jit, cfunc, types, carray

# hedge mode buy default
##### Compile Python Module 'dll' or 'share library'
#### on Windows `mingw` anaconda package must be installed

try:
    if os.name == 'nt': # windows
        os.system('gcc -std=gnu99 -c metaEngine.c -DBUILD_DLL -o metaEngine.o')
        os.system('gcc -shared -o metaengine.dll metaEngine.o -Wl,--out-implib,metaEngine.a')
        # WINFUNCTYPE for stdcall (standard call is c++ normally)
        _lib = c.cdll.LoadLibrary('./metaengine.dll')
    else:
        os.system('gcc -c -fPIC metaEngine.c -o metaEngine.o')
        os.system('gcc -shared -Wl,-soname,metaEngine.so -o metaengine.so metaEngine.o')
        _lib = c.cdll.LoadLibrary('./metaengine.so')
except:
    print("Problem compiling or loading the C dynamic library")
    print("Make sure you have gcc on Linux or `mingw` anaconda package on Windows")
    exit()

cMetaEngineLib = _lib

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

#### Global variables from DLL
### AMAZING C pointers with cast ! + numpy arrays with fixed memory loc.

_pointer_orders = c.cast(_lib._orders, c.POINTER(c.c_double))
Orders = np.ctypeslib.as_array(_pointer_orders, shape=(100, 15))

_pointer_positions = c.cast(_lib._positions, c.POINTER(c.c_double))
Positions = np.ctypeslib.as_array(_pointer_positions, shape=(100,15))

_pointer_deals = c.cast(_lib._deals_history, c.POINTER(c.c_double))
Deals = np.ctypeslib.as_array(_pointer_deals, shape=(10000, 15))

# counters as numpy arrays size 1

_pointer_norders = c.cast(_lib._iorder, c.POINTER(c.c_int))
_nOrders = np.ctypeslib.as_array(_pointer_norders, shape=(1,))

_pointer_npositions = c.cast(_lib._ipos, c.POINTER(c.c_int))
_nPositions = np.ctypeslib.as_array(_pointer_npositions, shape=(1,))

_pointer_ndeals = c.cast(_lib._ideals, c.POINTER(c.c_int))
_nDeals = np.ctypeslib.as_array(_pointer_ndeals, shape=(1,))

def nOrders():
    return _nOrders[0]

def nDeals():
    return _nDeals[0]

def nPositions():
    return _nPositions[0]

### Ctypes + Numba interface Awesome!!
### https://numba.pydata.org/numba-doc/dev/user/cfunc.html

_sendorder = _lib.sendOrder
_sendorder.argtypes = [c.c_double]*8
_sendorder.restype = None

@jit
def __sendorder(kind, price=-1.,  volume=-1.,
              sloss=-1., tprofit=-1., deviation=-1.,
              ticket=-1., source=Order_Source_Client):
    _sendorder(kind, price, volume, sloss, tprofit, deviation,
                 ticket, source)

sendOrder = c.CFUNCTYPE(None, c.c_double, c.c_double, c.c_double, c.c_double, \
                     c.c_double, c.c_double, c.c_double, c.c_double)(__sendorder)


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

BookIndexes = dict( zip("OK OP VV SL TP DI TK OS PP PT PC DT DV DR DE".split(' '), np.arange(15)))

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
