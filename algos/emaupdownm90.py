"""
Time-frame 1 minute Algorithm
Forecasting based on shifting n minutes of data.
Using a EMA to classify up or down above or bellow it in the next n minutes.
And averaging prediction of next n minutes to decide to buy or sell
Status - reviewing 
"""
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

def createTargetVector(X, targetsymbol, span=120, view=True):
    """
    Create y target vector (column) shift back in time 120 minutes.

    training : True
        when training we can drop the last samples that are nans
        default behavior
    """
    X['ema'] = ta.EMA(X[targetsymbol].values, span)
    X.loc[ X[targetsymbol] > X.ema, 'y'] = 1
    X.loc[ X[targetsymbol] < X.ema, 'y'] = 0
    X.y = X.y.shift(-span) # lag time assumption
    if view:
        f, axr = plt.subplots(2, sharex=True, figsize=(15,4))
        f.subplots_adjust(hspace=0)
        axr[0].plot(X.ema.values[-1200:], label='emaLag')
        axr[0].plot(X[targetsymbol].values[-1200:], label=targetsymbol)
        axr[0].legend()
        plt.figure(figsize=(10,3))
        axr[1].plot(X.y.values[-1200:], label='-Lag minutes target class up/down : 1/0')
        axr[1].legend(loc='center')
    y = X.y
    X.drop(['y','ema'], axis=1, inplace=True)
    # those are the minutes that will be used for prediction
    indexp = y[y.isnull()].index
    #y = y[~y.isnull()] # remove last 120 minutes
    return X, y, indexp

def createTargetVectorDiff(X, targetsymbol, span=120, cut=0.04, view=True):
    """
    Create y target vector (column) shift back in time 120 minutes.

    training : True
        when training we can drop the last samples that are nans
        default behavior
    """
    X['dif'] =  X[targetsymbol].shift(-span) - X[targetsymbol]
    X.loc[ X.dif > 0, 'y'] = 1
    X.loc[ X.dif <= 0, 'y'] = 0
    # X.y = X.y.shift(-span) # lag time assumption
    if view:
        f, axr = plt.subplots(3, sharex=True, figsize=(15,4))
        f.subplots_adjust(hspace=0)
        axr[0].plot(X[targetsymbol].values[-1200:], label=targetsymbol)
        axr[0].legend()
        plt.figure(figsize=(10,3))
        axr[1].plot(X.dif.values[-1200:], label='difLag')
        plt.figure(figsize=(10,3))
        axr[2].plot(X.y.values[-1200:], label='-Lag minutes target class up/down : 1/0')
        axr[2].legend(loc='center')
    y = X.y
    X.drop(['y','dif'], axis=1, inplace=True)
    # those are the minutes that will be used for prediction
    indexp = y[y.isnull()].index
    #y = y[~y.isnull()] # remove last 120 minutes
    return X, y, indexp

def removedayBorders(X, minutes):
    """
    X is dataframe must have a datetime index
    remove n minutes from beginning and end of day
    stock-market day starts at 10:00 am and ends at 5:00 pm
    """
    dt = datetime.timedelta(minutes=minutes)
    end = (datetime.datetime(1970, 1, 1, 17, 0)-dt).time()
    start = (datetime.datetime(1970, 1, 1, 10, 0)+dt).time()
    X['day'] = X.index.map(lambda x: x.date())
    for day, group in X.groupby('day'):
        if len(group) < minutes*3: # less than 3xminutes better not use it for tranning
            X.drop(group.index, inplace=True)
    X['start'] = X.day.map(lambda x: datetime.datetime(x.year, x.month, x.day, start.hour, start.minute, 0))
    X['end'] = X.day.map(lambda x: datetime.datetime(x.year, x.month, x.day, end.hour, end.minute, 0))
    Xn = X.query('index > start and index < end').copy()
    Xn.drop(columns=['day', 'start', 'end'], inplace=True)
    return Xn

def LogVols(X):
    for col in X:
        #  make log of them
        if col.endswith('R') or col.endswith('T'):
            X[col] = np.log(X[col].values+1.) # to avoid log(0)

