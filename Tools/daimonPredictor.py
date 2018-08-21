import os
import datetime
import calendar
import struct
import time
from util import progressbar
import Meta5_Ibov_Load as meta5load
import prepareData
import torchNNTrainPredict

# meta5filepath = "/home/andre/.wine/drive_c/Program Files/Rico MetaTrader 5/MQL5/Files"
# meta5filepath = "/home/andre/PycharmProjects/stocks/data"

# working path of Expert Advisor Metatrader 5
# save on the metatrader 5 files path that can be read by the expert advisor
meta5filepath = '/home/andre/.wine/drive_c/users/andre/Application Data/MetaQuotes/Terminal/Common/Files'
os.chdir(meta5filepath)
# clear terminal to start
os.system('cls' if os.name == 'nt' else 'clear')

def endmsg():
    print('='*70)
    time.sleep(3) # wait 3 seconds

while(True): # daemon allways running
    try:
        meta5load.Set_Data_Path(meta5filepath, meta5filepath)
        # don't want masterdf to be reused
        masterdf = meta5load.Load_Meta5_Data(suffix='RTM1.mt5bin',
                    cleandays=False, preload=False) # suffix='RTM1.mt5bin'
        X = meta5load.FixedColumnNames()
        print('Just Read Minute Data File Len: ', len(X))
        print("Last minute: ", X.index.values[-1])
    except Exception as e:
        print('Error reading input file: ', e)
        endmsg()
        continue

    if len(X) < (3200): # cannot make indicators or predictions with
        print('Too few data, cannot model neither predict!')
        endmsg()
        continue # so few data

    # Prepare Data for training classification : creating features etc.
    # file with list of selected columns with less than 95% correlation
    collumns_file = os.path.join(meta5filepath, 'collumns_selected.txt')
    X, y, Xp = prepareData.GetTrainingPredictionVectors(X,
        targetsymbol='PETR4_C', verbose=False, correlated=collumns_file)

    # classifier training and use
    result = torchNNTrainPredict.TrainPredict(X, y, Xp, verbose=True)

    buy = 0
    if result is None:
        print('No accuracy entry point to make orders.')
        endmsg()
        continue
    else:
        buy, score = result
        print(buy, score, Xp.index.values[-1])

    #### SAVE PREDICTION BINARY FILE
    prediction_time = Xp.index.values[-1]
    prediction_time = pd.Timestamp(prediction_time)
    # prediction_time to long
    prediction_time = np.int64(calendar.timegm(prediction_time.timetuple()))
    while(True): # try until is able to write prediction file
        try:
            # Writing a prediction for mql5 read
            with open('prediction.bin','wb') as f:
                # struct of ulong and int 8+4 = 12 bytes
                dbytes = struct.pack('<Qi', prediction_time, -1)
                if f.write(dbytes) == 12:
                    print('Just wrote prediction file! Lastime: ', prediction_time)
                    break
        except:
            continue
    endmsg()
