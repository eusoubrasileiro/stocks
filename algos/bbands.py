import pandas as pd
import numpy as np
from numba import jit
import sys
from sklearn import preprocessing
from sklearn.ensemble import ExtraTreesClassifier
import talib as ta
from matplotlib import pyplot as plt

debug=True
quantile_transformer = preprocessing.QuantileTransformer(
    output_distribution='normal', random_state=0)

@jit(nopython=True) # 1000x faster than using padas for loop
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
                # the previous batch o/f signals will be saved with this class (hold)
                yband[buyindex] = 0
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
def xyTrainingPairs(df, window, nsignal_features=8, nbands=6):
    """
    assembly the TRAINING vectors X and y

    default signal features number is 8:
        based on 6 y columns plus the 6+2 of the original data-frame  (above)
    """
    X = np.zeros((len(df), nsignal_features*window))*np.nan # signal training vector 8xwindow
    y = np.zeros(len(df))*np.nan
    time = np.zeros(len(df)) # let it be float after we convert back to int
    nt = 0 # number of training vectors
    for i in range(window, df.shape[0]):
        for j in range(nbands): # each band look at the ys' target class
            if df[i , j] == df[i, j]: # if y' is not nan than we have a training pair X, y
                # X feature vector is the last window (signals + askv and bidv 8 dimension)
                X[nt, :] = df[i-window:i, -nsignal_features:].flatten()
                y[nt] = df[i, j]
                time[nt] = i # index that represent date-time from the original data-frame
                nt+=1
    return X[:nt], y[:nt], time[:nt]

