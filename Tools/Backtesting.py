import numpy as np

def Nstocks(enterprice, exgain, minp=300, costorder=15., ir=0.2):
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
    return np.ceil(int((minp+costorder*2)/((1-ir)*enterprice*exgain))/100)*100
