import os
import sys
import pandas as pd
import numpy as np
import struct
import datetime
import calendar
import time
import argparse
from Tools.util import progressbar
from Tools import prepareData, torchNN
from Tools import meta5Ibov

parser = argparse.ArgumentParser()
parser.add_argument("--delay", type=int, default=10, nargs='?',
            help="delay time between reading data file in seconds (default 10s)")
args = parser.parse_args()

def zeroTime():
    return datetime.datetime(year=1970, month=1, day=1)

def recordMinute(entrytime=0, meta5time=0, sizeread=-1,
        percmiss=-1, daimontime=0, direction=-9, device='unknown'):
    """ record on file/print on stderr minute processed"""
    if entrytime==0:
        entrytime = zeroTime()
    if meta5time==0:
        meta5time = zeroTime()
    if daimontime==0:
        daimontime = zeroTime()
    entrytime = entrytime.strftime("%d/%m/%y %H:%M:%S")
    meta5time = meta5time.strftime("%d/%m/%y %H:%M:%S")
    daimontime = daimontime.strftime("%d/%m/%y %H:%M:%S")
    with open('processedminutes.txt', 'a') as f:
        msg="{:>8s}   {:>8s}   {:>8s}   {:>5d}  {:>5.1f}%  {:>3d} {:>8}".format(
        entrytime, daimontime, meta5time, sizeread, 100*percmiss,
        direction, str(device))
        f.write(msg+'\n')
        print(msg, file=sys.stderr)

# Working Path for Expert Advisor Metatrader 5
# Use same path that can be read by the expert advisor
meta5filepath = '/home/andre/.wine/drive_c/users/andre/Application Data/MetaQuotes/Terminal/Common/Files'
os.chdir(meta5filepath)
# statistical mean and variance from 2013+2018 used to make data
# with variance < 1 and mean close to 0, that is how it works!
stocks_stats = pd.read_csv('stocks_stats_2018.csv', index_col=0)
# selected columns by backtesting best performance accuracy
with open('collumns_selected.txt', 'r') as f: # txt column names divided by spaces
    columns = f.read()
selected_columns = columns.split(' ')[:-1]
# clear terminal to start
os.system('cls' if os.name == 'nt' else 'clear')
# clear folder
try:
    os.system('rm *.mt5bin masterdf.pickle SYMBOLS.pickle processedminutes.txt')
except:
    pass
# first previous time
previoustime = zeroTime()

while(True): # daemon allways running
    try:
        entrytime = datetime.datetime.now()
        meta5Ibov.setDataPath(meta5filepath, meta5filepath, verbose=False)
        # don't want masterdf to be reused
        masterdf = meta5Ibov.loadMeta5Data(suffix='RTM1.mt5bin',
                    cleandays=False, preload=False, verbose=False)
        X = meta5Ibov.simpleColumnNames()
        meta5time = pd.Timestamp(X.index.values[-1])
    except Exception as e:
        print('Exception while reading *RTM1.mt5bin files: ',
                str(e), file=sys.stderr)
        recordMinute(entrytime)
        time.sleep(args.delay) # wait a bit before next read
        continue

    # dont make a prediction twice, backtesting was done this way
    if meta5time > previoustime:
        previoustime = meta5time
    else: # dont make a prediction twice
        time.sleep(args.delay) # wait a bit before next read
        continue
    sizeread = len(X) # samples read
    # effective missing data
    percmiss = meta5Ibov.calculateMissing(meta5Ibov.masterdf[-3200:])
    # change this to evaluate the percentage of missing data? future!
    if sizeread < 3200: # cannot make indicators or predictions with
        print('Too few data, cannot work!', file=sys.stderr)
        recordMinute(entrytime, meta5time, sizeread, percmiss)
        continue # too few data
    # Prepare data for training classification : creating features etc.
    # stats and selected columns from backtesting
    X, y, Xp = prepareData.GetTrainingPredictionVectors(
        X, targetsymbol='PETR4_C', verbose=False,
        selected=selected_columns, stats=stocks_stats)
    # classifier training and use
    buy = torchNN.TrainPredictDecide(X, y, Xp, verbose=True)
    device = torchNN.getDevice() # get cpu or cuda
    if buy==0: # no entry point
        daimontime = datetime.datetime.now()
        recordMinute(entrytime, meta5time, sizeread, percmiss, daimontime, buy, device)
        continue
    # Create Prediction Binary File 12 bytes
    predtime = pd.Timestamp(meta5time) # to save on txt file
    # prediction time to long
    predtime = np.int64(calendar.timegm(predtime.timetuple()))
    while(True): # try until is able to write prediction file
        try: # write prediction for meta5 read
            with open('prediction.bin','wb') as f:
                # struct of ulong and int 8+4 = 12 bytes
                dbytes = struct.pack('<Qi', predtime, np.int32(buy))
                if f.write(dbytes) == 12:
                    break
        except: # keep trying read
            continue
    # end
    daimontime = datetime.datetime.now()
    recordMinute(entrytime, meta5time, sizeread, percmiss, daimontime, buy, device)
