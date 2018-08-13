import os
import numpy as np
import pandas as pd
import datetime
from matplotlib import pyplot as plt
from sklearn import preprocessing
from sklearn.model_selection import train_test_split
from sklearn.utils import shuffle
from sklearn.preprocessing import StandardScaler
import talib as ta
from Tools.util import progressbar
import torch as th
import torch.nn.functional as F

def createTargetVector(X, targetsymbol, view=True):
    span=120 # best long trend guide until now
    X['ema'] = ta.EMA(X[targetsymbol].values, span)
    X.loc[ X[targetsymbol] > X.ema, 'y'] = 1
    X.loc[ X[targetsymbol] < X.ema, 'y'] = 0
    X.y = X.y.shift(-span) # has to be 120!!!
    if view:
        f, axr = plt.subplots(2, sharex=True, figsize=(15,4))
        f.subplots_adjust(hspace=0)
        axr[0].plot(X.ema.values[-1200:], label='ema120')
        axr[0].plot(X[targetsymbol].values[-1200:], label=targetsymbol)
        axr[0].legend()
        plt.figure(figsize=(10,3))
        axr[1].plot(X.y.values[-1200:], label='-60 minutes target class up/down : 1/0')
        axr[1].legend(loc='center')

    Xp = X[X.y.isna()] # those are the minutes that will be used for prediction
    X.dropna(inplace=True) # drop the last 60 minutes (can be used only for prediction on real time)
    y = X.y
    X.drop(['y','ema'], axis=1, inplace=True)
    return X, y, Xp

def clip_outliers(X, percentil=0.1):
    pmin, pmax = np.percentile(X, [percentil, 100-percentil])
    return np.clip(X, pmin, pmax)

def bucketize(X, nclass=10):
    ### bucketize or discretize serie
    discrete = pd.cut(X, nclass)
    return discrete.codes

def bucketize_volume(V, nclass=10):
    """ Tick volume and money volume have huge values
    that are better represented by a log scale
    than in discrete classes"""
    logvols =  np.log(V)
    # clip outliers
    logvols = clip_outliers(logvols)
    return bucketize(logvols)

def createCrossedFeatures(df, span=60):
    """
    many indicators etc.
    note: all windows of moving averages are alligned to the right
    """
    def tofloat64(df): # talib needs this
        for col in df.columns:
            if df[col].dtype != np.float64:
                df.loc[:, col] = df.loc[:, col].values.astype(np.float64)
    tofloat64(df) # TALIB works only with real numbers (np.float64)
    # add two indicators hour and day of week
    df['hour'] = df.index.hour.astype(np.float64)
    df['weekday']  = df.index.dayofweek.astype(np.float64)
    df['minute'] = np.apply_along_axis(lambda x : np.float64(pd.to_datetime(x).minute),
                                       arr=df.index.values, axis=0)
    quotes = df.columns
    for quote in quotes:
        df['ema_2'+quote] = ta.EMA(df[quote].values, span*2)
        df['ema_3'+quote] = ta.EMA(df[quote].values, span*3)
        df['dema_2'+quote] = df[quote] - df['ema_2'+quote]
        df['dema_3'+quote] = df[quote] - df['ema_3'+quote]
        df['macd'+quote] = ta.MACD(df[quote].values)[0]
        df['macd_mean'+quote] = ta.MACD(df[quote].values, span, span*2)[0]
        df['macd_double'+quote] = ta.MACD(df[quote].values, span, span*3)[0]
        df['rsi_2'+quote] = ta.RSI(df[quote].values, span*2)
        df['rsi_3'+quote] = ta.RSI(df[quote].values, span*3)
    for i in range(len(quotes)-1):
        df[quotes[i]+'_'+quotes[i+1]] = df[quotes[i]]+df[quotes[i+1]]
    return df

X, y, Xp = createTargetVector(X, targetsymbol='PETR4_C')
# bucketize some features and clip outliers of all the data
for col in X:
    # bucketize volumes and tick volume but before make log of them
    if col.endswith('R') or col.endswith('T'):
        X[col] = bucketize_volume(X[col].values)
    # remove outliers of the rest of the data
    else:
        X[col] = clip_outliers(X[col])
X = createCrossedFeatures(X)
X.dropna(inplace=True) # remove nans due ema's etc.
y = y[X.index] # update training vector
# select only columns with more than 95% uncorrelated
# use file with name of collumns previouly backtested
with open('collumns_selected.txt', 'r') as f:
    collumns = f.read()
select_collumns = collumns.split(' ')[:-1]
X = X[select_collumns]


scaler = StandardScaler()
scaler.fit(X)
X[:] = scaler.transform(X)
