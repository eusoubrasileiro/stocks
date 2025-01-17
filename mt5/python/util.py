from io import StringIO
import pandas as pd
import datetime
import numpy as np
import sys
import os

def progressbar(it, prefix="", size=80):
    count = len(it)
    def _show(_i):
        x = int(size*_i/count)
        sys.stdout.write("%s[%s%s] %i/%i\r" % (prefix, "#"*x, "."*(size-x), _i, count))
        sys.stdout.flush()

    _show(0)
    for i, item in enumerate(it):
        yield item
        _show(i+1)
    sys.stdout.write("\n")
    sys.stdout.flush()


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

moneybar_dtype = np.dtype([
    ("bid", np.float64),
    ("ask", np.float64),
    ("mlast", np.float64),
    ("msc", np.int64)
])


def ticksnbars(tickfilepath, verbose=False):
    """
    Creates DataFrames of Ticks and 1M Bars cleaned.

    * tickfilepath: file path of Tick file (*.csv)
        Metatrader 5->View->Symbols->
        [Select desired symbol - must be colored yellow]
        Ticks->Export Ticks

    * Returns: 2 data frames:
     - Ticks
     - 1M Bars

    - Can be used to create clean ticks for further processing.
    - Used for backtesting 'Every tick based on real tick'.

    Metatrader 5 backtesting problems solved using this:

    - To fix creation of fake ticks by Metatrader 5 (everytick)
    - Mt5 creates new ticks because of those
      using the stupid 'EveryTick' methodology
    - MT5 creates fake ticks because of empty bars (in the end of day)

    Implementation Details:

    - Will remove
      - start of day ticks without volume - only requotes
      - end of day ticks without volume - only requotes
    - After market also data cannot be included
    Because there is a gap between end of market and after market
    - get minute bounds of days when there is the last non 0 volume bar
    - use those bounds to clip Ticks already cleaned-up

    Reminder:
    - OHLC are on executed prices (deals) that means tick.last when volume > 0
    Otherwise bars would be crazy up and down on ask or bid nonsense.

    """
    if verbose:
        print('Reading and Parsing...')

    tickfilepath = os.path.abspath(tickfilepath) # get the absolute file path in case
    try:
        ticks = pd.read_csv(tickfilepath,
                names=['date', 'time', 'bid', 'ask', 'last', 'vol', 'flags'], skiprows=1, delimiter='\t',
                       parse_dates={'dtindex' : [0, 1]}, infer_datetime_format=True, keep_date_col=True)
        # 10x faster if use pare_dates here to convert (on reading) to ms to_datetime
    except NotImplementedError as e:
        print("Wrong number of columns!")
        raise(e)

    newticks = pd.DataFrame()

    if verbose:
        print('Processing Ticks First Time...')
    # must be by day to avoid forwarding ask, bid, last to next day
    for i, gticksday in progressbar(list(enumerate(ticks.groupby(ticks.date))), "Daily Ticks: ", 60):
        gticksday = gticksday[1]
        # ask, bid, last copy forward last valid values - might be wrong but better option
        gticksday.loc[:, ['bid', 'ask', 'last']] = gticksday.loc[:, ['bid', 'ask', 'last']].fillna(method='ffill')
        # the rest of last nan should be 0 no negotiation only re-quotes
        # also 0 for volumes
        gticksday.loc[:,['last', 'vol']] = gticksday.loc[:,['last', 'vol']].fillna(0)
        # fixing creation of fake ticks by Metatrader 5 (everytick)
        # remove start of day ticks without volume - only requotes
        size = gticksday.shape[0]
        start_index = size-1 # case everything is == 0
        for i in range(size):
            if gticksday.vol.iloc[i] != 0:
                start_index = i
                break
        # remove end of day ticks without volume - only requotes
        end_index = 0 # case everything is == 0
        for i in range(size-1, 0, -1):
            if gticksday.vol.iloc[i] != 0:
                end_index = i
                break
        # all ticks with volume == 0 -- day skipped
        if(end_index < start_index):
            continue
        newticks = newticks.append(gticksday.iloc[start_index:end_index])

    del ticks

    newticks.reset_index(drop=True, inplace=True)
    ticks = newticks

    # create datetime index
    ticks.set_index(ticks.dtindex, inplace=True)
    del ticks['dtindex']

    if verbose:
        print('Creating Bars ...')

    bars1m_all = pd.DataFrame()
    for i, group in enumerate(ticks.groupby(ticks.date)):
        resampled1m = group[1].resample('1T')
        openfilled = resampled1m['last'].max().fillna(method='ffill') # copy last 'open' forward
        openfilled.fillna(0, inplace=True)
        # for those minutes where last were nan use previous valid last as open-price
        bars1m = resampled1m['last'].ohlc()
        bars1m.loc[ bars1m.isnull().any(axis=1), :] = np.tile(openfilled.loc[bars1m.isnull().any(axis=1)], (4, 1)).T
        bars1m['vol'] = resampled1m['vol'].sum()
        bars1m['nticks'] = 0
        # get ticks with deals (vol>0) to get 'tick_volume'
        bar1m_deals = group[1].loc[ group[1]['vol'] > 0 ]
        tick_volume = bar1m_deals.resample('1T')['last'].count()
        bars1m.loc[tick_volume.index, 'nticks'] = tick_volume.values
        # remove every 1 minute without volume -- metatrader does that
        # metatrader doesnt require all minutes be present
        bars1m.drop(bars1m[bars1m.vol == 0].index, inplace=True)
        bars1m_all = bars1m_all.append(bars1m, sort=False)
        if verbose:
            print('day valid bars: ', bars1m.index[0], '-', bars1m.index[-1].time())

    bars1m_all['date'] = bars1m_all.index.date
    bars1m_all['time'] = bars1m_all.index.time
    bars1m_all['spread'] =  1

    if verbose:
        print('Processing Ticks Second Time (Clipping)...')
    # Correct Now Ticks to be compatible with these Bars
    # Avoiding creation of fake ticks (nonsense values)
    # Get day boundaries
    daybounds = []
    for barsday in bars1m_all.groupby(bars1m_all.date):
        barsday = barsday[1]
        daybounds.append([barsday.index[0], barsday.index[-1]])

    newticks = pd.DataFrame()
    for i, ticksday in progressbar(list(enumerate(ticks.groupby(ticks.date))), "Daily Ticks Clipping: ", 60):
        ticksday = ticksday[1]
        newticks = newticks.append(ticksday.loc[daybounds[i][0]:
                                     daybounds[i][1] + datetime.timedelta(minutes=1)])

    return (newticks, bars1m_all)
    # return dataframe of ticks
    #        dataframe of bars

