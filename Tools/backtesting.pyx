#from libc.math cimport exp, sqrt
# Import Cython definitions for numpy
import numpy
cimport numpy
cimport cython
from cpython cimport array

#DTYPE = numpy.float
#ctypedef numpy.float_t double
from cython.parallel cimport prange

#__all__ = [
#    '_apply_damping',
#    '_step_elastic_sh',
#    '_step_elastic_psv',
#    '_xz2ps',
#    '_nonreflexive_sh_boundary_conditions',
#    '_nonreflexive_psv_boundary_conditions',
#    '_nonreflexive_scalar_boundary_conditions',
#    '_step_scalar',
#    ]

# is not fully object oriented / for loop based on
# a tick event. not seeing reason (performance)
# to do that way yet
# missing day start day end ... but i will receive

cdef double expected_var = 0.008 # 3* = profit 1* = stop

cdef int ExpTime=2*60

### open collumns
cdef int EP=0 # enter price
cdef int QT=1 # quantity
cdef int DR=2 # direction 1/-1 buy/sell
cdef int TP=3 # take profit
cdef int SL=4 # stop loss
cdef int OT=5 # order time
### close collumns
cdef int CP=6 # close price
cdef int SS=7 # sucess status
cdef int CT=8 # close time
cdef int MB=9 # money back

# ctypedef enum my_c_enum_type:
#     val1,    #Default value is 0
#     val2,    #Default value is 1
#     val3     #Default value is 2

@cython.boundscheck(False)
@cython.wraparound(False)
@cython.nonecheck(False)
cpdef Nstocks(double enterprice, double exgain,
            double minp=300, double costorder=15., double ir=0.2):
    """Needed number of stocks based on:

    * Minimal acceptable profit $MinP$ (in R$)

    * Cost per order $CostOrder$   (in R$)

    * Taxes: $IR$ imposto de renda  (in 0-1 fraction)

    * Enter Price $EnterPrice$

    * Expected gain on the operation $ExGain$ (reasonable)  (in 0-1 fraction)

    $$ N_{stocks} \ge \frac{MinP+2 \ CostOrder}{(1-IR)\ EnterPrice \ ExGain} $$

    Be reminded that Number of Stocks MUST BE in 100s.

    ##### Only for buying (long) orders for selling (short) orders `EnterPrice` is the expected final price with gain???

    This guarantees a `MinP` per order"""
    # round stocks to 100's
    ceil = int(int((minp+costorder*2)/((1-ir)*enterprice*exgain))/100)
    # numpy ceil avoid using it for perfomance
    return ceil*100

@cython.boundscheck(False)
@cython.wraparound(False)
@cython.nonecheck(False)
cpdef MoneyBack(double enter_price, double close_price,
            double quantity, double direction, double ir=0.2, double cost_order=12):
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


@cython.boundscheck(False)
@cython.wraparound(False)
@cython.nonecheck(False)
cpdef ExecuteOrder(int nk, double[:,:] book, double price, int quantity, int direction,
            double dgain, double dstop, int time, double cost_order=12):
    """
    place order on book:book at position k
    return money spent
    """
    cdef int k = nk # just to remove warning
    book[k, EP] = price # enter price
    book[k, QT] = quantity # number of contracts
    book[k, DR] = direction # 1 for buy, -1 for sell
    # dgain/dstop are in percent decimal
    book[k, TP] = price*(1+direction*dgain) # take profit
    book[k, SL] =  price*(1-direction*dstop) # stop loss
    # time order was placed, (should be datetime?)
    # no it can be just the index of the price on the rates array the "i"
    book[k, OT] = time

    return quantity*price + cost_order

@cython.boundscheck(False)
@cython.nonecheck(False)
@cython.wraparound(False)
cpdef CloseOrder(int io, double[:, :] obook, int irow, double[:] cbook,
                 int time, double close_price, int bytime=0):
    """
    close order at row irow in the open book of orders with io valid entries
    copy this order to the cbook array passed
    finally rearrange the open book of orders excluding this row

    bytime : order was closed due time stop
    """

    # copy to closed book + fill 3 collumns more
    cbook[:] = obook[irow, :]
    cbook[CP] = close_price # close price
    cbook[CT] = time # close time
    # money back (will be incremented)
    moneyback = MoneyBack(cbook[EP], cbook[CP],
                                cbook[QT], cbook[DR])
    cbook[MB] =  moneyback
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


