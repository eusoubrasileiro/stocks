import pandas as pd
import numpy as np
import datetime
import random
from numba import jit, njit, prange
from algos.util import progressbar
from algos.backtest.metaEngine import *

@njit(nogil=True, parallel=True)
def _sortina(money, risk_free):
    """reward-to-variability ratio fast sortina numba"""
    minutes = len(money) #
    freemoney = np.linspace(1., risk_free, minutes) # lin aprox. risk free
    swings = money - freemoney # variation based on a risk-free investment
    mean = 0.
    nswings = 1
    for i in range(len(swings)): # punish negative swings only
        if swings[i] < 0:
            mean += -swings[i]
            nswings += 1
    mean /= nswings # average of negative swings
    excessreturn = money[-1]/money[0]-risk_free
    return excessreturn/(1+mean)

def createTicks(obars, seed=0):
    """
    Create ticks from Bars (1 minute time-frame): time, Open, High, Low, TickVol, RealVol
    - Real volume and tick volume are equally spread across ticks.
    - Total ticks is len(bars*3) last minute Close is ignored
    - Time of `High` and `Low` ticks are random inside the minute bar

    * obars:
        bars data-frame 1 minute-time-frame downloaded by `meta5Ibov`
    * seed:
        by default reproducible seed=0

    Todo:
      - any time-frame
      - random tick and real volume?
    """
    if not (seed is None):  # random tick times
        random.seed(seed)

    bars = obars.copy()
    bars.drop(columns=['S'], inplace=True)
    bars['time'] = bars.index.astype(np.int64)//10**9 # (to unix timestamp seconds precision)
    bars.TV = bars.TV//3
    bars.RV = bars.RV//3
    bars['time'] = bars.index.astype(np.int64)//10**9 # (to unix timestamp seconds precision)
    # [ time open tv rv ] [ time high tv rv ] [ time low tv rv ]
    ticks = bars.iloc[:, [6, 0, 4, 5,
              6, 1, 4, 5,
              6, 2, 4, 5] ].values.flatten()
    ticks = ticks.reshape(-1, 4)
    # Create random seconds between 1 and 59 seconds for `High` and `Low`  Ticks
    range_in_minute = range(1, 59)
    timeinc = [ [0] + random.sample(range_in_minute, 2) for i in range(len(ticks)//3) ]
    timeinc = np.array(timeinc).flatten()
    assert len(timeinc) == len(ticks)
    ticks[:, 0] += timeinc
    # sort ticks by time collum
    ticks = ticks[ ticks[:, 0].argsort()]

    return ticks

# class strategyTester(object):
#     """
#     One-Minute time-frame strategy tester for stocks
#     allways consider worst case scenario to compensate
#     not using all ticks.
#         Some examples:
#         - Buy on minute High and sell on minute Low
#         - take-profit only in minute Low if direction is buy
#     Uses expire time for an order.
#     """
#     def __init__(self, dfprices, dfpredictions, start=datetime.timedelta(days=1),
#                         end=datetime.timedelta(hours=2)):
#         """
#             receives data-frames:
#             prices : created by `meta5Ibov` for specific symbol
#             predictions : book of 'predictions' each row must have columns
#              index : `datetime.datetime` of prediction (M1 time)
#              dir : direction of prediction int 1:buy or -1:sell
#             start / end : additional time to consider on array of prices during
#             simulation, to guarantee open and close of all orders.
#             (internal copies of dataframes are made)
#          """
#         self.dfprices = dfprices[["H","L"]].copy() # worst case simulation M1
#         self.dfpredictions = dfpredictions.copy()
#         self.obookdict = engine.ordersbookdict
#         self.dictsize = len(engine.ordersbookdict)
#         self.start = start
#         self.end = end
#         # simulation results
#         self.orders_open = None # very few orders stay here so can be very small
#         self.orders_closed = None # all or\ders end-up here so must be bigs
#         self.orders = None # DataFrame with result of each order after simulation
#         assert(self.dfpredictions.dir.unique() == [1, -1],
#                "acceptable directions are -1/1")
#         self.dfprices, self.dfpredictions = self._alignTime(self.dfprices,
#             self.dfpredictions, start, end)
#         self._createArrays()
#
#     def _alignTime(self, prices, predictions, startdelta=datetime.timedelta(days=1),
#                         enddelta=datetime.timedelta(hours=2)):
#         """
#         Align time indexes of prices and predictions dataframes
#         From that create integer needed by backtestEngine
#         Also create integer index of start of day and end of day needed by backtestEngine.
#         """
#         # "convert" minutes to relative minutes integers
#         prices['i'] = np.arange(0, len(prices), 1)
#         # to store begin and end "i" of days indexes
#         prices['idayend'] = 0
#         prices['idaystart'] = 0
#         prices['date'] = prices.index.date
#         for date, group in prices.groupby(prices.date):
#             prices.loc[group.index, 'idayend'] = group.i.max()
#             prices.loc[group.index, 'idaystart'] = group.i.min()
#         #dates = prices.groupby(prices.date).agregate(['min', 'max'])
#         #prices.loc['date' == dates]
#         ## add one day at the beggining and two hours in the end
#         prices = prices.loc[predictions.index[0]-startdelta:predictions.index[-1]+enddelta]
#         # get reference time indexes "i" from prices/prices
#         predictions['i'] = prices.loc[predictions.index].i
#         # set to zero at the begging of prices array
#         start = prices.i[0] # get the "i" corresponding to the begin of the predictions
#         # align everything to the begin of prices data frame
#         # time index "i" will be zero at that time
#         predictions.i -= start
#         prices.i -= start
#         prices.idayend -= start
#         prices.idaystart -= start
#         # in case we have a day cut in half the begin will be where the prices array started
#         # that means 0.
#         prices.loc[prices.idaystart < 0, 'idaystart'] = 0
#         prices.drop(columns=["date", "i"], inplace=True)
#         return prices, predictions
#
#     def _createArrays(self):
#         """
#         setup/create book of open orders and closed orders
#         those arrays are base for backtestEngine
#         """
#         self.prices = self.dfprices.values.astype(np.float64)
#         self.predictions = self.dfpredictions.values.astype(np.int32) # get array data only
#         self.orders_open = np.zeros((self.dfpredictions.shape[0], self.dictsize), order='C')*np.nan
#         self.orders_closed = np.zeros((self.dfpredictions.shape[0], self.dictsize), order='C')*np.nan
#         # *nan to make it easy to read after
#         #return arrayprices, arraypredictions
#
#     def _executedOrders(self):
#         self.orders = pd.DataFrame(self.orders_closed[:self.nc,:],
#                                 columns=self.obookdict.keys())
#         self.orders.dropna(inplace=True)
#
#     def setupScenarios(self, size, verbose=True):
#         """
#         Calculate maximum number of scenario simulations based on current data
#         and scenario `size` in minutes.
#         """
#         glast = self.predictions[-1, 1] - size + 1 # last possible int time to have the last scenario
#         # its index would be a search for a value less or equal it (in the array reversed)
#         # the last inde is the size minus that index
#         ilast = len(self.predictions)-engine.argnextLE(self.predictions[::-1, 1], 0, glast) + 1
#         # so ilast is the number of possible simulations with size nwindow (in time)
#         self.nscenarios = ilast
#         if verbose:
#             print('number of possible scenarios', self.nscenarios)
#         return self.nscenarios
#
#     # def runScenarios(self):
#     #     for i in np.random.randint(0, ilast, size=nsize))
#
#     def Scenario(self, begi, simperiodm, capital=50000, maxorders=10, norderperdt=1, perdt=60,
#                 minprofit=100., expected_var=0.01, exp_time=90,
#                 rwr=3., riskap=0.015, verbose=True):
#         """
#         begi : time integer index defining the 'begin' of this scenario (max: len(self.dfpredictions))
#         simperiodm : simulation size in minutes for this scenario (max: < len(self.dfpredictions))
#         """
#         endi=int(self.end.total_seconds()/60) # how many minutes after to close all orders
#         predictions = self.predictions
#         input_predictions = predictions[predictions[:,1] < predictions[begi,1]+simperiodm-1, :][begi:].copy()
#         input_prices = self.prices[predictions[begi, 1]:predictions[begi, 1]+simperiodm+1+endi].copy()
#         # allignt start day and end day to zero
#         input_prices[:, 2:] = np.clip(input_prices[:,2:]-predictions[begi,1], 0, np.inf)
#         input_predictions[:, 1] -= predictions[begi, 1]
#         self.exprices = input_prices # latest used/run
#         self.expredictions = input_predictions
#         money, nc, no = engine.Simulator(input_prices, input_predictions,
#                      self.orders_open, self.orders_closed, capital, maxorders,
#               norderperdt, perdt, minprofit, expected_var, exp_time, rwr, riskap)
#         self.money = money
#         self.nc = nc
#         self.no = no
#         self._executedOrders()
#
#     def Simulate(self, capital=50000, maxorders=10, norderperdt=1, perdt=60,
#                 minprofit=100., expected_var=0.01, exp_time=90,
#                 rwr=3., riskap=0.015, verbose=True):
#         self.exprices = self.prices # latest used/run
#         self.expredictions = self.predictions
#         money, nc, no = engine.Simulator(self.prices, self.predictions,
#                             self.orders_open,self.orders_closed, capital, maxorders,
#                      norderperdt, perdt, minprofit, expected_var, exp_time, rwr, riskap)
#         self.money = money
#         self.nc = nc
#         self.no = no
#         self._executedOrders()
#
#     def avgAccuracy(self):
#         """
#         last simulation average accuracy of executed orders (decimal percentage)
#         TODO: do better than just > 0 SS code?
#         """
#         if len(self.orders) == 0:
#             print('there are no executed orders')
#             return np.nan
#         return len(self.orders[self.orders.SS > 0])/len(self.orders)
#
#     def drawDown(self):
#         """
#         max negative variation of money during simulation (decimal percentage)
#         equivalent to np.percentile(..., 0)...
#         """
#         if len(self.orders) == 0:
#             print('there are no executed orders')
#             return np.nan
#         return np.min(self.money)/self.money[0]-1.
#
#     def profit(self):
#         """profit in percentage based on initial capital"""
#         if len(self.money) > 0:
#             return self.money[-1]/self.money[0]-1.
#         return np.nan
#
#     def basePercentis(self):
#         """P0, P10, 50 of money variation during simulation (decimal percentage)"""
#         if len(self.orders) == 0:
#             print('there are no executed orders')
#             return np.zeros(3)*np.nan
#         return np.percentile(self.money/self.money[0], [0, 10, 50])-1.
#
#     def volatility(self):
#         """simple money volatility stdev"""
#         if len(self.orders) == 0:
#             print('there are no executed orders')
#             return np.nan
#         var = strategytester.money[1:]/strategytester.money[:-1]
#         return np.std(var, ddof=1)
#
#     def sortina(self, risk_free=None):
#         """
#         reward-to-variability ratio
#         adapted sortino ratio with linear aproximation for fixed compound interest
#         risk_free : risk free investment return (treasure bonds etc.)
#             depend on the simulation period (default: daily SELIC)
#         """
#         if len(self.orders) == 0:
#             print('there are no executed orders')
#             return np.nan
#         # based on daily interest rate 0.75 month SELIC
#         # corrected to business days 22 per month
#         if risk_free is None:
#             days = self.days()
#             risk_free = 1.00034**days
#         return _sortina(self.money, risk_free)
#
#     def days(self):
#         """number of days on last simulation"""
#         ndays = len(np.unique(self.exprices[:,3])) # based on unique istart
#         return ndays
#
#     def avgOrdersDay(self):
#         """average number of orders executed per day on last simulation run"""
#         if hasattr(self, 'exprices'):
#             return len(self.orders)/self.days()
#         else:
#             print('Did not run any simulation yet')
#             return 0.
