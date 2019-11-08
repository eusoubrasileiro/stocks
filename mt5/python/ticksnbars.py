# Making Bars 1M from Ticks to be used on backtesting
# 'Every tick based on real tick'
#
# Creates *.csv to be loaded on MT5 Symbol:
#    - 1M Bars
#    - Ticks
#
# - To fix creation of fake ticks by Metatrader 5 (everytick)
#   - Will remove
#       - start of day ticks without volume - only requotes
#       - end of day ticks without volume - only requotes
#  - Mt5 creates new ticks because of those
#       using the stupid 'EveryTick' methodology
#
#  - After market also data cannot be included
#  - MT5 also creates fake ticks because of empty bars (in the end of day)
#     - created because there is a gap between end of market and after market
#     - get minute bounds of days when there is the last non 0 volume bar
#     - use those bounds to clip Ticks already cleaned-up
#
#   Note:
#   OHLC are on executed prices (deals) that means tick.last when volume > 0
#   Otherwise bars would be crazy up and down on ask or bid nonsense.
#
#

import pandas as pd
import numpy as np
import datetime
import argparse

parser = argparse.ArgumentParser()
parser.add_argument("--file", "-f", type=str, required=True)
parser.add_argument("-mqltick", "-m", dest="mqltick", default=False, action="store_true",
                    help="also creates binary file of array of mt5 mqltick suffix _mql5tick.bin")
args = parser.parse_args()
tickfileinput = args.file
# tickfileinput = 'PETR4_Ticks_201910010800_201910241759.csv' # exported from MT5

ticks = pd.read_csv(tickfileinput,
            names=['date', 'time', 'bid', 'ask', 'last', 'vol'], skiprows=1, delimiter='\t')

newticks = pd.DataFrame()

print('Processing Ticks First Time...')
# must be by day to avoid forwarding ask, bid, last to next day
for gticksday in ticks.groupby(ticks.date):
    gticksday = gticksday[1]
    # ask, bid, last copy forward last valid values - might be wrong but better option
    gticksday.loc[:, ['bid', 'ask', 'last']] = gticksday.loc[:, ['bid', 'ask', 'last']].fillna(method='ffill')
    # the rest of last nan should be 0 no negotiation only re-quotes
    # also 0 for volumes
    gticksday.loc[:,['last', 'vol']] = gticksday.loc[:,['last', 'vol']].fillna(0)
    # fixing creation of fake ticks by Metatrader 5 (everytick)
    # remove start of day ticks without volume - only requotes
    size = gticksday.shape[0]
    start_index = 0
    for i in range(size):
        if gticksday.vol.iloc[i] != 0:
            start_index = i
            break
    # remove end of day ticks without volume - only requotes
    end_index = size-1
    for i in range(size-1, 0, -1):
        if gticksday.vol.iloc[i] != 0:
            end_index = i
            break
    newticks = newticks.append(gticksday.iloc[start_index:end_index])

newticks.reset_index(drop=True, inplace=True)
ticks = newticks

# create datetime index
dtindex = pd.to_datetime(ticks.apply(lambda x: str(x[0])+ ' ' +str(x[1]), axis=1), unit='ns')
ticks.set_index(dtindex, inplace=True)

print('Creating Bars ...')
bars1m_all = pd.DataFrame()
for group in ticks.groupby(ticks.date):
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
    # get valid end of day bars - removing 0 volume bars
    end_index = 0
    for i in range(size):
        if bars1m.vol.iloc[i] == 0:
            end_index = i
            break
    bars1m_all = bars1m_all.append(bars1m.iloc[:end_index], sort=False)
    print('day last valid bar: ', bars1m_all.index[-1])

bars1m_all['date'] = bars1m_all.index.date
bars1m_all['time'] = bars1m_all.index.time
bars1m_all['spread'] =  1

print('Processing Ticks Second Time (Clipping)...')
# Correct Now Ticks to be compatible with these Bars
# Avoiding creation of fake ticks (nonsense values)
# Get day boundaries
daybounds = []
for barsday in bars1m_all.groupby(bars1m_all.date):
    barsday = barsday[1]
    daybounds.append([barsday.index[0], barsday.index[-1]])

newticks = pd.DataFrame()
for i, ticksday in enumerate(ticks.groupby(ticks.date)):
    ticksday = ticksday[1]
    newticks = newticks.append(ticksday.loc[daybounds[i][0]:
                                 daybounds[i][1] + datetime.timedelta(minutes=1)])

print('Writting Files...')
### Write Ticks Csv
### Make it metatrader 5 Format
ticks = newticks
mt5colnames = "<DATE>	<TIME>	<BID>	<ASK>	<LAST>	<VOLUME>".split('\t')
ticks.columns = mt5colnames
ticks.to_csv(tickfileinput.split('.csv')[0]+'_tickmt5.csv',
             index=False, sep='\t')
### Write Bars 1M Csv
### Make it metatrader 5 Format
mt5colnames = "<DATE>	<TIME>	<OPEN>	<HIGH>	<LOW>	<CLOSE>	<TICKVOL>	<VOL>	<SPREAD>".split("\t")
bars1m_all = bars1m_all.loc[:, ['date', 'time', 'open', 'high', 'low', 'close', 'nticks', 'vol', 'spread']]
bars1m_all.columns = mt5colnames
bars1m_all.to_csv(tickfileinput.split('.csv')[0]+'_m1barmt5.csv',
             index=False, sep='\t')

if args.mqltick:
    print('Writting MqlTick Array Binary File...')

print('Done')
