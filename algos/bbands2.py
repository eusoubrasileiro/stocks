import pandas as pd
import numpy as np
from numba import jit, prange
import sys
from sklearn import preprocessing
from sklearn.ensemble import ExtraTreesClassifier
import talib as ta
from matplotlib import pyplot as plt

debug=True
quantile_transformer = preprocessing.QuantileTransformer(
    output_distribution='normal', random_state=0)

@jit(nopython=True,  parallel=True)
def bollingerSignal(price, pricem1, mean, meanm1, lband, lbandm1):
    """
    Based on a bollinger band defined by upper-band and lower-band
    return signal:
        buy 1 : crossing from down-outside to inside it's buy
        hold : nothing usefull happend
    m1 stands for minus one, the sample before.
    """
    n = len(price)
    signal = np.zeros(n)
    for i in prange(n):
        if price[i] > lband[i] and pricem1[i] <= lbandm1[i]: # crossing from down to up
            signal[i] = 1
        elif price[i] > mean[i] and pricem1[i] <= meanm1[i]: # crossing the mean
            signal[i] = 2
        else:
            signal[i] = 0
    return signal

def rawSignals(obars, window=21, nbands=3):
    """
    Detect crossing of bollinger bands those created
    buy or sell raw signals.
    """
    bars = obars.copy() # avoid warnings
    bars['OHLC'] = np.nan # typical price
    bars.OHLC.values[:] = np.mean(bars.values[:,0:4], axis=1) # 1000x faster
    price = bars.OHLC.values
    inc = 0.5
    for i in range(nbands):
        upband, sma, lwband =  ta.BBANDS(price, window*inc)
        bars['bandlw'+str(i)] = lwband
        bars['bandsma'+str(i)] = sma
        bars['bandsg'+str(i)] = 0 # signal for this band
        signals = bollingerSignal(price[1:], price[:-1], sma[1:], sma[:-1],
                                  lwband[1:], lwband[:-1])
        bars.loc[1:, 'bandsg'+str(i)] = signals.astype(int) # signal for this band
        inc += 0.5
    bars.dropna(inplace=True)
    return bars

def stdFeatures(obars, nbands, nostd=[]):
    """"
    standardize features for signal vector
    return index of feature columns
    """
    bars = obars.copy()
    features = bars.columns
    nostd.extend(['dated', 'date']) # dont std by default
    for feature in nostd:
        if feature in features:
            features.remove(feature) # remove what should not be standardized

    # standardize
    bars.iloc[:, features] = bars.iloc[:, features].fillna(0) #  quantile Transform dont like nans
    bars.iloc[:, features] = bars.iloc[:, features].apply(lambda x: x.clip(*x.quantile([0.001, 0.999]).values), axis=0)

    bars.iloc[:, features] = quantile_transformer.fit_transform(
        bars.iloc[:, features].values)

    return features, bars

def dayFeatures(obars):
    bars = obars.copy() # avoid warning
    # needed to reset orders by day
    days = bars.index.to_period('D').asi8
    bars['date'] = days # # integer day from year 1 day 1 gregorian day
    # day information signal [0, 1]     # i think can be faster
    bars['dated'] = 0
    for i, vars in enumerate(bars.groupby(bars.date)):
        day, group = vars
        bars.loc[group.index, 'dated'] = 1 if i%2==0 else 0 # odd or even day change
    bars.dropna(inplace=True)
    return bars

def crossFeatures(obars, window=21, nbands=3, log=[], nofeatures=[]):
    """
    Create cross-feature columns and return a new dataframe
    """
    bars = obars.copy() # avoid warning
    ### Signal Features
    features = bars.columns
    for feature in log:
        if feature not in features:
            print("column dont exist ", feature)
            return
        bars.loc[:, feature] = np.log(bars.loc[:, feature]+5)  # to avoid division by zero/inf etc
    for feature in nofeatures:
        if feature in features:
            features.remove(feature) # remove what should not cross-featured
    # tecnical indicators features
    inc = 0.5
    for column in features: # ignore date/dated
        for i in range(nbands):      # 1e-8 to avoid creating nans
            ema =  ta.EMA(bars[column].values.astype(np.float64), window*inc)
            sfx = str(column)+str(i)  # name suffix
            # remove nbands emas
            bars['dema'+sfx] = bars[column] - ema
            bars.loc[:, 'demav'+sfx] = np.nan
            # return of the differences to the ema (know to have good response for prediction)
            bars.loc[1:, 'demav'+sfx] = bars['dema'+sfx].values[:-1]/(1e-8+bars['dema'+sfx].values[1:])
            macd, sg, ht = ta.MACD(bars[column].values.astype(np.float64),
                                   int(window*inc*0.5), int(window*0.75*inc), int(0.25*inc*window))
            #dow.loc[:, 'macd'+sfx] = macd
            bars.loc[:, 'dmacdv'+sfx] = np.nan
            bars.loc[1:, 'dmacdv'+sfx] = macd[:-1]/(1e-8+macd[1:])
            inc += 0.5
    return bars
