import pandas as pd
from datetime import datetime
import numpy as np
from matplotlib import pylab as plt
import sys
from matplotlib.patches import Rectangle
import matplotlib.pylab as plt

sys.path.append(r'D:\\Metatrader 5')
sys.path.append(r'F:\\Metatrader 5')

import explotest


def print_percs(name='PERCENTILES', data=None, percs=[0, 1, 10, 50, 90, 99, 100], ffmt='f'):
    """With no params prints PERCENTILES : xx xx"""
    str_fmt = ('{:12.3'+ffmt+'} ')*len(percs);
    str_fmtf0 = ('{:12.0f} ')*len(percs);
    str_fmts = '{:18s} :'
    data_percs = None
    if data is None:
        print(str_fmts.format('PERCENTILES') + str_fmtf0.format(*percs))
    else:
        data_percs = np.percentile(data[~np.isnan(data)], percs)
        print(str_fmts.format(name) + str_fmt.format(*data_percs))
    return data_percs

def money_bar_summary():
    bars = explotest.mbars()
    names = [ name for name in bars.dtype.names ]
    dbars = pd.DataFrame.from_records(bars, exclude=['time'],
                                       index=[datetime.utcfromtimestamp(x) for x in (bars['smsc']*0.001).astype(int)])
    # bars per day
    dbars['yday'] = bars['time']['tm_yday']
    # dbars['one'] = 1 # not needed
    gpb=dbars.groupby(dbars['yday'])
    perday = gpb.count().values.flatten()

    span = (bars['emsc']-bars['smsc'])*0.001
    daysecs = 10*60*3600

    print_percs()
    nticks_percs = print_percs('N. ticks', bars['nticks'])
    dt_percs = print_percs('Time to Previous', bars['dtp'], ffmt='e')
    span_percs = print_percs('Life Span', span, ffmt='e')
    perday_percs = print_percs('N. Per day ', perday)

    fig, axes = plt.subplots(2, 4, figsize=(14,8))
    ############
    axes[0,0].hist(bars['nticks'], bins=80, density=True,
                    range=[nticks_percs[0], nticks_percs[-2]]);
    axes[0,0].set_title('Number of Ticks')
    axes[1,0].hist(bars['nticks'], bins=80, density=True,
                    range=[nticks_percs[0], nticks_percs[-2]],
                   cumulative=True);
    axes[1,0].set_title('Number of Ticks')
    ############
    axes[0,1].hist(bars['dtp'], bins=80, density=True,
                   range=[dt_percs[0], dt_percs[-2]]);
    axes[0,1].set_title('Time to previous (seconds)')
    axes[0,1].set_yscale('log')
    axes[0,1].grid()
    axes[1,1].hist(bars['dtp'], bins=80, density=True, cumulative=True,
                   range=[dt_percs[0], dt_percs[-2]]);
    axes[1,1].set_title('Time to previous (seconds)')
    axes[1,1].grid()
    ############
    axes[0,2].hist(span, bins=80,  density=True, cumulative=False,
                   range=[0, span_percs[-2]]);
    axes[0,2].set_title('Life Span (seconds)')
    axes[0,2].grid()
    axes[0,2].set_yscale('log')
    axes[1,2].hist(span, bins=80,  density=True, cumulative=True,
                   range=[0, span_percs[-2]]);
    axes[1,2].set_title('Life Span (seconds)')
    axes[1,2].grid()
    ############
    axes[0,3].hist(perday, bins=80, );
    axes[0,3].set_title('Bars per Day-y')
    axes[0,3].grid()
    axes[1,3].hist(perday, bins=80,
                   cumulative=True,density=True);
    axes[1,3].set_title('Bars per Day-y')
    axes[1,3].grid()