@cython.boundscheck(False)
@cython.nonecheck(False)
@cython.wraparound(False)
cpdef tryCloseOrder(int io, double[:,:] obook, int irow, double[:] cbook,
                 int time, double high, double low):
    """
    Try to close row specifying one order from the open book.
    If closed it is placed on the closed book row passed.

    Consider worst case cenario where we sell on low
    and buy on high. tentative to compensate for M1, OHLC in backtesting
    return 1 if closed or -1 contrary
    high, low is from the current minute M1
    """

    cdef double close_price = 0 # different from zero only when
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

@cython.boundscheck(False)
@cython.wraparound(False)
@cython.nonecheck(False)
cpdef tryCloseOrders(int io, double[:,:] obook, int ic, double[:,:] cbook,
             int time, double high, double low):
    """
    evalue if order should be closed (on obook)
    io, ic are indexes of the last entry on each book
    """
    cdef double money=0 # money increment due to all orders
    cdef int sucess=0
    cdef int status=0

    cdef int i;
    # closed in this loop
    for i in range(io):
        status = tryCloseOrder(io, obook, i, cbook[ic, :], time, high, low)
        if status > 0: # the order was closed
            # money back (will be incremented)
            money += cbook[ic, MB] # money back
            ic += 1 # one more on closed book
            io -= 1 # one less on open book

    return money, io, ic

@cython.boundscheck(False)
@cython.wraparound(False)
@cython.nonecheck(False)
cpdef tryClosebyTime(int io, double[:,:] obook, int ic, double[:,:] cbook,
             int time, int end_day, double high, double low, int expire_time=ExpTime):
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
    cdef double money=0 # money increment due to all orders closed

    cdef int i;
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
        elif time - int(obook[i, OT]) >  expire_time:
            # worst case scenario
            close_price = low if obook[i, DR] == 1 else high
            # open book, closed book, time, close_price
            CloseOrder(io, obook, i, cbook[ic, :], time, close_price, 1)
            # money back (will be incremented)
            money += cbook[ic, MB] # money back
            ic += 1 # one more on closed book
            io -= 1 # one less on open book

    return money, io, ic

@cython.boundscheck(False)
@cython.wraparound(False)
@cython.nonecheck(False)
cpdef isEqual(double[:] x, double value):
    cdef int i
    for i in range(x.size):
        if x[i] != value:
            return -1
    return 1

@cython.boundscheck(False)
@cython.wraparound(False)
@cython.nonecheck(False)
cpdef nextGuess(int[:] guess_time, int iguess, int time):
    """"find the first time_index bigger or
    equal than `time`"""
    cdef int t
    n = guess_time.size
    # find the next time index after index time
    for t in range(iguess, n):
        if guess_time[t] >= time:
            return t
    # couldn\'t find any. time to stop simulation
    return -1

@cython.boundscheck(False)
@cython.wraparound(False)
@cython.nonecheck(False)
cpdef actualMoney(int io, double[:,:] open_book, double money,
                  double H, double L, double cost_order=12):
    """"calculate actual money on open orders + pouch / pocket"""
    cdef int i
#     cdef double money = money
    for i in range(io): # for all open orders calculate money now
        if open_book[i, DR] == 1: # buy (allways worst case scenario)
            money += MoneyBack(open_book[i,EP], L,
                                open_book[i,QT], open_book[i,DR])
        else:
            money += MoneyBack(open_book[i,EP], H,
                                open_book[i,QT], open_book[i,DR])
    return money

