import numpy as np
from numba import jit, njit, prange



# backtest
# is not fully object oriented / for loop based on
# a tick event. not seeing reason (performance)
# to do that way yet

### open collumns
EP=0 # enter price
QT=1 # quantity
DR=2 # direction 1/-1 buy/sell
TP=3 # take profit
SL=4 # stop loss
OT=5 # order time
### close collumns
CP=6 # close price
SS=7 # sucess status
CT=8 # close time
MB=9 # money back
DY=10 # day identifier


ordersbookdict = {"EP" : 0, "QT" : 1, "DR" : 2, "TP": 3, "SL" : 4,
                      "OT" : 5, "CP" : 6, "SS" : 7, "CT" : 8, "MB" : 9, "DY": 10}

@njit(nogil=True)
def MaxStocks(enterprice, dstop, capital, riskap=0.015):
    """
    Maximum number number of stocks to buy/sell based on:
     - Enter Price
     - Stop variation in percent decimal
     - Capital available to support this operation
     - Risk-Apettite in percent decimal.
          How much you accept to loose of you capital.
          Default 1.5%
    Be reminded that Number of Stocks MUST BE in 100s.
    """
    loss = dstop*enterprice # loss per share
    riskap *= capital # available to loose
    return int((riskap/loss)/100)*100 # round stocks to 100's

@njit(nogil=True, parallel=True)
def MoneyBack(enter_price, close_price,
            quantity, direction, ir=0.2, cost_order=12):
    """
    Money back after an order is closed day trade
    direction=1/-1 buy/sell == long/short

    Costs:
    20% taxes day trade
    12 R$ cost per order (may be improved!!)

    Just for stocks yet!
    """
    # delta money (profit or loss) (positive or negative)
    # already deduce cost of order
    dm = ((close_price-enter_price)*
    quantity*direction-cost_order)
    if dm > 0: # deduce taxes day trade 20%
        dm *= (1-ir)
    return enter_price*quantity + dm

@njit(nogil=True, parallel=True)
def ExecuteOrder(k, book, price, quantity, direction,
            dgain, dstop, time, day, cost_order=12):
    """
    place order on book:book at position k
    return money spent
    """
    book[k, EP] = price # enter price
    book[k, QT] = quantity # number of contracts
    book[k, DR] = direction # 1 for buy, -1 for sell
    # dgain/dstop are in percent decimal
    book[k, TP] = price*(1+direction*dgain) # take profit
    book[k, SL] =  price*(1-direction*dstop) # stop loss
    # time order was placed, (should be datetime?)
    # no it can be just the index of the price on the rates array the "i"
    book[k, OT] = time
    book[k, DY] = day # day identifier

    return quantity*price + cost_order

@njit(nogil=True, parallel=True)
def CloseOrder(io, obook, irow, cbook,
                 time, close_price, bytime=0):
    """
    close order at row irow in the open book of orders with io valid entries
    copy this order to the cbook array passed
    finally rearrange the open book of orders excluding this row

    bytime : order was closed due time stop
    """
    moneyback = 0
    # copy to closed book + fill 3 collumns more
    cbook[:] = obook[irow, :]
    cbook[CP] = close_price # close price
    cbook[CT] = time # close time
    # money back (will be incremented)
    moneyback = MoneyBack(cbook[EP], cbook[CP],
                                cbook[QT], cbook[DR])
    cbook[MB] = moneyback
    # more money in the end than in the start
    # Status codes -2 stop, -1 stop time, 1 gain time, 2 gain
    if moneyback > cbook[EP]*cbook[QT]:
        cbook[SS] = 2-bytime # sucess
    else:
        cbook[SS] = -2+bytime # failed
    # now rearrange book of open orders
    # if the book has only one order nothing needs to be done
    if io > 1: # if there are more orders copy the last to this row
        obook[irow, :] = obook[io-1, :] # io will be also decremented

@njit(nogil=True, parallel=True)
def tryCloseOrder(io, obook, irow, cbook,
                 time, high, low):
    """
    Try to close row specifying one order from the open book.
    If closed it is placed on the closed book row passed.

    Consider worst case cenario where we sell on low
    and buy on high. tentative to compensate for M1, OHLC in backtesting
    return 1 if closed or -1 contrary
    high, low is from the current minute M1
    """
    close_price = 0 # different from zero only when
    # it will be closed
    #if direction: # is a buy order
    if obook[irow, DR] == 1: # buy order
        ## close with sucess or close with failure
        if low >= obook[irow, TP] or low <= obook[irow, SL]:
            close_price = low
    else: # is a sell order
        ## close with sucess or close with failure:
        if  high <= obook[irow, TP] or high > obook[irow, SL]:
            close_price = high
    if close_price > 0:
        # open book, closed book, time, close_price
        CloseOrder(io, obook, irow, cbook, time, close_price)
        return 1 # it was closed

    # nothing to close, nothing happened!
    return 0

