import numpy as np
import matplotlib.pyplot as plt
import pandas as pd
from sklearn import preprocessing
from sklearn.ensemble import RandomForestClassifier, ExtraTreesClassifier
from scipy.stats import mode
import talib as ta


def plot_quotes(quotes, dataframe): 
    """
    Plot all columns in the daframe, shared axis x
    """   
    f, axr = plt.subplots(len(quotes), sharex=True, figsize=(15,10))
    for i, ax in enumerate(axr):
        dataframe.iloc[:,i].plot(ax=axr[i])
        axr[i].set_ylabel(quotes[i])

def create_binary_zig_zag_class_sm(X, target_quote, span=10, smooth=3, plot=True):
    """"Binary Class UP or DOWN  trend created based on simple moving average tool
    if bellow moving average is going down if above is going up
    span is the number of points used for the moving average
    smooth remove spikes of one or two points crossing moving average and going back fast 
    (3 samples median filter)
    """ 
    sd = X[target_quote]
    # Talib ewm because and it is proper alligned to the right
    sdsm = pd.Series(ta.EMA(sd.values, span), index=sd.index)
    # signal change points crossings
    supdown = sd-sdsm
    supdown[supdown<0] = 0
    supdown[supdown>0] = 1

    # remove spikes of one/n sample using statistic mode, the most commom value on the window
    # don't want to train the model on flutuations much smaller than SPAN window
    # low pass filter on it 
    #impossible to filter or smooth without using information from the future

    #supdown = supdown.rolling(smooth, center=True).apply(lambda x: mode(x, nan_policy='omit')[0][0])
    # calculate the new pivot index from the smotheed version 
    srpivots= supdown.dropna()
    pivots = (srpivots.shift(1) != srpivots) # same original time indexes 
    def app(x):
        if x:
            return 1
        else:
            return np.nan
    pivots = pivots.apply(lambda x : x if x else np.nan)
    pivots = pivots.dropna()
    pivots = pivots[1:] # remove the first pivots it is just showing the left border

    # help plotting 
    srindex = pd.DataFrame(index=sd.index)
    srindex['ix'] = np.arange(0, len(srindex))

    if plot:
        fig, arx = plt.subplots(2, sharex=True, figsize=(18,10))
        fig.subplots_adjust(hspace=0)
        # upp
        arx[0].plot(sdsm.values, 'g', lw=1.)
        arx[0].plot(sd.values, '-', lw=1.0)
        #arx[0].plot(ipivots, sd[pivots.index].values, '.r') # plot just the intersection points
        arx[0].plot(srindex.loc[pivots.index, 'ix'].values, sd[pivots.index].values, '.r') # plot just the intersection points
        arx[0].vlines(srindex.loc[pivots.index, 'ix'].values, -0.5+sd.values.min(), 0.5+sd.values.max(), linewidth=1, linestyle='dashed')
        arx[0].grid()        
        # down
        #arx[1].plot(, '.r', lw=0.5)                  
        arx[1].plot(supdown.values, '-g', lw=1)
        arx[1].vlines(srindex.loc[pivots.index, 'ix'].values, -0.5, 1.5, linewidth=1, linestyle='dashed')
        arx[1].set_ylim(-0.2, 1.2)
        arx[1].grid()      

    X['du'] = supdown
    # that's the pivots to return with the original index
    return sd[pivots.index]


def make_shift_binary_class(X, shift, plot=True):
    """
    Also mark as NaN the last n samples shifted
    """

    # last not NaN Sample
    # for controlling plot, plot 50 samples before for seeing the displacement
    # caused by the shift
    last_sample = X['du'].dropna(inplace=False).index[-50-shift] 

    if plot:
        f, axr = plt.subplots(1, sharex=True, figsize=(15,1))
        X.loc[last_sample:, 'du'].plot(style='.-b', ax=axr)
        axr.set_ylim(-0.15, 1.15)
    
    X['du_shifted'] = X['du'].shift(-shift) #shift = 1 # shift -1 sample to the left

    # SET NAN last n shifted samples That cannot 
    # be used since they come from FUTURE VALUES we dont know yet.
    X.loc[ X.tail(shift).index, 'du_shifted'] = np.nan

    if plot:
        X.loc[last_sample:, 'du_shifted'].plot(style='.-r', ax=axr)
        axr.set_ylim(-0.15, 1.15)


    du_shifted = X['du_shifted'] # classification shifted class
    # wont be inserted on this table
    X.drop('du_shifted', axis='columns', inplace=True)
    # wont be needed
    X.drop('du', axis='columns', inplace=True)

    return du_shifted


