%%writefile execsims.py
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
from Tools import backtest
from Tools.util import progressbar
import seaborn as sns
import talib as ta
import paramcase 
import multiprocessing

%%writefile paramcase.py

import os
import sys
import pandas as pd
import numpy as np
import struct
import datetime
from Tools.util import progressbar
from Tools import prepareData, torchNN, torchCV
from Tools import meta5Ibov 
from Tools import backtest
from Tools.util import progressbar
import seaborn as sns
import talib as ta


def paramcase(params):
    prices, X, lag, clip = params
    X["y"] = np.nan
    X["ema"] = ta.EMA(X.C.values, lag)
    X.loc[ X.C > X.ema, 'y'] = 1
    X.loc[ X.C < X.ema, 'y'] = 0
    X.y = X.y.shift(-lag) 
    y = X.y.dropna()    
    Yn = pd.DataFrame(index=y.index)
    Yn['dir'] = y.values
    sumy = np.convolve(Yn.dir.values, np.ones(lag), mode='valid')
    sumy = np.concatenate((np.ones(lag)*np.nan, sumy))[:-1]
    Yn['ysum'] = sumy
    Yn['dir'] = np.nan
    Yn.loc[Yn.ysum > (1-clip)*lag, 'dir'] = 1
    Yn.loc[Yn.ysum <= clip*lag, 'dir'] = -1
    Yn = prepareData.removedayBorders(Yn, int(lag))
    Yn.dropna(inplace=True)
    Yn.drop(columns=['ysum'], inplace=True)    
    case = []
    for i in range(10):
        minprofit=np.random.randint(150, 500, 50)
        expectvar=np.random.rand()*0.03+0.003
        isamples = np.random.randint(0, len(Yn), int(len(Yn)*0.03)) #3%
        Y = Yn.iloc[isamples].copy()
        Y.sort_index(inplace=True)
        strategytester = backtest.strategyTester(prices.copy(), Y)
        strategytester.setupBacktesting(strategytester.dfprices, strategytester.dfpredictions)
        # 10 random cenarios 6 months???? seams good
        # get function, todo future
        money, cbook = strategytester.Simulaten(5000000, 10,  2, 60, minprofit, expectvar, lag)
        accuracy = len(cbook [ cbook.SS > 0])/ len(cbook)
        case.append([accuracy, lag, clip, minprofit, expectvar])        
    return case

if __name__ == '__main__':
    datapath = r'C:\Users\alferreira\Downloads\stocks\data'
    meta5Ibov.setDataPath(datapath, datapath, verbose=False)
    meta5Ibov.setDataPath(datapath, datapath, verbose=False)
    meta5Ibov.loadMeta5Data(suffix='M1.mt5bin')
    prices = meta5Ibov.getSymbol("PETR4")
    #lag=90
    #clip=10
    X = prices[["C"]].copy()
     # random samples
    size=100
    lag, clip = np.linspace(30, 240, size, dtype=int), np.linspace(60, 95, size, dtype=int)
    pool = multiprocessing.Pool(60) #creates a pool of process, controls worksers
    metrics = pool.map(paramcase.paramcase, [[prices.copy(), X.copy(), lag[i], clip[i]] for i in range(size)]) #make our results with a map call
    pool.close() #we are not adding any more processes
    pool.join() #tell it to wait until all threads are done before going on
#     metrics = Parallel(n_jobs=72, verbose=4, backend="threading")(
#             delayed(paramcase.paramcase)(prices.copy(), X.copy(), lag[i], clip[i])  for i in np.random.randint(0, 100, 100))
    metrics = np.array(metrics)
    metrics.tofile('metrics.npy')