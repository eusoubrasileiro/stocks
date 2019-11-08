from io import StringIO
import pandas as pd
import datetime
import numpy as np

def fromMt5OptParam(fname='optNaiveGap'):
    """ read mt5 optimization param *.set file
    return dataframe
    """
    with open(fname+'.set', encoding='utf_16') as f:
        txt = f.read()
    optparamscsv = txt.replace(';', '').replace('||', ';').replace('=', ';').strip()
    optparams = pd.read_csv(StringIO(optparamscsv), skiprows=4, header=None, delimiter=';',
                         names=['param', 'default', 'min', 'step', 'max', 'Use'])
    return optparams

# optparams = fromMt5OptParam()
# optparams.loc[:, 'Use'] = 'Y'
# optparams

def toMt5OptParam(dfoptparams, expert='NaiveGapExpert', fname='optNaiveGapC'):
    """ write mt5 optimization param *.set file
    from dataframe read with `fromMt5OptParam`
    """
    # header with time and expert name
    txt = "; saved on "+datetime.datetime.now().strftime("%Y.%m.%d %H:%M:%S")+'\n'\
              "; this file contains input parameters for testing/optimizing "+expert+" expert advisor\n"\
              "; to use it in the strategy tester, click Load in the context menu of the Inputs tab\n;\n"
    for row in dfoptparams.iterrows():
        sformat = '{:s}={:g}||{:g}||{:g}||{:g}||{:s}\n'
        if np.all(list(map(lambda x: (int(x)-x) == 0, row[1].to_list()[1:-1]))): # check integer row
            sformat = sformat.replace('g', '.0f') # print 'as integers' removing trailing zeros
        txt+=str.format(sformat, row[1][0],
                           *(list(map(float, row[1].to_list()[1:-1]))), row[1][-1])

    with open(fname+'.set', 'w', encoding='utf_16') as f:
        f.write(txt)
    return txt

# toMt5OptParam(optparams)
import numpy as np
import os
import pandas as pd

moneybar_dtype = np.dtype([
    ("bid", np.float64),
    ("ask", np.float64),
    ("mlast", np.float64),
    ("msc", np.int64)
])

# struct MqlTick
#   {
#    datetime     time;          // Time of the last prices update
#    double       bid;           // Current Bid price
#    double       ask;           // Current Ask price
#    double       last;          // Price of the last deal (Last)
#    ulong        volume;        // Volume for the current Last price
#    long         time_msc;      // Time of a price last update in milliseconds
#    uint         flags;         // Tick flags
#    double       volume_real;   // Volume for the current Last price with greater accuracy
#   };

mqltick_dtype = np.dtype([
    ("time", np.int64),
    ("bid", np.float64),
    ("ask", np.float64),
    ("last", np.float64),
    ("volume", np.int64),
    ("msc", np.int64),
    ("flags", np.int32),
    ("vreal", np.float64)
])

def ticksnbars(tickfileinput):
    pass
    # return dataframe of ticks
    #        dataframe of bars

def writecsv_dfticks(ticks):
    pass

def writecsv_dfbars1m(bars):
    pass

def writemqlticks_dfticks(ticks):
    pass
