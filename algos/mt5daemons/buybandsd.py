import os
import sys
import pandas as pd
import numpy as np
import struct
import datetime
import calendar
import time
import argparse
import sys
from .. import bbands, meta5Ibov
from ..util import progressbar

# Working Path for Expert Advisor Metatrader 5
# Use same path that can be read by the expert advisor
meta5filepath = '/home/andre/.wine/drive_c/users/andre/Application Data/MetaQuotes/Terminal/Common/Files'
debug=True
testingpath = r"C:\Users\alferreira\Documents\stocks\algos\tests"
meta5filepath = testingpath
cname="WIN@"# "WING19"

parser = argparse.ArgumentParser()
parser.add_argument("--delay", type=int, default=10, nargs='?',
            help="delay time between reading data file in seconds (default 10s)")
args = parser.parse_args()

def zeroTime():
    return datetime.datetime(year=1970, month=1, day=1)

def recordMinute(entrytime=0, meta5time=0, sizeread=-1,
        percmiss=-1, daimontime=0, direction=0):
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
        msg="{:>8s}   {:>8s}   {:>8s}   {:>5d}  {:>5.1f}%  {:>3d}".format(
        entrytime, daimontime, meta5time, sizeread, 100*percmiss,
        direction)
        f.write(msg+'\n')
        print(msg, file=sys.stderr)

os.chdir(meta5filepath)
# clear terminal to start
os.system('cls' if os.name == 'nt' else 'clear')
# clear folder
try:
    os.system('rm *.mt5bin masterdf.pickle symbols.pickle processedminutes.txt')
except:
    pass
# first previous time
previoustime = zeroTime()

while(True): # daemon allways running
    try:
        entrytime = datetime.datetime.now()
        meta5Ibov.setDataPath(meta5filepath, meta5filepath, verbose=False)
        # don't want masterdf to be reused
        loaded = meta5Ibov.loadMeta5Data(suffix='RTM1.mt5bin',
                    cleandays=False, preload=False, verbose=False)
        bars = meta5Ibov.getSymbol('WIN@')
        meta5time = pd.Timestamp(bars.index.values[-1])
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
    sizeread = len(bars) # samples read
    # effective missing data
    percmiss = meta5Ibov.calculateMissing(meta5Ibov.masterdf[-3200:])
    # change this to evaluate the percentage of missing data? future!
    if sizeread < 3200: # cannot make indicators or predictions with
        print('Too few data, cannot work!', file=sys.stderr)
        recordMinute(entrytime, meta5time, sizeread, percmiss)
        continue # too few data

    # Prepare data for training classification : creating features etc.
    window=21
    signal, Xp, X, y  = bbands.getTrainingForecastVectors(bars, window, 3)

    if Xp is None: # no entry point
        daimontime = datetime.datetime.now()
        recordMinute(entrytime, meta5time, sizeread, percmiss, daimontime, 0)
        continue

    # training window latest data
    twindow  = window*4
    ypred = bbands.fitPredict(X[-twindow:], y[-twindow:], Xp)

    if ypred is None:
        # no entry point
        daimontime = datetime.datetime.now()
        recordMinute(entrytime, meta5time, sizeread, percmiss, daimontime, 0)
        continue

    # decide base on prediction and probability clip
    ypred = np.argmax(ypred, axis=-1)
    yprob = np.max(ypred, axis=-1)
    ypred = ypred[0] # turn in integer
    if yprob < 0.8: # must be 80% sure this is the class
        # no entry point
        daimontime = datetime.datetime.now()
        recordMinute(entrytime, meta5time, sizeread, percmiss, daimontime, ypred)
        continue

    # turn in -1/1 class + quantity
    qty = np.count_nonzero(signal)  # or all going up or
    # all going down
    ypred = -1 if ypred == 2 else ypred# turn in -1+1 x qunatity
    ypred = ypred*qty # quantity

    # Create Prediction Binary File 12 bytes
    predtime = pd.Timestamp(meta5time) # to save on txt file
    # prediction time to long
    predtime = np.int64(calendar.timegm(predtime.timetuple()))
    while(True): # try until is able to write prediction file
        try: # write prediction for meta5 read
            with open('prediction.bin','wb') as f:
                # struct of ulong and int 8+4 = 12 bytes
                dbytes = struct.pack('<Qi', predtime, np.int32(ypred))
                if f.write(dbytes) == 12:
                    break
        except Exception as e: # keep trying read
            print(e, ypred, file=sys.stderr)
            if debug:
                raise(e)
            continue
    # end
    daimontime = datetime.datetime.now()
    recordMinute(entrytime, meta5time, sizeread, percmiss, daimontime, ypred)
