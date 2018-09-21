from matplotlib import pyplot as plt
import os
import sys
import pandas as pd
import numpy as np
import struct
import datetime
from Tools.util import progressbar
from Tools import prepareData, torchNN, torchCV
from Tools import meta5Ibov
import seaborn as sns
import talib as ta
from Tools.backtestEngine import Simulate

def alignTime(prices, predictions, startdelta=datetime.timedelta(days=1),
                    enddelta=datetime.timedelta(hours=2)):
    """
    Allign index of prices and predictions dataframes so indexes of minutes
    can be used as integers by backtestEngine.
    Also create index of start of day and end of day for backtestEngine.

    """
    # "convert" minutes to relative minutes integers
    prices['i'] = np.arange(0, len(prices), 1)
    # to store begin and end "i" of days indexes
    prices['idayend'] = 0
    prices['idaystart'] = 0
    prices['date'] = prices.index.date.astype(np.datetime64)
    ## Get the the indexes the define each day. Min and Max
    iday_end = prices.groupby(prices['date']).i.max()
    iday_end = iday_end.values
    iday_start = prices.groupby(prices['date']).i.min()
    iday_start = iday_start.values
    for date, group in prices.groupby(prices['date']):
        prices.loc[group.index, 'idayend'] = group.i.max()
        prices.loc[group.index, 'idaystart'] = group.i.min()
    predictions.set_index('time', inplace=True)
    ## add one day at the beggining and two hours in the end
    prices = prices[predictions.index[0]-startdelta:predictions.index[-1]+enddelta].copy()
    # get reference time indexes "i" from prices/prices
    predictions['i'] = prices.i.loc[predictions.index]
    # set to zero at the begging of prices array
    start = prices.i[0] # get the "i" corresponding to the begin of the predictions
    # align everything to the begin of prices data frame
    # time index "i" will be zero at that time
    predictions.i -= start
    prices.i -= start
    prices.idayend -= start
    prices.idaystart -= start
    # in case we have a day cut in half the begin will be where the prices array started
    # that means 0.
    prices.loc[prices.idaystart < 0, 'idaystart'] = 0
    return prices, predictions

def pricesClean(prices):
    """only used collumns by backtestEngine"""
    return prices[['H', 'L', 'idayend', 'idaystart']]

# very few orders stay here so can be very small
orders_open = None
# all or\ders end-up here so must be big
orders_closed = None

def setupBacktesting(prices, predictions):
    """
    setup/create book of open orders and closed orders
    those arrays are base for backtestEngine
    """
    global orders_open, orders_closed
    arrayprices = prices.values.astype(np.float64)
    arraypredictions = predictions.values.astype(np.int32) # get array data only
    # C contigous necessary for Cython
    orders_open = np.zeros((predictions.shape[0], 10), order='C')*np.nan
    orders_closed = np.zeros((predictions.shape[0], 10), order='C')*np.nan
    # *nan to make it easy to read after
    return arrayprices, arraypredictions
