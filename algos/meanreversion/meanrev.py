import pandas as pd
import numpy as np
from numba import jit, prange
import sys
from sklearn import preprocessing
from sklearn.ensemble import ExtraTreesClassifier
import talib as ta
from .bbands3 import xyTrainingPairs, getIndexFeatures, lastSignal, barsFeatured, \
            getTrainingVectors, getForecastVector, fitPredict, sumdPred, sumyProb

@jit(nopython=True,  parallel=True)
def fastbollingerSignal(price, uband, lband):
    """
    Based on a bollinger band defined by upper-band and lower-band
    return signal:
        buy   1 : crossing down-outside it's buy
        sell -1 : crossing up-outside it's sell
        hold : nothing usefull happend
    """
    n = len(price)
    signal = np.zeros(n)
    for i in prange(n):
        if price[i] <= lband[i]: # crossing  down
            signal[i] = 1
        elif price[i] >= uband[i]: # crossing  up
            signal[i] = -1
        else:
            signal[i] = 0
    return signal

@jit(nopython=True) # 1000x faster than using pandas for loop
def traverseBuyBand(bandsg, high, low, day, amount, targetprofit, stoploss):
    """
    buy at ask = High at minute time-frame (being pessimistic)
    sell at bid = Low at minute time-frame
    classes are [0, 1] = [hold, buy]
    stoploss - to not loose much money (positive)
    targetprofit - profit to close open position
    amount - tick value * quantity bought in $$
    """
    buyprice = 0
    history = 0 # nothing, buy = 1, sell = -1
    buyindex = 0
    previous_day = 0
    ybandsg = np.empty(bandsg.size)
    ybandsg.fill(np.nan)
    for i in range(bandsg.size):
        if day[i] != previous_day: # a new day reset everything
            if history == 1:
                # the previous batch o/f signals will be saved with Nan don't want to train with that
                ybandsg[buyindex] = np.nan
            buyprice = 0
            history = 0 # nothing, buy = 1, sell = -1
            buyindex = 0
            previous_day = day[i]
        if int(bandsg[i]) == 1:
            if history == 0:
                buyprice = high[i]
                buyindex = i
                ybandsg[i] = 1 #  save this buy
            else: # another buy in sequence -> cancel the first (
                # the previous batch of signals will be saved with this class (hold)
                ybandsg[buyindex] = 0 # reclassify the previous buy as hold
                # new buy signal
                buyprice = high[i]
                buyindex = i
                #print('y: ', 0)
            history=1
            # net mode
        elif int(bandsg[i]) == -1: # a sell, cancel the first buy
            ybandsg[buyindex] = 0 # reclassify the previous buy as hold
            #print('y: ', 0)
            history=0
        elif history == 1:
            profit = (low[i]-buyprice)*amount # current profit
            #print('profit: ', profit)
            if profit >= targetprofit:
                # ybandsg[buyindex] = 1 # a real (buy) index class nothing to do
                history = 0
                #print('y: ', 1)
            elif profit <= (-stoploss): # reclassify the previous buy as hold
                ybandsg[buyindex] = 0  # not a good deal, was better not to entry
                history = 0
                #print('y: ', 0)
    # reached the end of data but did not close one buy previouly open
    if history == 1: # don't know about the future cannot train with this guy
        ybandsg[buyindex] = np.nan # set it to be ignored
    return ybandsg # will only have 0 (false positive) or 1's

@jit(nopython=True) # 1000x faster than using pandas for loop
def traverseSellBand(bandsg, high, low, day, amount, targetprofit, stoploss):
    """
    same as traverseSellBand but for sell positions
    buy at high = High at minute time-frame (being pessimistic)
    sell at low = Low at minute time-frame
    classes are [0, -1] = [hold, sell]
    stoploss - to not loose much money (positive)
    targetprofit - profit to close open position
    amount - tick value * quantity bought in $$
    """
    sellprice = 0
    history = 0 # nothing, buy = 1, sell = -1
    sellindex = 0
    previous_day = 0
    ybandsg = np.empty(bandsg.size)
    ybandsg.fill(np.nan)
    for i in range(bandsg.size):
        if day[i] != previous_day: # a new day reset everything
            if history == -1: # was selling when day ended
                # the previous batch o/f signals will be saved with Nan don't want to train with that
                ybandsg[sellindex] = np.nan
            sellprice = 0
            history = 0 # nothing, buy = 1, sell = 2
            sellindex = 0
            previous_day = day[i]
        if int(bandsg[i]) == -1:
            if history == 0:
                sellprice = low[i]
                sellindex = i
                ybandsg[i] = -1
            else: # another sell in sequence -> cancel the first
                # the previous batch of signals will be saved with this class (hold)
                ybandsg[sellindex] = 0 # reclassify the previous sell as hold
                # new buy signal
                sellprice = low[i]
                sellindex = i
            history = -1
        elif int(bandsg[i]) == 1: # a buy
            ybandsg[sellindex] = 0 # reclassify the previous sell as hold
            history = 0
        elif history == -1:
            profit = (sellprice-high[i])*amount # current profit
            if profit >= targetprofit:
                # ybandsg[sellindex] = -1 # a real (sell) index class nothing to do
                history = 0
            elif profit <= (-stoploss): # reclassify the previous buy as hold
                ybandsg[sellindex] = 0  # not a good deal, was better not to entry
                history = 0
    # reached the end of data but did not close one buy previouly open
    if history == -1: # don't know about the future cannot train with this guy
        ybandsg[sellindex] = np.nan # set it to be ignored
    return ybandsg