def create_indicators(df, target_variable, span=60):
### all windows of moving averages must be alligned to the right... 
### otherwise we will be fitting forcing data to the model
    df.drop(target_variable, axis='columns', inplace=True)
    
    def tofloat64(df):
        for col in df.columns:
            if df[col].dtype != np.float64:
                df.loc[:, col] = df.loc[:, col].values.astype(np.float64)

    tofloat64(df) # TALIB works only with real numbers (np.float64)
    # add two indicators hour and day of week
    df['hour'] = df.index.hour.astype(np.float64)
    df['weekday']  = df.index.dayofweek.astype(np.float64)

    quotes = df.columns
    for quote in quotes:
        df['ema_2'+quote] = ta.EMA(df[quote].values, span*0.5)
        df['ema_3'+quote] = ta.EMA(df[quote].values, span*3)      
        df['ema_5'+quote] = ta.EMA(df[quote].values, span*5)  
        df['dema_2'+quote] = df[quote] - df['ema_2'+quote]
        df['dema_3'+quote] = df[quote] - df['ema_3'+quote]
        df['dema_5'+quote] = df[quote] - df['ema_5'+quote]
        df['macd'+quote] = ta.MACD(df[quote].values)[0]
        df['macd_tiny'+quote] = ta.MACD(df[quote].values, span, span*0.5)[0]
        df['macd_mean'+quote] = ta.MACD(df[quote].values, span, span*0.3)[0]
        df['macd_double'+quote] = ta.MACD(df[quote].values, span*2, span)[0]
        df['rsi_2'+quote] = ta.RSI(df[quote].values, span*0.5)
        df['rsi_3'+quote] = ta.RSI(df[quote].values, span*3)
        df['rsi_5'+quote] = ta.RSI(df[quote].values, span*5)
    for i in range(len(quotes)-1):
        df[quotes[i]+'+'+quotes[i+1]] = df[quotes[i]]+df[quotes[i+1]]

def calculate_corr_and_remove(df, serie_binary_class, verbose=True):
    """
    calculate correlation with binary classification serie
    collumns less then 5% are removed and wont be used on
    random forest
    Remove after high correlate variables...Todo
    """
    df['target_variable_dup'] = pd.Series(serie_binary_class, index=df.index)
    # drop NANs cannot be used for correlation
    dfcpy = df.dropna(inplace=False)
    corr = dfcpy.corr().ix['target_variable_dup', :-1]
    corr = corr.apply(lambda x: np.abs(x)) # make all correlations positive
    corr.sort_values(ascending=False, inplace=True)
    if verbose:
        print(corr.head(5))
        print(corr.tail(5))
    df.drop('target_variable_dup', axis='columns', inplace=True)
    df.drop(corr.index[corr < 0.05], axis='columns', inplace=True)

def prepareDataForClassification(dataset):
    """
    generates categorical to be predicted column, 
    attach to dataframe 
    and label the categories
    """
    le = preprocessing.LabelEncoder()
    
    dataset['UpDown'] = dataset['target_variable_dup'] # target variable up and down shifted
    dataset.UpDown = le.fit(dataset.UpDown).transform(dataset.UpDown) #create classes for randomForest    
    # last two are the target quote and variable. IGNORE THEM
    features = dataset.columns[0:-2] 
    X = dataset[features]    
    y = dataset.UpDown    
    
    return X, y


def performCV_analysis(X_train, y_train, windown, nanalysis=-1, nvalidate=2, algorithm='ET'):    
    """
    Cross-Validate the model    

    train the model using a sliding window of size windown

    the training is progressive cumulative

    and will produce nanalysis (array) of size number of window movements
    """

    # nvalidate  number of samples to use for validation
    pshifts = round(X_train.index.size-windown+1-nvalidate) # possible shifts N-windown+1    

    print('Size train set: ', X_train.shape)
    print('samples in each window: ', windown)

    if nanalysis < 0:
        # number of samples default, equal number of 
        # windows inside data
        nanalysis = round((X_train.index.size-nvalidate)/windown)
        print('number of analysis: ', nanalysis)
    elif nanalysis >= pshifts:
        print("Error")
        return

    if algorithm == 'RF': # classification model
    # random forest binary classifier
        clf = RandomForestClassifier(n_estimators=700, n_jobs=-1)
    else:
    # extra tree binary classifier
        clf = ExtraTreesClassifier(n_estimators=700, n_jobs=-1)

    step = int(round(pshifts/nanalysis)) # window shift step in samples
    #diff = pshifts-
    shifts = range(0, pshifts, step)
    accuracies = np.zeros(len(shifts)) # store the result of cross-validation
    iaccuracies = np.zeros(len(shifts)) # index of the last sample in the window

    f = FloatProgress(min=0, max=nanalysis)
    display(f)

    # sliding window with step sample of shift     
    for j, i in enumerate(shifts): # shift, classify and cross-validate
        f.value = j # counter of analysis

        # train using the window samples
        X_trainFolds = X_train[i:i+windown]
        y_trainFolds = y_train[i:i+windown]
        # test using the samples just after the window       
        X_testFold = X_train[i+windown+1:i+windown+nvalidate]
        y_testFold = y_train[i+windown+1:i+windown+nvalidate]    

        clf.fit(X_trainFolds, y_trainFolds)        
        accuracies[j] = clf.score(X_testFold, y_testFold)
        iaccuracies[j] = i+windown

    return (iaccuracies, accuracies), clf


