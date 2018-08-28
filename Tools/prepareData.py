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


# run the firs time the module is Load/run
meta5filepath = '/home/andre/.wine/drive_c/users/andre/Application Data/MetaQuotes/Terminal/Common/Files'
os.chdir(meta5filepath)
stocks_stats = pd.read_csv('stocks_stats_2018.csv', index_col=0)

def createTargetVector(X, targetsymbol, view=True):
    """
    Create y target vector (column) shift back in time 120 minutes.

    training : True
        when training we can drop the last samples that are nans
        default behavior
    """
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
        axr[1].plot(X.y.values[-1200:], label='-120 minutes target class up/down : 1/0')
        axr[1].legend(loc='center')
    y = X.y
    X.drop(['y','ema'], axis=1, inplace=True)
    # those are the minutes that will be used for prediction
    indexp = y[y.isnull()].index
    #y = y[~y.isnull()] # remove last 120 minutes
    return X, y, indexp

# def clip_outliers(X, percentil=0.1):
#     pmin, pmax = np.percentile(X, [percentil, 100-percentil])
#     return np.clip(X, pmin, pmax)
#
# def bucketize(X, nclass=10):
#     ### bucketize or discretize serie
#     discrete = pd.cut(X, nclass)
#     return discrete.codes
#
# def bucketize_volume(V, nclass=10):
#     """ Tick volume and money volume have huge values
#     that are better represented by a log scale
#     than in discrete classes"""
#     logvols =  np.log(V)
#     # clip outliers
#     logvols = clip_outliers(logvols)
#     return bucketize(logvols)
#
# def BucketizeAndClip(X):
#     # bucketize some features and clip outliers of all the data
#     for col in X:
#         # bucketize volumes and tick volume but before make log of them
#         if col.endswith('R') or col.endswith('T'):
#             X[col] = bucketize_volume(X[col].values)
#         # remove outliers of the rest of the data
#         else:
#             X[col] = clip_outliers(X[col])

def LogVols(X):
    for col in X:
        #  make log of them
        if col.endswith('R') or col.endswith('T'):
            X[col] = np.log(X[col].values+1.) # to avoid log(0)

def ScaleNormalize(X, stats):
    """given mean and variance (stats dataframe)
    for each collum `convert` to variance 1 and mean 0
    subtract mean and divide by variance"""
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
        df['macd_mean'+quote] = ta.MACD(df[quote].values, span, span*2)[0]
        df['macd_double'+quote] = ta.MACD(df[quote].values, span, span*3)[0]
        df['rsi_2'+quote] = ta.RSI(df[quote].values, span*2)
        df['rsi_3'+quote] = ta.RSI(df[quote].values, span*3)
    return df

def RemoveSimilarFeatures(X, correlation=95.):
    """
    remove features with more than 95% absolute correlation
    note : very slow for calculate on big data frames
    """
    correlation = X.corr()
    corr_matrix = correlation.abs()
    # Select upper triangle of correlation matrix
    upper = corr_matrix.where(np.triu(np.ones(corr_matrix.shape), k=1).astype(np.bool))
    # Find index of feature columns with correlation greater than 0.95
    to_drop = [column for column in upper.columns if any(upper[column] > 0.95)]
    # Drop features
    X.drop(to_drop, axis=1, inplace=True)
    return X

def GetTrainingPredictionVectors(X, targetsymbol='PETR4_C',
        verbose=True, correlated=95.):
    """
    Calculate features for NN training and target binary class.
    Returns X, y, Xp (--future prediction--)

    inputs
    X : dataframe fully populated with symbols from Meta5_Ibov_Load
    targetsymbol : chosen symbol from ibovespa dataframe
    correlated : str (file path) or float
        str : file path with ascii file with selected feature columns
        float : correlation cutoff for removing similar feature
    """
    X, y, indexp = createTargetVector(X, targetsymbol=targetsymbol, view=verbose)
    LogVols(X)
    X = createCrossedFeatures(X)

    if isinstance(correlated, str): # file path
        # select only columns use file with name of collumns previouly backtested
        # ascii file each column diveded by spaces
        with open(correlated, 'r') as f:
            columns = f.read()
        select_columns = columns.split(' ')[:-1]
        X = X[select_columns]
    else: # calculate correlations and remove by cut-off
        X = RemoveSimilarFeatures(X, correlation=correlated)

    X = X.dropna() ## drop nans at the begging of the data

    # scaler using known stats of stocsk 2018
    ScaleNormalize(X, stocks_stats)

    # last 120 minutes (can be used only for prediction on real time)
    Xp = X.loc[indexp] ## for prediction get the last
    ## for training remove last 120 minutes for prediction
    X.drop(Xp.index, inplace=True)
    y = y[X.index] ## update training vector, this already removes the last 120 nan values
    return X, y, Xp

def GetTrainingVectors(X, targetsymbol='PETR4_C', verbose=True):
    """
    calls GetTrainingPredictionVectors
    """
    X, y, Xp = GetTrainingPredictionVectors(X, targetsymbol, verbose=verbose)
    return X, y
