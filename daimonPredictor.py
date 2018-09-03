import os
import sys
import pandas as pd
import numpy as np
import struct
import datetime
import calendar
import time
from Tools.util import progressbar
from Tools import prepareData, torchNNTrainPredict
from Tools import meta5Ibov

def save_prediction(time, direction):
    """ create a file with the list of all predictions executed"""
    with open('executed_predictions.txt', 'a') as f:
        f.write("{:>5} {:5d} \n".format(str(time), direction))

def endmsg():
    print('='*70)

# working path of Expert Advisor Metatrader 5
# save on the metatrader 5 files path that can be read by the expert advisor
#meta5filepath = '/home/andre/.wine/drive_c/users/andre/Application Data/MetaQuotes/Terminal/Common/Files'
#meta5filepath = '/home/andre/.wine/drive_c/users/andre/Application Data/MetaQuotes/Terminal/Common/Files'
meta5filepath = '/home/andre/.wine/drive_c/users/andre/Application Data/MetaQuotes/Terminal/Common/Files'
os.chdir(meta5filepath)
# statistical mean and variance from 2013+2018 used to make data
# with variance < 1 and mean close to 0, that is how it works!
stocks_stats = pd.read_csv('stocks_stats_2018.csv', index_col=0)
# select columns by backtesting best performance accuracy
# ascii file each column divided by spaces,
with open('collumns_selected.txt', 'r') as f:
    columns = f.read()
selected_columns = columns.split(' ')[:-1]
# clear terminal to start
os.system('cls' if os.name == 'nt' else 'clear')
# first last time
last_time = datetime.datetime(year=1970, month=1, day=1)
last_time = np.datetime64(last_time)

while(True): # daemon allways running
    try:
        meta5Ibov.setDataPath(meta5filepath, meta5filepath)
        # don't want masterdf to be reused
        masterdf = meta5Ibov.loadMeta5Data(suffix='RTM1.mt5bin',
                    cleandays=False, preload=False, verbose=False)
        X = meta5Ibov.simpleColumnNames()
        print('Just Read Minute Data File Len: ', len(X))
        print("Last minute: ", X.index.values[-1])
    except Exception as e:
        print('Error reading input file: ', str(e), file=sys.stderr)
        endmsg()
        continue

    # dont make a prediction twice, testing was done this way
    if X.index.values[-1] > last_time:
        last_time = X.index.values[-1]
    else: # dont make a prediction twice
        continue

    missing = meta5Ibov.calculateMissing(meta5Ibov.masterdf)
    print('Missing Data: ', missing, file=sys.stderr)
    # change this to evaluate the percentage of missing data
    if len(X) < (3200): #or missing > 0.015: # cannot make indicators or predictions with
        print('Too few data, cannot model neither predict!', file=sys.stderr)
        endmsg()
        continue # so few data

    # Prepare Data for training classification : creating features etc.
    # stats and selected columns from backtesting
    X, y, Xp = prepareData.GetTrainingPredictionVectors(
        X, targetsymbol='PETR4_C', verbose=False,
        selected=selected_columns, stats=stocks_stats)

    # classifier training and use
    buy = torchNNTrainPredict.TrainPredict(X, y, Xp, verbose=True)

    if buy==0:
        #print('No accuracy entry point to make orders.')
        endmsg()
        continue

    #### SAVE PREDICTION BINARY FILE
    prediction_time = Xp.index.values[-1]
    prediction_time = pd.Timestamp(prediction_time) # to save on txt file
    # save on file
    save_prediction(prediction_time, buy)
    # prediction_time to long
    prediction_time = np.int64(calendar.timegm(prediction_time.timetuple()))
    while(True): # try until is able to write prediction file
        try:
            # Writing a prediction for mql5 read
            with open('prediction.bin','wb') as f:
                # struct of ulong and int 8+4 = 12 bytes
                dbytes = struct.pack('<Qi', prediction_time, np.int32(buy))
                if f.write(dbytes) == 12:
                    print('Just wrote prediction file! time: ', prediction_time, file=sys.stderr)
                    print('prediction direction: ', buy, file=sys.stderr)
                    break
        except:
            continue

    endmsg()