def performQuickTrainigAnalysis(X_train, y_train, nvalidate, clfmodel=None, windown=60, nanalysis=-1):    
    """
    Cross-Validate the model    
    train the model using a sliding window of size windown
    the training is not progressive cumulative, a new tree every time
    and will produce nanalysis (array) of size number of window movements

    if nanalysis is -1 


    """

    # nvalidate  number of samples to use for validation
    pshifts = round(X_train.index.size-windown+1-nvalidate) # possible shifts N-windown+1    

    print('Size train set: ', X_train.shape)
    print('samples in each window: ', windown)
    print('maximum analysis possible: ', pshifts)


    if nanalysis < 0:
        # number of samples default, equal number of 
        # windows inside data
        nanalysis = round((X_train.index.size-nvalidate)/windown)
    elif nanalysis >= pshifts:
        print("Error")
        return

    print('number of analysis: ', nanalysis)
    print('percent of analysis to data space: (%) ', 100*nanalysis/pshifts)



    if clfmodel is None : # in the absence create one
        clfmodel = ExtraTreesClassifier(n_estimators=700, n_jobs=-1)

    step = int(round(pshifts/nanalysis)) # window shift step in samples
    #diff = pshifts-
    if step < 1:
        print("Error")
        return
    shifts = range(0, pshifts, step)

    accuracies = np.zeros(len(shifts)) # store the result of cross-validation
    iaccuracies = np.zeros(len(shifts)) # index of the last sample in the window

    f = FloatProgress(min=0, max=nanalysis)
    display(f)

    # sliding window with step sample of shift     
    for j, i in enumerate(shifts): # shift, classify and cross-validate
        f.value = j # counter of analysis

        # train using the window samples
        X_trainFolds = X_train[i:i+windown]
        y_trainFolds = y_train[i:i+windown]
        # test using the samples just after the window       
        X_testFold = X_train[i+windown+1:i+windown+nvalidate]
        y_testFold = y_train[i+windown+1:i+windown+nvalidate]    

        clfmodel.fit(X_trainFolds, y_trainFolds)        
        accuracies[j] = clfmodel.score(X_testFold, y_testFold)
        iaccuracies[j] = i+windown

    print("percent of data used ", 100*float(iaccuracies[-1]/X_train.index.size))

    return (iaccuracies, accuracies), clfmodel


def performRandomQuickTrainigAnalysis(X_train, y_train, nvalidate, windown=60, nanalysis=-1):    
    """
    Cross-Validate the model    

    train the model using a sliding window of size windown

    the training is not progressive cumulative, a new tree every time

    and will produce nanalysis (array) of size number of window movements
    """

    # nvalidate  number of samples to use for validation
    pshifts = round(X_train.index.size-windown+1-nvalidate) # possible shifts N-windown+1    

    print('Size train set: ', X_train.shape)
    print('samples in each window: ', windown)
    print('maximum analysis possible: ', pshifts)


    if nanalysis < 0:
        # number of samples default, equal number of 
        # windows inside data
        nanalysis = round((X_train.index.size-nvalidate)/windown)
    elif nanalysis >= pshifts:
        print("Error")
        return

    print('number of analysis: ', nanalysis)
    print('percent of analysis to data space: (%) ', 100*nanalysis/pshifts)

    clfmodel = ExtraTreesClassifier(n_estimators=700, n_jobs=-1)

    step = int(round(pshifts/nanalysis)) # window shift step in samples
    #diff = pshifts-
    shifts = range(0, pshifts, step)
    accuracies = np.zeros(len(shifts)) # store the result of cross-validation
    iaccuracies = np.zeros(len(shifts)) # index of the last sample in the window

    f = FloatProgress(min=0, max=nanalysis)
    display(f)

    # sliding window with step sample of shift     
    for j, i in enumerate(shifts): # shift, classify and cross-validate
        f.value = j # counter of analysis
        
        # create a random begin for the window
        i = np.random.randint(0, X_train.index.size-windown-nvalidate)

        # train using the window samples
        X_trainFolds = X_train[i:i+windown]
        y_trainFolds = y_train[i:i+windown]
        # test using the samples just after the window       
        X_testFold = X_train[i+windown+1:i+windown+nvalidate]
        y_testFold = y_train[i+windown+1:i+windown+nvalidate]    

        clfmodel.fit(X_trainFolds, y_trainFolds)        
        accuracies[j] = clfmodel.score(X_testFold, y_testFold)
        iaccuracies[j] = i+windown

    return (iaccuracies, accuracies), clfmodel