@njit(nogil=True, parallel=True)
def tryCloseOrders(io, obook, ic, cbook,
             time, high, low):
    """
    evalue if order should be closed (on obook)
    io, ic are indexes of the last entry on each book
    """
    money=0 # money increment due to all orders
    status=0
    # closed in this loop
    for i in range(io):
        status = tryCloseOrder(io, obook, i, cbook[ic, :], time, high, low)
        if status > 0: # the order was closed
            # money back (will be incremented)
            money += cbook[ic, MB] # money back
            ic += 1 # one more on closed book
            io -= 1 # one less on open book

    return money, io, ic

@njit(nogil=True, parallel=True)
def tryClosebyTime(io, obook, ic, cbook,
             time, end_day, high, low, exptime):
    """
    evalue if order should be closed (on obook)
    io, ic are indexes of the last entry on each book

    expire (close)  orders if it has passed already (expire_time) (default'90') minutes or
    if it is less than 1 hour for the end of the day (end_day)

    consider worst case cenario where we sell on low
    and buy on high. tentative to compensate for M1, OHLC in backtesting
    return True if closed:
    high, low is from the current minute M1
    """
    money=0 # money increment due to all orders closed

    # closed in this loop
    for i in range(io):
        # time is to end operations (end of session is near)
        if time >= int(end_day-15):
            # worst case scenario
            close_price = low if obook[i, DR] == 1 else high
            # open book, closed book, time, close_price
            CloseOrder(io, obook, i, cbook[ic, :], time, close_price, 1)
            # money back (will be incremented)
            money += cbook[ic, MB] # money back
            ic += 1 # one more on closed book
            io -= 1 # one less on open book
        # this order expired by time : more than (60'++) minutes passed
        elif time - int(obook[i, OT]) >  exptime:
                # worst case scenario
                close_price = low if obook[i, DR] == 1 else high
                # open book, closed book, time, close_price
                CloseOrder(io, obook, i, cbook[ic, :], time, close_price, 1)
                # money back (will be incremented)
                money += cbook[ic, MB] # money back
                ic += 1 # one more on closed book
                io -= 1 # one less on open book

    return money, io, ic

@njit(nogil=True, parallel=True)
def isEqual(x, value):
    n = x.size
    for i in range(n):
        if x[i] != value:
            return -1
    return 1

@njit(nogil=True, parallel=True)
def nextGuess(guess_time, iguess, time):
    """"find the first time_index bigger or
    equal than `time` in the guess book"""
    n = guess_time.shape[0]
    # find the next time index after index time
    for t in range(iguess, n):
        if guess_time[t, 1] >= time:
            return t
    # couldn\'t find any. time to stop simulation
    return -1

@njit(nogil=True, parallel=True)
def actualMoney(io, open_book, money, H, L):
    """"calculate actual money on open orders + pouch / pocket"""
    for i in range(io): # for all open orders calculate money now
        if open_book[i, DR] == 1: # buy (allways worst case scenario)
            money += MoneyBack(open_book[i,EP], L,
                                open_book[i,QT], open_book[i,DR])
        else:
            money += MoneyBack(open_book[i,EP], H,
                                open_book[i,QT], open_book[i,DR])
    return money


# orders open on the last hour
@njit(nogil=True, parallel=True)
def norderslastHour(time, io, obook,
        ic,  cbook, perdt):
    """ number of orders openned on the last perdt time delta-hour-minute """
    norders=0
    for i in range(io): # for all open orders sum those open on the nminutes last hour
        if obook[i, OT] > time - perdt: # open on the last nminutes hour
            norders += 1
    for i in range(ic): # for all closed orders sum those open on the nminutes last hour
        if cbook[i, OT] > time - perdt: # open on the last nminutes hour
            norders += 1
    return norders


