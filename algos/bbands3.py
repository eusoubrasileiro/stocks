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

def viewBands(bars, window=21, nbands=3, lastn=-500):
    if verbose:
        plt.figure(figsize=(15,7))
        plt.subplot(2, 1, 1)
        plt.plot(price[lastn:], 'k-')
        for i in range(nbands):
            # plot bands
            plt.plot(bars['bandup'+str(i)].values[lastn:], 'b--', lw=0.3)
            plt.plot(bars['bandlw'+str(i)].values[lastn:], 'b--', lw=0.3)
        plt.subplot(2, 1, 2)
        for i in range(nbands):
            plt.plot(bars['bandsg'+str(i)].values[lastn:], '+', label='band '+str(i))
            plt.ylabel('signal-code')
        plt.legend()
        plt.savefig('bbands.png')
        plt.close()

@jit(nopython=True,  parallel=True)
def bollingerSignal(price, pricem1, uband, ubandm1, lband, lbandm1):
    """
    Based on a bollinger band defined by upper-band and lower-band
    return signal:
        buy 1 : crossing from down-outside to inside it's buy
        sell 2 : crossing from up-outside to inside it's sell
        hold : nothing usefull happend
    m1 stands for minus one, the sample before.
    """
    n = len(price)
    signal = np.zeros(n)
    for i in prange(n):
        if price[i] > lband[i] and pricem1[i] <= lbandm1[i]: # crossing from down to up
            signal[i] = 1
        elif price[i] < uband[i] and pricem1[i] >= ubandm1[i]: # crossing from up to down
            signal[i] = 2
        else:
            signal[i] = 0
    return signal

@jit(nopython=True) # 1000x faster than using pandas for loop
def traverseBand(bandsg, yband, ask, bid, day):
    """
    buy at ask = High at minute time-frame
    sell at bid = Low at minute time-frame
    classes are [0, 1, 2] = [ hold, buy, sell]
    """
    buyprice = 0
    history = -1 # nothing, buy = 1, sell = 2
    buyindex = 0
    previous_day = 0
    for i in range(bandsg.size):
        if day[i] != previous_day: # a new day reset everything
            if history == 1:
                # the previous batch o/f signals will be saved with Nan don't want to train with that
                yband[buyindex] = np.nan
            buyprice = 0
            history = -1 # nothing, buy = 1, sell = 2
            buyindex = 0
            previous_day = day[i]
        if int(bandsg[i]) == 1:
            if history != 1:
                buyprice = ask[i]
                buyindex = i
            else:
                # the previous batch of signals will be saved with this class (hold)
                yband[buyindex] = 0
                # new buy signal
                buyprice = ask[i]
                buyindex = i
            history=1
        if int(bandsg[i]) == 2 and history == 1: # sell what was previously bought
            sellprice = bid[i]
            profit = sellprice - buyprice
            if profit > 0:
                yband[buyindex] = 1 # a real (buy) index class
                yband[i] = 2 # a real (sell) class
            else:
                yband[buyindex]  = 0 # not a good deal, was better to hold - index class
                yband[i] = 0 # not a good sell, better hold
            history=2
    # reached the end of data but did not close one buy previouly open
    if history == 1: # don't know about the future cannot train with this guy
        yband[buyindex] = np.nan # set it to be ignored
    return yband

@jit(nopython=True)
def xyTrainingPairs(df, batchn, nsignal_features=8, nbands=6):
    """
    assembly the TRAINING vectors X and y

    default signal features number is 8:
        based on 6 y columns plus the 6+2 of the original data-frame  (above)
    """
    X = np.zeros((len(df), nsignal_features*batchn))*np.nan # signal training vector 8xwindow
    y = np.zeros(len(df))*np.nan
    time = np.zeros(len(df))*np.nan # let it be float after we convert back to int
    # prange break this here, cannot use this way
    # still breaking somehow parallel doesnt like np.isnan
    for i in range(batchn, df.shape[0]):
        for j in range(nbands): # firs columns are y's each band look at the ys' target class
            if df[i , j] == df[i , j]: # if y' is not nan than we have a training pair X, y
                # X feature vector is the last window (signals + askv and bidv 8 dimension)
                X[i, :] = df[i-batchn:i, -nsignal_features:].flatten()
                y[i] = df[i, j]
                time[i] = i # index that represent date-time from the original data-frame
    notnan = ~np.isnan(y)
    return X[notnan], y[notnan], time[notnan]


