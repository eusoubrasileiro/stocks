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
from util import progressbar
import Meta5_Ibov_Load as meta5load
import torch as th
import torch.nn.functional as F
import copy

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

def TorchModelPredict(model, X):
    y_prob = model(X)
    y_pred = th.argmax(y_prob, 1)
    y_pred = y_pred.to("cpu")
    return y_prob, y_pred.data.numpy()

def tensor_shuffle(X, y, size):
    """better use the dataset api in the future"""
    i = np.random.randint(X.shape[0]-size)
    return X[i:i+size], y[i:i+size]

def TrainTorchNet(X, y, X_s, y_s, input_size, model=None, nepochs=30, device="cuda",
                  batch_size=256, verbose=True):
    """
    X, y         : vectors for training
    X_s, y_s     : vector for validation (scoring the model)
    the best model with smaller error in validation will be returned
    after the nepochs of training
    """
    if model == None:
        model = th.nn.Sequential(th.nn.Linear(input_size, 1024), th.nn.ReLU(), th.nn.Dropout(.1),
                                 th.nn.Linear(1024,280), th.nn.ReLU(), th.nn.Dropout(.3),
                                 th.nn.Linear(280, 70), th.nn.ReLU(),  th.nn.Dropout(.05),
                                 th.nn.Linear(70, 2), th.nn.Softmax(dim=1)) # dim == 1 collumns add up to 1 probability
        criterion = th.nn.CrossEntropyLoss()
        model = model.to(device)
        #weight_decay regularization not applied
        optimizer = th.optim.Adam(model.parameters(), lr=0.1) # learning rate optimal

    # Decay LR by a factor of 0.1 every 10 epochs
    scheduler = lr_scheduler.StepLR(optimizer, step_size=15, gamma=0.1)

    best_model = None
    best_lossv = 1000.
    best_error = 100.

    for t in range(nepochs):

        X_t, y_t = tensor_shuffle(X, y, batch_size)

        y_pred = model(X_t)
        # Compute and print loss
        lossy = criterion(y_pred, y_t)
        # Zero gradients, perform a backward pass, and update the weights.
        optimizer.zero_grad()
        lossy.backward()
        optimizer.step()
        # Model Evaluation
        model.eval()
        # calculate error complement of accuracy
        # on training set
        y_pred = th.argmax(y_pred, 1)
        error_train = th.nn.functional.mse_loss(
            y_pred.float(), y_t.float())
        # on validation set
        y_pred = model(X_s)
        lossv = criterion(y_pred, y_s)
        y_pred = th.argmax(y_pred, 1)
        error_validation = th.nn.functional.mse_loss(
            y_pred.float(), y_s.float())
        ### error on validation set next 60 samples / minutes
        ### todo: better tecniques of early stopping, early stopping
        ### is a regularization so other techinques might be better?
        ### or easier?
        if lossv < best_lossv: # now loss for validation is smaller
            # lets save this network
            best_lossv = lossv
            best_errorv = error_validation
            best_errort = error_train
            best_model = copy.deepcopy(model.state_dict()) # save the actual weights

        if verbose:
            if t%int(nepochs/5) == 0:
                # on the loss values could apply a kind of normalization due the fact that they
                # don't have same size of samples BUT just BEAR ON MIND
                # loss error on validation set absolute value has NOTHING to do with
                # with the absolute value of the training set
                print("iteration : {:6d} l training: {:.2f} l validation: {:.2f}"
                      " Err trainging :{:.2f}  Err validation ; {:.3f}".format(
                      t, lossy, lossv, error_train, error_validation))

        model.train()
        scheduler.step()

    # return the best model in the validation and the accuracy
    best_errorv = best_errorv.to("cpu").item()
    best_errort = best_errort.to("cpu").item()
    model.load_state_dict(best_model)
    #print(best_errort, best_errorv)
    # return the model and accuracy of training and validation
    return model, best_errort, best_errorv

# parameters
nforecast=120
nvalidation=nforecast # to validate the model prior prediction
ntraining = 5*8*60 # 5*8 hours before for training - 1 week
nwindow = nvalidation+nforecast+ntraining