def ScaleNormalize(X, stats):
    """given mean and variance (stats dataframe)
    for each collum `convert` to variance 1 and mean 0
    subtract mean and divide by variance"""
    if stats is None: # calculate mean and variance for each colum
    # and normalize than for mean 0 and variance 1
        for col in X: # make variance 1 and mean 0
            X[col] = (X[col]-np.mean(X[col]))/np.std(X[col])
    else: # mean and variance were specied
        for col in X: # make variance 1 and mean 0
            X[col] = (X[col]-stats.loc[col, 'mean'])/stats.loc[col, 'std']

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
        # [0] ignore MACD signal Line
        df['macd_mean'+quote] = ta.MACD(df[quote].values, span, span*2)[0]
        # [0] ignore MACD signal Line
        df['macd_double'+quote] = ta.MACD(df[quote].values, span, span*3)[0]
        df['rsi_2'+quote] = ta.RSI(df[quote].values, span*2)
        df['rsi_3'+quote] = ta.RSI(df[quote].values, span*3)
    return df

def getSimilarFeatures(X, correlation=0.95):
    """
    calculate features with more than 95% absolute correlation
    note : very slow for calculate on big data frames
    return: column names
    might be used to remove (drop) highly correlated columns
    """
    xcorrelation = X.corr()
    corr_matrix = xcorrelation.abs()
    # Select upper triangle of correlation matrix
    upper = corr_matrix.where(np.triu(np.ones(corr_matrix.shape), k=1).astype(np.bool))
    # Find index of feature columns with correlation greater than 0.95
    similar_columns = [column for column in upper.columns if any(upper[column] > correlation)]
    return similar_columns

def GetTrainingPredictionVectors(X, targetsymbol='PETR4_C', span=120,
        selected=None, stats=None, correlation=0.98, verbose=True):
    """
    Calculate features for NN training and target binary class.
    Returns X, y, Xp (--future prediction--)

    number of lost samples due (shift + EMA's + unknown):
    nwasted = 120+120+7

    inputs
    X : dataframe fully selectedcollumns, populated with symbols from Meta5_Ibov_Load
    targetsymbol : chosen symbol from bovespa dataframe
    selected : list
        list : list of collumns to keep others will be removed
    stats :
        statistical mean and variance for each collumn in X DataFrame
        for normalization
    """
    X, y, indexp = createTargetVector(X, targetsymbol, span, verbose)
    LogVols(X)
    X = createCrossedFeatures(X, span)

    if selected is None:  # calculate correlations and remove by cut-off
        selected = getSimilarFeatures(X, correlation)
    # select only desired columns
    X = X[selected]
    X = X.dropna() ## drop nans at the begging of the data
    ScaleNormalize(X, stats) # normalize by stats

    # last 120 minutes (can be used only for prediction on real time)
    Xp = X.loc[indexp] ## for prediction get the last
    ## for training remove last 120 minutes for prediction
    X.drop(Xp.index, inplace=True)
    y = y[X.index] ## update training vector, this already removes the last 120 nan values
    return X, y, Xp

def GetTrainingPredictionVectorsDiff(X, targetsymbol='PETR4_C', span=120,
        selected=None, stats=None, correlation=0.98, verbose=True):
    """
    Calculate features for NN training and target binary class.
    Returns X, y, Xp (--future prediction--)

    number of lost samples due (shift + EMA's + unknown):
    nwasted = 120+120+7

    inputs
    X : dataframe fully selectedcollumns, populated with symbols from Meta5_Ibov_Load
    targetsymbol : chosen symbol from bovespa dataframe
    selected : list
        list : list of collumns to keep others will be removed
    stats :
        statistical mean and variance for each collumn in X DataFrame
        for normalization
    """
    X, y, indexp = createTargetVectorDiff(X, targetsymbol, span, verbose)
    LogVols(X)
    X = createCrossedFeatures(X, span)

    if selected is None:  # calculate correlations and remove by cut-off
        selected = getSimilarFeatures(X, correlation)
    # select only desired columns
    X = X[selected]
    X = X.dropna() ## drop nans at the begging of the data
    ScaleNormalize(X, stats) # normalize by stats

    # last 120 minutes (can be used only for prediction on real time)
    Xp = X.loc[indexp] ## for prediction get the last
    ## for training remove last 120 minutes for prediction
    X.drop(Xp.index, inplace=True)
    y = y[X.index] ## update training vector, this already removes the last 120 nan values
    return X, y, Xp