@cython.boundscheck(False)
@cython.nonecheck(False)
@cython.wraparound(False)
cpdef Simulate(double[:,:] rates, int[:,:] guess_book,
               double[:,:] book_orders_open, double[:,:] book_orders_closed,
               double money=60000, int restrict=1):
    """
    guess_book contains : {time index, direction, index endday, index startday}
    array of orders to be placed at {time index},

    the first and last {time index} must be
    syncronized and included in the array  of rates
    """
    cdef int n = rates.shape[0]
    cdef double H, L
    cdef int iday_end
    # system variables
    # keeep track of money evolution
    cdef double[:]  moneyprogress = numpy.zeros(n)
    cdef int iro = 0 # real book of orders open index "i"
    cdef int irc = 0 # real book of orders close index "i"
    # control variables
    cdef int pirc = 0 # previous real book of orders close index "i"
    # commanding orders index, none yet executed
    cdef int iguess = 0 # indexes of guess orders executed done
    cdef int i = 0
    for i in range(n):#len(rates)):

        # get the next guess based on this "i" time_index (might be the same)
        iguess = nextGuess(guess_book[:,1], iguess, i)

        # get the current price (allways worst case scenario), and the end of the day
        H, L, iday_end, iday_start = rates[i, :]

        if i < iday_start+2*60: # no orders in the first 30 minutes
            # track money evolution
            moneyprogress[i] = actualMoney(iro, book_orders_open, money, H, L)
            continue

        if iro > 0: # Try to Close any previosly open order oney back if any on closing orders
            pirc = irc # copy to the previous size of the closed book
            moneyback, iro, irc = tryCloseOrders(iro, book_orders_open, irc,
                                book_orders_closed, i, H, L) # close by hit stop/gain
            money += moneyback # money back to the pouch
            moneyback, iro, irc = tryClosebyTime(iro, book_orders_open, irc,
                                book_orders_closed, i, iday_end, H, L) # close by expiring time or day
            money += moneyback # money back to the pouch

            if irc > pirc and restrict > 0: # one or more orders were closed
                # five sucessive orders failed
                if irc > 4 and isEqual(book_orders_closed[irc-5:irc, SS], -1) == 1:
                    print('i  ', i, 'skipped! too many losses!')
                    iguess = nextGuess(guess_book[:,1], iguess, iday_end+1)
                # four sucessive orders failed
                if irc > 3 and isEqual(book_orders_closed[irc-4:irc, SS], -1) == 1:
                    # make a waiting pause due to sucessive wrong orders
                    # wait 60 minutes
                    iguess = nextGuess(guess_book[:,1], iguess, i+60)
                # two sucessive orders failed
                if irc > 1 and isEqual(book_orders_closed[irc-2:irc, SS], -1) == 1:
                    # make a waiting pause due to sucessive wrong orders
                    # wait 30 minutes
                    iguess = nextGuess(guess_book[:,1], iguess, i+30)

            # try adjust stop making it go closer to the
            # target
            # tryAdjustStop(iro, book_orders_open, H, L)

        # it is not time to place orders anymore (end of session is near)
        if i == (iday_end-90):
            # go to the next guess after this day
            iguess = nextGuess(guess_book[:,1], iguess, iday_end+1)
        # is there any order to be done at this time_index
        elif i == guess_book[iguess, 1]:
            # order spree? no more than one order open at once
            if iro > 2 and restrict > 0:
                continue
            #if iro > 0 and i-int(book_orders_open[iro-1, OT]) < 30 and restrict > 0:
            #    continue

            # get the direction (buy or sell)
            buy = guess_book[iguess, 0]
            # enter price or H or L (allways worst case scenario)
            enter_price = H if buy==1 else L
            # Calculate number of stocks based on minimual profit expected etc.
            # enter price, expected gain variance
            quantity = Nstocks(enter_price, exgain=expected_var*3, minp=150)
            #print('time: ', i, quantity*enter_price, money)
            # is there money to buy?
            if quantity*enter_price < money:
                moneyspent = ExecuteOrder(iro, book_orders_open,
                            enter_price, quantity, buy,
                            expected_var*3, expected_var, i)
                iro +=1 # new entry on book
                money -= moneyspent

        # Close orders that suffer hit stop in the same minute
        if iro > 0:
            moneyback, iro, irc = tryCloseOrders(iro, book_orders_open, irc,
                                book_orders_closed, i, H, L) # close by hit stop/gain
            money += moneyback # money back to the pouch

        # track money evolution
        moneyprogress[i] = actualMoney(iro, book_orders_open, money, H, L)


    return moneyprogress, irc, iro