def TorchNNTrainPredict(X, y, X_p, input_size, verbose=False, device='cuda'):
    """
    Using Pytorch train binary classifier NN based on X, y and classify Xp

    * input_size : X.shape[1]

    Return : {1 or 0, score}
        if criteria of model accuracy is reached otherwise returns None
    """
    # training samples are the first ones
    X_t = X[:ntraining]
    y_t = y[:ntraining]
    # control samples for scoring (validation of the model)
    # SCORE using the nforecast samples just after the window
    X_s = X[ntraining:ntraining+nvalidation]
    y_s = y[ntraining:ntraining+nvalidation]

    # return model, accuracy
    clfmodel, errort, errorv = TrainTorchNet(X_t, y_t, X_s, y_s,
                                                   input_size, device=device, verbose=False)
    # accurary of the model if higher than 90% we can predict
    if verbose:
        print(" error t. : ", errort, " error v.: ",errorv)
    # we cannot predict unless the model is 90%+ accurate
    # accurate on validation. on training we accept 70%+
    if errorv > 0.1 or errort > 0.3:
        del clfmodel # cleanup memory gac can get it back
        return

    probability, prediction = TorchModelPredict(clfmodel, X_p)
    down = abs(prediction.sum()-len(prediction))/len(prediction)
    up = abs(1-down)

    if up > 0.9 or down > 0.9: # only if certain by 90% that will go up or down
        buy = up > down
        if buy: # buy
            buy = 1
        else: # sell
            buy = -1

    del clfmodel # cleanup memory gac can get it back
    return buy, errorv

import time

# meta5filepath = "/home/andre/.wine/drive_c/Program Files/Rico MetaTrader 5/MQL5/Files"
# meta5filepath = "/home/andre/PycharmProjects/stocks/data"
meta5filepath = '/home/andre/.wine/drive_c/users/andre/Application Data/MetaQuotes/Terminal/Common/Files'
os.chdir(meta5filepath)
# don't want it to be reused
try:
    os.remove('masterdf.pickle')
    os.remove('SYMBOLS.pickle')
except:
    pass

while(True):
    try:
        meta5load.Set_Data_Path(meta5filepath, meta5filepath)
        masterdf = meta5load.Load_Meta5_Data(suffix='RTM1.mt5bin', cleandays=False) # suffix='RTM1.mt5bin'
        X = meta5load.FixedColumnNames()
        print('Just Read Minute Data File Len:', len(X))
        print("last minute: ", X.index.values[-1])
        # don't want it to be reused
        meta5load.masterdf = None
        meta5load.SYMBOLS = None
        time.sleep(3) # wait 3 seconds
        os.chdir(meta5filepath)
        os.remove('masterdf.pickle')
        os.remove('SYMBOLS.pickle')
        #X = X[:3200]
    except Exception as e:
        print(e)
        continue

    if len(X) < (3200-7): # cannot make indicators or predictions with
        continue # so few data

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

    ###########################
    # Training and prediction
    ###########################

    from torch.optim import lr_scheduler

    times = Xp.index # same times, last time will saved for prediction
    X = X.values.astype(np.float32) # due Float tensors
    Xp = Xp.values.astype(np.float32)
    y = y.values.astype(np.int64) # due Cross Entropy Loss requeires Tensor Long
    input_size = X.shape[1]

    device = th.device("cuda" if th.cuda.is_available() else "cpu")

    X = th.tensor(X)
    X = X.to(device)
    y = th.tensor(y)
    y = y.to(device)
    Xp = th.tensor(Xp)
    Xp = X.to(device)

    result = TorchNNTrainPredict(X, y, Xp, input_size, verbose=True, device=device)

    buy = 0
    if result is None:
        print('No Prediction')
        continue
    else:
        buy, score = result
        print(buy, score, times.values[-1])

    import pandas as pd
    import datetime
    import calendar
    import time
    import struct

    #### SAVE PREDICTION
    # create long datetime
    times = times.map(lambda x: calendar.timegm(x.timetuple()))
    times = times.values.astype(np.int64)
    #buy = buy.astype(np.int32)

    # save on the metatrader 5 files path that can be read by the expert advisor
    os.chdir('/home/andre/.wine/drive_c/users/andre/Application Data/MetaQuotes/Terminal/Common/Files')

    try:
        # Writing a prediction for mql5 read
        # https://docs.python.org/2/library/struct.html
        with open('prediction.bin','wb') as f:
            # struct of ulong and int 8+4 = 12 bytes
            dbytes = struct.pack('<Qi', times[-1], buy)
            f.write(dbytes)
        print('Just wrote prediction')
    except:
        continue