def writecsv_tickmt5(dfticks, ticksfilename):
    ### Write Ticks Csv
    ### Make it metatrader 5 Format
    dfticks = dfticks.copy() # avoid modifications on orignal
    mt5colnames = "<DATE>	<TIME>	<BID>	<ASK>	<LAST>	<VOLUME>	<FLAGS>".split('\t')
    dfticks.columns = mt5colnames
    dfticks.to_csv(ticksfilename+'_tickmt5.csv',
                 index=False, sep='\t')

def writecsv_m1barmt5(dfbars, barsfilename):
    ### Write Bars 1M Csv
    ### Make it metatrader 5 Format
    dfbars = dfbars.copy() # avoid modifications on orignal
    mt5colnames = "<DATE>	<TIME>	<OPEN>	<HIGH>	<LOW>	<CLOSE>	<TICKVOL>	<VOL>	<SPREAD>".split("\t")
    dfbars = dfbars.loc[:, ['date', 'time', 'open', 'high', 'low', 'close', 'nticks', 'vol', 'spread']]
    dfbars.columns = mt5colnames
    dfbars.to_csv(barsfilename+'_m1barmt5.csv',
                 index=False, sep='\t')

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
    ("time_msc", np.int64),
    ("flags", np.int32),
    ("volume_real", np.float64)
])

def create_nprecords(dfticks):
    dfticks = dfticks.copy() # avoid modifications on orignal
    # Create mqltick fake flags but this is unacessary figure out now
    # for not to say useless  only flag_buy and flag_sell would be useful
    # but they are rare
    # // 2      TICK_FLAG_BID –  tick has changed a Bid price
    # // 4      TICK_FLAG_ASK  – a tick has changed an Ask price
    # // 8      TICK_FLAG_LAST – a tick has changed the last deal price
    # // 16     TICK_FLAG_VOLUME – a tick has changed a volume
    # // 32     TICK_FLAG_BUY – a tick is a result of a buy deal  -- cannot be calculated
    # // 64     TICK_FLAG_SELL – a tick is a result of a sell deal  -- cannot be calculated
    #flags = np.not_equal(dfticks.iloc[1:, [2, 3, 4, 5]].values,
    #                     dfticks.iloc[:-1, [2, 3, 4, 5]].values)
    #flags = np.sum(flags*np.array([2, 4, 8, 16]), axis=-1)
    dfticks['flags'] = np.int32(0) # to be created or not
    #dfticks.loc[1:, 'flags'] = flags
    # the first tick is by definition above created when at least volume
    # changed from 0 to something > 0 so  set accordingly
    #dfticks.loc[0, 'flags'] = np.int32(16)
    dfticks['vreal'] = dfticks['vol'].astype(np.float64)
    dfticks['vol'] = dfticks['vol'].astype(np.int64)
    dfticks['ms'] = (dfticks.index.astype(np.int64)//1000000) # datetime unit='ns' nano e9 seconds to ms e3
    dfticks['time'] = dfticks['ms']//1000 # ms to seconds
    dfticks = dfticks.iloc[:, [1, 2, 3, 4, 5, 8, 6, 7]]
    # make names match exactly to mql5 struct
    dfticks.columns = ["time", "bid", "ask", "last", "volume", "time_msc", "flags", "volume_real"]
    mqlticks =  dfticks.to_records(index=False)
    return mqlticks

def writebin_mql5ticks(dfticks, mqlticksfilename):
    dfticks = dfticks.copy() # avoid modifications on orignal
    nprecords_mqlticks = create_nprecords(dfticks)
    nprecords_mqlticks.tofile(mqlticksfilename+'_mqltick.bin')
