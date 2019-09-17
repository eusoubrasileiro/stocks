from matplotlib import pyplot as plt

def viewBands(bars, window=21, nbands=3, lastn=-500, str_bandsg='bandsg'):
    """Visualize signal for each band"""
    price = bars.OHLC.values
    plt.figure(figsize=(15,7))
    plt.subplot(2, 1, 1)
    plt.plot(price[lastn:], 'k-')
    for i in range(nbands):
        # plot bands
        plt.plot(bars['bandup'+str(i)].values[lastn:], 'b--', lw=0.3)
        plt.plot(bars['bandlw'+str(i)].values[lastn:], 'b--', lw=0.3)
    plt.subplot(2, 1, 2)
    for i in range(nbands):
        plt.plot(bars[str_bandsg+str(i)].values[lastn:], '+', label=str_bandsg+str(i))
        plt.ylabel('signal-code')
        plt.ylim([-1.25, 1.25])
    plt.legend()