@njit(nogil=True)
def Simulator(rates, guess_book, book_orders_open, book_orders_closed,
               capital=60000., maxorders=12, norderperdt=3, perdt=15,
               minprofit=300., expected_var=0.008, exp_time=2*60,
               rwr=3., riskap=0.015):
    """
    guess_book contains : {time index, direction, index endday, index startday}
    array of orders to be placed at {time index},

    the first and last {time index} must be
    syncronized and included in the array  of rates

    rwr:
        reward-risk-ratio default 3 (3:1) gain/stop ratio
        stop is expected_var/rwr and gain is expected_var
    expected_var :
        expected variation in decimal percent is the expected average volatility

    riskap:
        risk-appetite percentage of money you risk to loose per order
    """
    n = rates.shape[0] # minute-by-minute prices
    dgain = expected_var # gain variation
    dstop = dgain/rwr # stop variation using reward-to-risk ratio
    money = capital # initial money
    norder_day=0 # number of orders already executed on this day
    # system variables
    # keeep track of money evolution
    moneyprogress = np.zeros(n)
    iro = 0 # real book of orders open index "i"
    irc = 0 # real book of orders close index "i"
    # commanding orders index, none yet executed
    iguess = 0 # indexes of guess orders executed done
    i = 0
    for i in range(n):
        # get the next guess based on this "i" time_index (might be the same)
        iguess = nextGuess(guess_book, iguess, i)
        # get the current price (allways worst case scenario), and the end of the day
        H, L, iday_end, iday_start = (rates[i, 0], rates[i, 1],
                                            int(rates[i, 2]), int(rates[i, 3]))
        if i < iday_start+exp_time: # no orders in the first xx hours/minutes
            # track money evolution
            moneyprogress[i] = actualMoney(iro, book_orders_open, money, H, L)
            continue
        if iro > 0: # Try to Close previosly open order
            moneyback, iro, irc = tryCloseOrders(iro, book_orders_open, irc,
                                book_orders_closed, i, H, L) # close by hit stop/gain
            money += moneyback # money back to the pouch
            moneyback, iro, irc = tryClosebyTime(iro, book_orders_open, irc,
                                book_orders_closed, i, iday_end, H, L, exp_time) # close by expiring time or day
            money += moneyback # money back to the pouch

        # it is not time to place orders anymore (end of session is near)
        if i == (iday_end-exp_time):
            # go to the next guess after this day
            iguess = nextGuess(guess_book, iguess, iday_end+1)
            norder_day = 0
        # is there any order to be done at this time_index
        elif i == guess_book[iguess, 1] and norder_day <= maxorders:
            # no more than n orders open per hour perdt
            if norderslastHour(i, iro, book_orders_open,
                irc, book_orders_closed, perdt) >= norderperdt:
                moneyprogress[i] = actualMoney(iro, book_orders_open, money, H, L)
                continue;
            # get the direction (buy or sell)
            buy = guess_book[iguess, 0]
            # enter price or H or L (allways worst case scenario)
            enter_price = H if buy==1 else L
            # calculate how much to buy spreaded over max orders per day
            acmoney = actualMoney(iro, book_orders_open, money, H, L)
            quantity = int((acmoney/(maxorders*enter_price)/100))*100 # 100's
            if quantity > 0: # has money to spend
                #  max number of stocks based on risk appetite
                maxstocks = MaxStocks(enter_price, dstop, acmoney, riskap)
                quantity = maxstocks  if quantity > maxstocks else  quantity
                moneyspent = ExecuteOrder(iro, book_orders_open,
                            enter_price, quantity, buy,
                            dgain, dstop, i, iday_start)
                iro +=1 # new entry on book
                money -= moneyspent
                norder_day += 1
        # Close orders that suffer hit stop in the same minute
        if iro > 0:
            moneyback, iro, irc = tryCloseOrders(iro, book_orders_open, irc,
                                book_orders_closed, i, H, L) # close by hit stop/gain
            money += moneyback # money back to the pouch
        # track money evolution
        moneyprogress[i] = actualMoney(iro, book_orders_open, money, H, L)
    return moneyprogress, irc, iro

@njit(nogil=True, parallel=True)
def argnextGT(array, i, value):
    """
    find the next number in array starting from index i
    that's Greater or Equal `value` return it's index
    """
    for j in range(i, array.size):
        if array[j] > value:
            return j
    return array.size # not found return size

@njit(nogil=True, parallel=True)
def argnextLE(array, i, value):
    """
    find the next number in array starting from index i
    that's Less or Equal `value` return it's index
    """
    for j in range(i, array.size):
        if array[j] <= value:
            return j + i
    return 0 # not found return 0