def xyTrainingIndex(bars):
    # %%time
    # ys = bars.iloc[:, iyband].values
    # iXy = np.argwhere(~np.isnan(ys))
    # y = np.zeros(iXy.shape[0])
    # y = ys[iXy[:, 0], iXy[:, 1]]
    pass

def standardizeFeatures(obars, nbands):
    """"
    standardize features for signal vector
    return index of feature columns
    """
    bars = obars.copy()
    nindfeatures = 3*nbands*7 # number of indicator features
    # columns corresponding to the indicator features
    fi = bars.columns.get_loc('demaO0') # first column corresponding to a indicator feature
    nind = fi+nindfeatures # last column of the indicator features
    # standardize
    bars.iloc[:, fi:nind] = bars.iloc[:, fi:nind].fillna(0) #  quantile Transform dont like nans
    bars.iloc[:, fi:nind] = bars.iloc[:, fi:nind].apply(lambda x: x.clip(*x.quantile([0.001, 0.999]).values), axis=0)
    # # signal vector needed columns lets not normalize than
    ibandsgs = [ bars.columns.get_loc('bandsg'+str(j)) for j in range(nbands) ]
    # # can only have values 0, 1, 2 turn it in normalized floats
    # bars.iloc[:, ibandsgs] = ((bars.iloc[:, ibandsgs] - bars.iloc[:, ibandsgs].mean())/
    #                          bars.iloc[:, ibandsgs].std()) # normalize variance=1 mean=0
    id = bars.columns.get_loc('dated')
    bars.iloc[:, list(range(fi,nind))] = quantile_transformer.fit_transform(
        bars.iloc[:, list(range(fi,nind))].values)
    bars.iloc[:, id] = ((bars.iloc[:, id] - bars.iloc[:, id].mean())/
                             bars.iloc[:, id].std()) # normalize variance=1 mean=0
    # dimension of training signal features len first returned list
    #features = nindfeatures+1+nbands
    # return index of feature columns
    return [*ibandsgs, *list(range(fi,nind)), id], bars

def barsRawSignals(obars, window=21, nbands=3):
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
        bars['bandup'+str(i)] = upband
        bars['bandsg'+str(i)] = 0 # signal for this band
        signals = bollingerSignal(price[1:], price[:-1],
                                  upband[1:], upband[:-1], lwband[1:], lwband[:-1])
        bars.loc[1:, 'bandsg'+str(i)] = signals.astype(int) # signal for this band
        inc += 0.5
    bars.dropna(inplace=True)
    return bars

def lastSignal(bars, nbands=3):
    """
    Latest signal - not standardized - used for predicting future.
    That is for each band.
    """
    return bars.loc[:, ['bandsg'+str(i) for i in range(nbands)]].tail(1).values

def barsTargetFromSignals(obars, window=21, nbands=3):
    """
    Create target class by analysing raw bollinger band signals
    those that went true receive 1 buy or 2 sell
    those that wen bad receive 0 hold
    A target class exist for each band.
    An essemble of bands together and summed are a better classifier.
    Note:.
    A day integer identifier column name 'date' must exist prior call.
    That is used to hold positions from passing to another day.
    """
    bars = obars.copy()
    for j in range(nbands): # for each band traverse it
        bars['y'+str(j)] = np.nan
        ibandsg = bars.columns.get_loc('bandsg'+str(j))
        # being pessimistic ... is this right?
        # should i buy at what price? I dont know maybe the typical one is more realistic.
        # but setting the worst case garantees profit in the worst scenario!!
        # better.
        # So i Buy at the highest price and sell at the lowest one.
        yband = traverseBand(bars.iloc[:, ibandsg].values.astype(int),
                                        bars['y'+str(j)].values,
                                        bars.H.values, bars.L.values, bars.date.values)
        bars['y'+str(j)]  = yband

    return bars