def rawSignals(obars, window=21, nbands=3, inc=0.5, save=True):
    """
    Detect crossing of bollinger bands those created
    buy or sell raw signals.
    Better do on MQL5
    """
    bars = obars.copy() # avoid warnings
    bars['OHLC'] = np.nan # typical price
    bars.OHLC.values[:] = np.mean(bars.values[:,0:4], axis=1) # 1000x faster
    price = bars.OHLC.values
    for i in range(nbands):
        upband, sma, lwband =  ta.BBANDS(price, window*inc)
        if save: # for plotting stuff
            bars['bandlw'+str(i)] = lwband
            bars['bandup'+str(i)] = upband
        bars['bandsg'+str(i)] = 0 # signal for this band
        signals = fastbollingerSignal(price, upband, lwband)
        bars.loc[:, 'bandsg'+str(i)] = signals.astype(int) # signal for this band
        inc += 0.5
    bars.dropna(inplace=True)
    return bars


@jit(nopython=True)
def mergebandsignals(ybandsell, ybandbuy):
    ybandsg = np.empty(ybandbuy.size)
    ybandsg.fill(np.nan)
    #ybandsg = ybandbuy + ybandsell
    for i in range(ybandsell.size):
        if np.isnan(ybandsell[i]) or ybandsell[i] == 0:  # no signal of sell
            ybandsg[i] = ybandbuy[i]
        elif ybandbuy[i] == 1: # special case when buy and sell at same time
            ybandsg[i] = 0 # we dont want that
        else:
            ybandsg[i] = ybandsell[i]
    return ybandsg

def targetFromSignals(obars, nbands=3, amount=1, targetprofit=15., stoploss=45.):
    """
    Create target class by analysing raw bollinger band signals
    those that went true receive 1 buy or -1 sell
    those that wen bad receive 0 hold
    A target class exist for each band.
    An essemble of bands together and summed are a better classifier.
    Note:.
    A day integer identifier column name 'date' must exist prior call.
    That is used to hold positions from passing to another day.
    """
    # bandsg, yband, ask, bid, day, amount, targetprofit, stoploss
    bars = obars.copy()
    for j in range(nbands): # for each band traverse it
        ibandsg = bars.columns.get_loc('bandsg'+str(j))
        # being pessimistic ... right
        ybandsell = traverseSellBand(bars.iloc[:, ibandsg].values.astype(int),
                                        bars.H.values, bars.L.values, bars.date.values,
                                        amount, targetprofit, stoploss)
        ybandbuy = traverseBuyBand(bars.iloc[:, ibandsg].values.astype(int),
                                        bars.H.values, bars.L.values, bars.date.values,
                                        amount, targetprofit, stoploss)
        bars['y'+str(j)] = mergebandsignals(ybandsell, ybandbuy)

    return bars

######################################################################
###################### MAIN function #################################
######################################################################
def getTrainingForecastVectors(obars, window=21, nbands=3,
        amount=1, targetprofit=15., stoploss=45., batchn=180):
    """
    return Xpredict, Xtrain, ytrain

    if Xpredict has signal all zero return None
    """
    bars = rawSignals(obars, window, nbands)
    signal = lastSignal(bars, nbands)
    if np.all(signal == 0): # no signal in any band no training
        return signal, None, None, None
    bars = barsFeatured(bars, window, nbands)
    bars = targetFromSignals(bars, nbands,
            amount, targetprofit, stoploss) # needs day identifier integer
    isgfeatures = getIndexFeatures(bars, nbands); # signal features standardized
    X, y, time = getTrainingVectors(bars, isgfeatures, nbands, batchn)
    Xforecast = getForecastVector(bars, isgfeatures, nbands, batchn)

    return signal, Xforecast, X, y