def variables_summary():
    bars = explotest.mbars()
    ret = explotest.mbars_returns()
    stdret = explotest.mbars_stdev_returns() # average return for ~300 money bars
    sadf  = explotest.mbars_sadf()
    sadfd = explotest.mbars_sadfd() # dispersion of high ADF values
    fd = explotest.mbars_fdiff()

    print_percs()
    wprice_percs = print_percs('MB w.price', bars['wprice'])
    ret_percs = print_percs('MB Returns', ret, ffmt='e')
    stdret_percs = print_percs('Stdev Returns', stdret, ffmt='e')
    sadf_percs = print_percs('SADF on MB', sadf, ffmt='e')
    sadfd_percs = print_percs('SADF Dispers.', sadfd, ffmt='e')
    fd_percs = print_percs('MB Fractional diff', fd, ffmt='e')

    fig, axes = plt.subplots(2, 3, figsize=(14,8))
    axes[0,0].hist(bars['wprice'], bins=100, fc=(0, 0, 1, 0.5),
                    label='Money Bars Weight Price');
    axes[0,0].hist(bars['high'], bins=100, fc=(1, 0, 0, 0.5),
                    label='Money Bars High');
    axes[0,0].hist(bars['low'], bins=100, fc=(0, 1, 0, 0.5),
                   label='Money Bars Low');
    axes[0,0].legend()
    ######
    axes[0,1].hist(ret[~np.isnan(ret)], bins=100, density=True,
                range=[ret_percs[1], ret_percs[-2]]);
    axes[0,1].set_yscale('log')
    axes[0,1].set_title('MB Returns')
    ######
    axes[0,2].hist(stdret[~np.isnan(stdret)], bins=100, density=True,
                range=[stdret_percs[1], stdret_percs[-2]]);
    axes[0,2].set_title('Stdev Returns (Vol. %)')
    #######
    axes[1,0].hist(sadf[~np.isnan(sadf)], bins=100, density=True,
                range=[sadf_percs[1], sadf_percs[-2]]);
    axes[1,0].set_title('SADF on MB');
    #######
    axes[1,1].hist(sadfd[~np.isnan(sadfd)], density=True,
                   range=[sadfd_percs[1], sadfd_percs[-2]]);
    axes[1,1].set_title('SADF Dispers.');
    #######
    axes[1,2].hist(fd[~np.isnan(fd)], bins=100, density=True,
                    range=[fd_percs[1], fd_percs[-2]]);
    axes[1,2].set_title('MB Fractional diff');

def plot_region(minplot=None, maxplot=None):

    bars = explotest.mbars()
    if maxplot is None: # random end at each call
        maxplot = np.random.randint(len(bars))
    if minplot is None:
        minplot = max(0, maxplot-4500)

    ret = explotest.mbars_returns()
    stdret = explotest.mbars_stdev_returns() # average return for ~300 money bars
    sadf  = explotest.mbars_sadf()
    sadfd = explotest.mbars_sadfd() # dispersion of high ADF values
    fd = explotest.mbars_fdiff()
    cumsum  = explotest.mbars_sadf_csum()

    price_percs = np.percentile(bars['wprice'][minplot:maxplot], [0, 100])
    sadf_percs = np.percentile(sadf[~np.isnan(sadf)], [1, 99])
    sadf_percs[0] -= 0.2
    sadf_percs[1] += 0.2

    # day operationally separation
    upentries = np.argwhere(cumsum > 0).flatten()
    dwentries = np.argwhere(cumsum < 0).flatten()
    day_sep = np.diff(bars['inday'])
    day_end = np.argwhere(np.array(day_sep)<0).flatten()
    day_str = np.argwhere(np.array(day_sep)>0).flatten()
    # with rectangles
    w = day_end - day_str # width of rectangles
    miny, maxy = sadf_percs
    rc_x = day_str[:-1] # rectangle x coordinate
    rc_y = np.repeat(miny, len(w)) # rectangle y coordinate
    h = np.repeat(maxy-miny, len(w)) # heigh of rectangle

    fig, axes = plt.subplots(5, 1, sharex=True, figsize=(18,12))
    axes[0].plot(bars['low'], 'r', lw=0.6)
    axes[0].plot(bars['high'], 'b', lw=0.6)
    axes[0].set_ylim(price_percs[0]-0.1, price_percs[1]+0.1)
    axes[0].set_xticks(np.arange(0, len(bars), 250))
    axes[0].set_xlim(minplot, maxplot)
    axes[0].grid()
    axes[1].plot(sadf, 'k', lw=0.8)
    axes[1].plot(upentries, sadf[upentries], 'b^', lw=0.8)
    axes[1].plot(dwentries, sadf[dwentries], 'rv', lw=0.8)
    # create a rectangle patches for every operational window inside a day
    for i, xy in enumerate(zip(rc_x, rc_y)):
        rect = Rectangle(xy,w[i],h[i],linewidth=0.6, edgecolor='r',facecolor='b', alpha=0.6)
        # Add the patch to the Axes
        axes[1].add_patch(rect)
    axes[1].set_ylim(sadf_percs)
    axes[1].grid()
    axes[2].plot(sadfd, lw=0.6)
    axes[2].set_ylim(0, 2.)
    axes[2].grid()
    axes[3].plot(stdret*100, lw=0.6, label='(volatility) stdev min avg. returns (%)')
    axes[3].plot(ret*100, ',k', lw=0.6, label='returns + positive (%)')
    axes[3].set_ylim(-0.01, 0.1)
    axes[3].legend()
    axes[3].grid()
    axes[4].plot(fd, lw=0.6)
    axes[4].grid()
    axes[4].set_ylim(0.09, 0.12)