def barsFeatured(obars, window=21, nbands=3):
    """
    Create feature columns and return a new dataframe
    """

    bars = obars.copy() # avoid warning
    # needed to reset orders by day
    days = bars.index.to_period('D').asi8
    bars['date'] = days # # integer day from year 1 day 1 gregorian day
    # 7*60 is one day I cannot rely on overalapping days without adding more info
    ## Now assembly X and Y TRAINING vectors
    # **window*nsignal features = X second dimension**
    # ## Signal Features
    bars.RV = np.log(bars.RV+5)
    bars.TV = np.log(bars.TV+5) # to avoid division by zero/inf etc
    # tecnical indicators features
    inc = 0.5
    for column in ['O', 'H', 'L', 'C', 'RV', 'TV', 'OHLC']: # ignore date/dated
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
    # day information signal [0, 1]
    # i think can be faster
    bars['dated'] = 0
    for i, vars in enumerate(bars.groupby(bars.date)):
        day, group = vars
        bars.loc[group.index, 'dated'] = 1 if i%2==0 else 0 # odd or even day change
    bars.dropna(inplace=True)
    return bars


def getTrainingVectors(bars, isgfeatures, window=21, nbands=3, batchn=180):
    """
    receives a standardized dataframe with all feature columns
    """
    # y target class column index
    iybands = [ bars.columns.get_loc('y'+str(j)) for j in range(nbands)]
    # assembly training pairs
    X, y, time = xyTrainingPairs(bars.iloc[:, [*iybands, *isgfeatures]].values, batchn, len(isgfeatures), nbands)
    time = time.astype(int)
    y = y.astype(int)
    return X, y, time

def getForecastVector(bars, isgfeatures, window=21, nbands=3, batchn=180):
    """
    receives a standardized dataframe with all feature columns
    """
    # create prediction vector X
    Xforecast = bars.iloc[-batchn:, isgfeatures]
    Xforecast = Xforecast.values.flatten()
    return Xforecast

# window=21; nbands=3 # number of bbands
def getTrainingForecastVectors(obars, window=21, nbands=3, batchn=180):
    """
    return Xpredict, Xtrain, ytrain

    if Xpredict has signal all zero return None
    """
    bars, signal = barsRawSignals(obars, window, nbands)
    if np.all(signal == 0): # no signal no training
      return signal, None, None, None
    bars = barsFeatured(obars)
    bars = barsTargetFromSignals(bars, window, nbands) # needs day identifier integer
    isgfeatures, bars = standardizeFeatures(bars, nbands); # signal features standardized
    Xforecast, X, y = getTrainingForecastVectorsn(bars, isgfeatures, nbands, batchn)

    return signal, Xforecast, X, y


def fitPredict(X, y, Xp, njobs=None):
    if njobs is None:
        trees = ExtraTreesClassifier(n_estimators=120, verbose=0)#, max_features=300)
    else:
        trees = ExtraTreesClassifier(n_estimators=120, verbose=0, n_jobs=njobs)
    ypred = None
    try:
        # if don't have 3-classes for training cannot train model
        if len(np.unique(y)) == 3:
            trees.fit(X, y)
            ypred = trees.predict_proba(Xp.reshape(1, -1))
    except Exception as e:
        print(e, file=sys.stderr)
        if debug:
            raise(e)
    return ypred

def sumdTarget(y):
    """
    from the minute bars data-frame
    summarize y-target class for all bands
    turning classes 0, 1, 2 in
    -1, 0, 1 (sell, hold, buy)
    works for data-frame
    don't drop nan's when summed by row they become 0
    """
    y = y.copy()
    y[y == 2] = -1
    y = y.sum(axis=1) # summarized prediction
    y[ y < 0] = -1
    y[ y > 0] = +1
    return y

def sumPred(y):
    """
    summarize predictions probabilities
    turn classes 0, 1, 2 in
    -1, 0, 1 (sell, hold, buy)
    works for 3 column array
    """
    y = y.copy()
    y = np.argmax(y, axis=-1) # summarized prediction
    #yprob = np.max(y, axis=-1)
    y[ y == 2] = -1
    return y
