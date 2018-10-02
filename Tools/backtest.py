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
from Tools.backtestEngine import Simulator
from Tools.backtestEnginen import Simulator as Simulatorn

class strategyTester(object):
    """
    One-Minute time-frame strategy tester for stocks
    allways consider worst case scenario to compensate
    not using all ticks.
        Some examples:
        - Buy on minute High and sell on minute Low
        - take-profit only in minute Low if direction is buy
    """
    def __init__(self, dfprices, dfpredictions, start=datetime.timedelta(days=1),
                        end=datetime.timedelta(hours=2)):
        """
            receives data-frames:
            prices : created by `meta5Ibov` for specific symbol
            predictions : book of 'predictions' each row must have columns
             time : `datetime.datetime` of prediction (M1 time-frame)
             dir : direction of prediction int 1:buy or -1:sell
            start / end : additional time to consider on array of prices during
            simulation, to guarantee open and close of all orders.
        """
        self.dfprices = dfprices
        self.dfpredictions = dfpredictions
        self.dfprices, self.dfpredictions = self.alignTime(dfprices.copy(),
            dfpredictions.copy(), start, end)
        self.dfprices = self.pricesClean(self.dfprices)
        self.orders_open = None # very few orders stay here so can be very small
        self.orders_closed = None # all or\ders end-up here so must be bigs

    def alignTime(self, prices, predictions, startdelta=datetime.timedelta(days=1),
                        enddelta=datetime.timedelta(hours=2)):
        """
        Allign time indexes of prices and predictions dataframes
        From that create integer needed by backtestEnginei
        Also create integer index of start of day and end of day needed by backtestEngine.
        """
        # "convert" minutes to relative minutes integers
        prices['i'] = np.arange(0, len(prices), 1)
        # to store begin and end "i" of days indexes
        prices['idayend'] = 0
        prices['idaystart'] = 0
        prices['date'] = prices.index.date.astype(np.datetime64)
        for date, group in prices.groupby(prices['date']):
            prices.loc[group.index, 'idayend'] = group.i.max()
            prices.loc[group.index, 'idaystart'] = group.i.min()
        predictions.set_index('time', inplace=True)
        ## add one day at the beggining and two hours in the end
        prices = prices[predictions.index[0]-startdelta:predictions.index[-1]+enddelta].copy()
        # get reference time indexes "i" from prices/prices
        predictions['i'] = prices.loc[predictions.index].i
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

    def pricesClean(self, prices):
        """only used collumns by backtestEngine"""
        return prices[['H', 'L', 'idayend', 'idaystart']]

    def setupBacktesting(self, prices, predictions):
        """
        setup/create book of open orders and closed orders
        those arrays are base for backtestEngine
        """
        self.arrayprices = prices.values.astype(np.float64)
        self.arraypredictions = predictions.values.astype(np.int32) # get array data only
        # C contigous necessary for Cython
        self.orders_open = np.zeros((predictions.shape[0], 10), order='C')*np.nan
        self.orders_closed = np.zeros((predictions.shape[0], 10), order='C')*np.nan
        # *nan to make it easy to read after
        #return arrayprices, arraypredictions


    def Simulate(self, money=50000, maxorders=10, norderperdt=1, perdt=60,
                minprofit=100, expected_var=0.01, exp_time=90):
        money, nc, no = Simulator(np.ascontiguousarray(self.arrayprices),
                                np.ascontiguousarray(self.arraypredictions),
                            self.orders_open,self.orders_closed, money, maxorders,
                     norderperdt, perdt, minprofit, expected_var, exp_time)
        ordersbook = {"EP" : 0, "QT" : 1, "DR" : 2, "TP": 3, "SL" : 4,
                      "OT" : 5, "CP" : 6, "SS" : 7, "CT" : 8, "MB" : 9}
        dfclosedbook = pd.DataFrame(self.orders_closed[:nc,:],
                                columns=ordersbook.keys())
        dfclosedbook.dropna(inplace=True)
        return money, dfclosedbook

    def Simulaten(self, money=50000, maxorders=10, norderperdt=1, perdt=60,
                minprofit=100, expected_var=0.01, exp_time=90):
        money, nc, no = Simulatorn(np.ascontiguousarray(self.arrayprices),
                                np.ascontiguousarray(self.arraypredictions),
                            self.orders_open,self.orders_closed, money, maxorders,
                     norderperdt, perdt, minprofit, expected_var, exp_time)
        ordersbook = {"EP" : 0, "QT" : 1, "DR" : 2, "TP": 3, "SL" : 4,
                      "OT" : 5, "CP" : 6, "SS" : 7, "CT" : 8, "MB" : 9}
        dfclosedbook = pd.DataFrame(self.orders_closed[:nc,:],
                                columns=ordersbook.keys())
        dfclosedbook.dropna(inplace=True)
        return money, dfclosedbook