# window=21; nbands=3 # number of bbands
def getTrainingForecastVectors(bars, window=21, nbands=3, verbose=False):
    """
    return Xpredict, Xtrain, ytrain

    if Xpredict has signal all zero return None
    """
    #del bars.S
    #bars['OHLC'] = symbols.apply(lambda row: np.mean([row['O'], row['H'], row['L'], row['C']]), axis=1)
    bars['OHLC'] = np.nan # typical price
    bars.OHLC.values[:] = np.mean(bars.values[:,0:4], axis=1) # 1000x faster
    # needed to reset orders by day
    bars['date'] = bars.index.date
    bars.date = bars.apply(lambda x: x.date.toordinal(), axis=1) # integer day from year 1 day 1 gregorian day
    # 7*60 is one day I cannot rely on overalapping days without adding more info
    #window = 90 # 2 hours trying not rely on previous days
    inc = 0.5
    # if verbose plotting
    lastn=-500
    for i in range(nbands):
        upband, sma, lwband =  ta.BBANDS(bars.OHLC.values, window*inc)
        bars['bandlw'+str(i)] = lwband
        bars['bandup'+str(i)] = upband
        inc += 0.5
    bars.dropna(inplace=True)

    # can improve here too slw with conditions
    if verbose:
        figure = plt.figure(figsize=(15,7))
        plt.subplot(2, 1, 1)
        plt.plot(bars.OHLC.values[lastn:], 'k-')
    for i in range(nbands):
        bars['bandsg'+str(i)] = np.nan # signal for this band
        # buy signal condition combination
        bars['bandlwabv'] = bars.OHLC > bars['bandlw'+str(i)] # above lower band now
        bars['bandlwblw'] = bars.OHLC <= bars['bandlw'+str(i)] # bellow lower band before
        # sell signal condition combinatoin
        bars['bandupblw'] = bars.OHLC < bars['bandup'+str(i)] # bellow upper band now
        bars['bandupabv'] = bars.OHLC >= bars['bandup'+str(i)] # above upper band before
        # use condition combination to get buy and sell signals for each band
        buy = np.logical_and(bars['bandlwabv'].values[1:], bars['bandlwblw'].values[:-1])
        buy = np.append(False, buy) # first index cannot be used due past comparison
        bars.loc[buy, 'bandsg'+str(i)] = 1
        sell = np.logical_and(bars['bandupblw'].values[1:], bars['bandupabv'].values[:-1])
        sell = np.append(False, sell) # first index ...
        bars.loc[sell, 'bandsg'+str(i)] = 2 # set signal
        bars.loc[~(buy|sell), 'bandsg'+str(i)] = 0 # everything else is hold
        # plot bands
        if verbose:
            plt.plot(bars['bandup'+str(i)].values[lastn:], 'b--', lw=0.3)
            plt.plot(bars['bandlw'+str(i)].values[lastn:], 'b--', lw=0.3)
    # clean-up temp rows
    bars.drop(['bandupblw', 'bandupabv'], axis=1, inplace=True) # clean-up temp rows
    if verbose:
        plt.subplot(2, 1, 2)
        for i in range(nbands):
            plt.plot(bars['bandsg'+str(i)].values[lastn:], '+', label='band '+str(i))
            plt.ylabel('signal-code')
        plt.legend()
        plt.savefig('bbands.png')
        plt.close()
    ### Latest Signal - not standardized - used for predicting future
    signal = bars.loc[:, ['bandsg0', 'bandsg1', 'bandsg2']].tail(1).values
    #### Traverse bands
    for j in range(nbands): # for each band you might need to save training-feature-target vectors
        # save batch from here to behind (batch-size)
        bars['y'+str(j)] = np.nan
    for j in range(nbands): # for each band traverse it
        ibandsg = bars.columns.get_loc('bandsg'+str(j))
        iyband = bars.columns.get_loc('y'+str(j))
        # being pessimistic ... is this right?
        # should i buy at what price? I dont know maybe the typical one is more realistic.
        # but setting the worst case garantees profit in the worst scenario!!
        # better.
        # So i Buy at the highest price and sell at the lowest one.
        yband = traverseBand(bars.iloc[:, ibandsg].values.astype(int),
                                        bars.iloc[:, iyband].values,
                                        bars.H.values, bars.L.values, bars.date.values)
        bars.iloc[:, iyband] = yband
    ## Now assembly X and Y TRAINING vectors
    # **window*nsignal features = X second dimension**
    # ## Signal Features
    bars.RV = np.log(bars.RV+5)
    bars.TV = np.log(bars.TV+5) # to avoid division by zero/inf etc
    # tecnical indicators features
    inc = 0.5
    for column in bars.columns[:7]: # ignore date/dated
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
    bars['dated'] = 0
    for i, vars in enumerate(bars.groupby(bars.date)):
        day, group = vars
        bars.loc[group.index, 'dated'] = 1 if i%2==0 else 0 # odd or even day change
    nindfeatures = 3*nbands*7 # number of indicator features
    #nbands=nbands
    nfeatures = nindfeatures+1+nbands #175
    # print('number of feature signals', nfeatures)
    # columns corresponding to the indicator features
    fi = len(bars.columns)-(nbands*7*3+1)# first column corresponding to a indicator feature
    nind = fi+nindfeatures # last column of the indicator features
    # print(fi, nind)
    # standardize
    bars.iloc[:, fi:nind] = bars.iloc[:, fi:nind].fillna(0) #  quantile Transform dont like nans
    bars = bars.apply(lambda x: x.clip(*x.quantile([0.001, 0.999]).values), axis=0)
    # signal vector needed columns
    ibandsgs = [ bars.columns.get_loc('bandsg'+str(j)) for j in range(nbands) ]
    # y target class
    iybands = [ bars.columns.get_loc('y'+str(j)) for j in range(nbands)]
    id = bars.columns.get_loc('dated')
    # can only have values 0, 1, 2 turn it in normalized floats
    bars.iloc[:, ibandsgs] = ((bars.iloc[:, ibandsgs] - bars.iloc[:, ibandsgs].mean())/
                             bars.iloc[:, ibandsgs].std()) # normalize variance=1 mean=0
    bars.iloc[:, list(range(fi,nind))] = quantile_transformer.fit_transform(
        bars.iloc[:, list(range(fi,nind))].values)
    bars.iloc[:, id] = ((bars.iloc[:, id] - bars.iloc[:, id].mean())/
                             bars.iloc[:, id].std()) # normalize variance=1 mean=0
    X, y, time = xyTrainingPairs(bars.iloc[:, [*iybands, *ibandsgs, *list(range(fi,nind)), id]].values, window, nfeatures, nbands)
    time = time.astype(int)
    y = y.astype(int)

    # is there a signal?
    Xforecast = None
    if np.any(signal != 0): # create prediction vector X
        Xforecast = bars.iloc[-window:, [*ibandsgs, *list(range(fi,nind)), id]]
        Xforecast = Xforecast.values.flatten()

    #assert len(y[y == 1]) == len(y[y == 2])

    return signal, Xforecast, X, y

def fitPredict(X, y, Xp):
    trees = ExtraTreesClassifier(n_estimators=70, verbose=0, max_features=300)
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
