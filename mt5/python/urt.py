# Unit root Tests using Pytorch
# adf augmented dickey fuller test

import torch as th
from matplotlib import pyplot as plt
import util

def torch_adf(data, p=30, verbose=False):
    """batched version usefull for data of equal size
    could be usefull for SADF if batchs are created for equal sub-data size
    for example same backward lags when computing multiple sadf t's"""
    dev = th.device('cpu')
    if th.cuda.is_available():
        dev = th.device('cuda')
    if len(data.shape) > 1:
        batch, n = data.shape
    else:
        n = data.size
        batch = 1
        data = data.reshape(1, -1)
    nobs = n-p-1 # data used for regression
    X = th.zeros((batch, n-p-1, 3+p), device=dev, dtype=th.float32)
    if verbose:
        print(batch, p, n)
    y = th.tensor(data, device=dev, dtype=th.float32)
    diffilter = th.tensor([-1., 1.], device=dev, dtype=th.float32).reshape(1, 1, 2)
    y = y.reshape(batch, 1, -1)
    dy = th.conv1d(y, diffilter).reshape(batch, -1)
    y = y.reshape(batch, -1)
    z = dy[:, p:].clone()
    if verbose:
        print(len(z), nobs, p, n, X.shape)
    # X matrix
    X[:, :, 0] = 1 # drift
    X[:, :, 1] = th.arange(p+1, n) # deterministic trend
    X[:, :, 2] = y[:, p:-1]# regression data (nobs)
    # fill in columns, max lagged serial correlations
    for i in range(1, p+1):
        X[:, :, 2+i] = dy[:, p-i:-i]
    if verbose:
        plt.figure(figsize=(5,5))
        plt.imshow(X[0].to('cpu').data.numpy(), aspect='auto', vmin=-0.05, vmax=0.05)
    Xt = X.transpose(dim0=1, dim1=-1)
    z = z.unsqueeze(2)
    L = th.cholesky(th.bmm(Xt, X))
    Gi =  th.cholesky_solve(th.eye(p+3), L) # ( X^T . X ) ^-1
    Xtz = th.bmm(Xt, z)
    Bhat = th.bmm(Gi, Xtz)
    er = z - th.bmm(X, Bhat)
    s2 = (th.matmul(er.transpose(dim0=1, dim1=-1), er)/(nobs-(p+3))).view(-1)
    Bhat = Bhat.squeeze(2)
    tstat = Bhat[:, 2]/th.sqrt(s2*Gi[:,2,2])
    #print(nobs-(p+3))
    return tstat.item(), Gi.sum().item()

#using float32
def torch_sadft(indata, maxw, minw, p=30, dev=th.device('cpu'),verbose=False):
    """sadf for one t, the given indata from maxw until minw
    batched version all adfs are sent at once to GPU"""
    th.set_grad_enabled(False)
    n = indata.size
    nobs = n-p-1 # data used for regression
    X = th.zeros((n-p-1, 3+p), device=dev, dtype=th.float32)
    y = th.tensor(indata, device=dev, dtype=th.float32)

    diffilter = th.tensor([-1., 1.], device=dev, dtype=th.float32).view(1, 1, 2)
    dy = th.conv1d(y.view(1, 1, -1), diffilter).view(-1)
    z = dy[p:].clone()
    # X matrix
    X[:, 0] = 1 # drift
    X[:, 1] = th.arange(p+1, n) # deterministic trend
    X[:, 2] = y[p:-1]# regression data (nobs)
    # fill in columns, max lagged serial correlations
    for i in range(1, p+1):
        X[:, 2+i] = dy[p-i:-i]
    # sadf loop until minw, every matrix is smaller than the previous
    # zeroing i lines, observations are becomming less also
    batch_size = maxw-minw
    Xb = X.repeat(batch_size, 1).view(batch_size, X.shape[0], X.shape[1])
    zb = z.repeat(batch_size, 1).view(batch_size, X.shape[0], 1) # additonal dim for matrix*vector mult.
    nobsi = th.zeros(batch_size, dtype=th.float32)
    #zeromask = th.ones(batch_size, nobs, dtype=th.float32)
    for i in range(batch_size): # each is smaller than previous
        Xb[i, :i, :] = 0
        zb[i, :i] = 0
        nobsi[i] = n-(float(i)+2*p+4)
        #zeromask[i, :i] = 0;

    # """given the ADF test ordinary least squares problem with
    # matrix, vector already assembled and number of observations
    #     Xbt : batch of X matrixes
    #     zbt : batch of z (independent vector)
    #     nobi : number of observations for each test in batch
    # compute the given batch ammount of augmented dickey fuller tests
    # return test statistics torch array
    # """
    Xt = Xb.transpose(dim0=1, dim1=-1)
    L = th.cholesky(th.bmm(Xt, Xb))
    Gi = th.cholesky_solve(th.eye(p+3), L) # ( X^T . X ) ^-1
    # how to ignore i first lines  of bmm(Xt, zbt)
    Xtz= th.bmm(Xt, zb)
    Bhat = th.bmm(Gi, Xtz)
    zhat = th.bmm(Xb, Bhat) #*zeromask.unsqueeze(-1)  # estimated z
    # need to zeroing zhat entries
    # each lines must b
    er = (zb - zhat) #*zeromask.unsqueeze(-1) # zeromask # hadamart product
    Bhat = Bhat.squeeze()
    s2 = th.matmul(er.transpose(dim0=1, dim1=-1), er).view(-1)/nobsi
    tstats = Bhat[:, 2]/th.sqrt(s2*Gi[:, 2,2])

    return tstats.data.numpy(), Gi.sum(1).sum(1).data.numpy()


def torch_sadf(indata, maxw, minw, p=30, pcrlimit=0.1, dev=th.device('cpu'),
        gpumem_GB=3.0, verbose=False):
    """fastest version
    - assembly rows of OLS problem using entire input data
    - send batchs of 1GB adfs tests to GPU until entire
    sadf is calculated, last batch might (should be smaller)
    """

    if minw <= (2*p+4):
        print("need more data for perform a OLS and calculate s2")
        print("need minw > p+1+(p+3) => 2p+3+1 => 2p+4 : ", 2*p+4)
        return

    if minw > maxw:
        print("error minw > maxw ")
        return

    th.set_grad_enabled(False)
    n = indata.size
    nobs = n-p-1 # data used for regression
    X = th.zeros((nobs, 3+p), device=dev, dtype=th.float32)
    if verbose:
        print(p, n)
    y = th.tensor(indata, device=dev, dtype=th.float32)
    diffilter = th.tensor([-1., 1.], device=dev, dtype=th.float32).view(1, 1, 2)
    dy = th.conv1d(y.view(1, 1, -1), diffilter).view(-1)
    y = y.view(-1)
    z = dy[p:].clone()

    if verbose:
        print(len(z), nobs, p, n, X.shape)
    # X matrix
    X[:, 0] = 1 # drift
    X[:, 1] = th.arange(p+1, n) # deterministic trend
    X[:, 2] = y[p:-1]# regression data (nobs)
    # fill in columns, max lagged serial correlations
    for i in range(1, p+1):
        X[:, 2+i] = dy[p-i:-i]
    if verbose:
        print(len(z), nobs, p, n, X.shape)
    # 1 sadf point requires at least GB (only matrix storage)
    adfs_count = maxw-minw # number of adfs for one sadf t
    nadf = maxw # data used is (maximum - first adf)
    nobsadf = nadf-p-1 # number of observations (maximum - first adf)
    xsize = (nobsadf*(3+p))*4 # 4 bytes float32
    sadft_GB = xsize*adfs_count/(1024**3) # matrix storage for 1 sadf point
    nsadft = n-maxw # number of sadf t's to calculate the entire SADF

    batch_size = 1 # number of sadft points to calculate at once
    if sadft_GB < gpumem_GB: # each batch will have at least 1GB in OLS matrixes
        batch_size = int(gpumem_GB/sadft_GB)
        batch_size = nsadft if batch_size > nsadft else batch_size

    nbatchs = nsadft//batch_size # number of batchs to sent to GPU
    lst_batch_size = nsadft - nbatchs*batch_size # last batch of adfs (integer %)

    if verbose:
        print('adfs_count ', adfs_count)
        print('sadft_GB ', sadft_GB)
        print('batch_size ', batch_size)
        print('nsadft ', nsadft)
        print('nbatchs ', nbatchs)
        print('lst_batch_size ', lst_batch_size)

    # master X for a sadft (biggest adf OLS X matrix)
    Xm = th.zeros((nobsadf, 3+p), device=th.device('cpu'), dtype=th.float32)
    # master Z for a sadft (biggest adf OLS independent term)
    zm = th.zeros(nobsadf, device=th.device('cpu'), dtype=th.float32)

    # batch matrix, vector and observations for multiple adfs
    Xbt = th.zeros(batch_size, adfs_count, nobsadf, (3+p), device=th.device('cpu'), dtype=th.float32)
    zbt = th.zeros(batch_size, adfs_count, nobsadf, device=th.device('cpu'), dtype=th.float32)
    nobt = th.zeros(batch_size, adfs_count, device=th.device('cpu'), dtype=th.float32)
    # CUDA
    Xbt_ = th.zeros(batch_size*adfs_count, nobsadf, (3+p),  device=dev, dtype=th.float32)
    zbt_ = th.zeros(batch_size*adfs_count, nobsadf, 1, device=dev, dtype=th.float32)
    nobt_ = th.zeros(batch_size*adfs_count,  device=dev, dtype=th.float32)


    sadf = th.zeros(nsadft, device=dev, dtype=th.float32)

    t = 0 # start line for master main X OLS matrix/ z vector
    for i in util.progressbar(range(nbatchs)):

        for j in range(batch_size): # assembly batch_size sadf'ts matrixes
        #   Xm[:] = X[t:t+nobsadf] # master X for this sadft - biggest adf OLS X matrix
        #   zm[:]  = z[t:t+nobsadf] # master Z for this sadft (biggest adf OLS independent term)
            Xm.copy_(X.narrow(0, t, nobsadf)) # master X for this sadft - biggest adf OLS X matrix
            zm.copy_(z.narrow(0, t, nobsadf)) # master Z for this sadft (biggest adf OLS independent term)

            Xbts = Xbt.select(0, j)
            zbts = zbt.select(0, j)
            nobts = nobt.select(0, j)
            # Xbt[j, :, :, :] = Xm.repeat(adfs_count, 1).view(adfs_count, nobsadf, (3+p))
            # zbt[j, :, :] = zm.repeat(adfs_count, 1).view(adfs_count, nobsadf)
            Xbts.copy_(Xm.repeat(adfs_count, 1).view(adfs_count, nobsadf, (3+p)))
            zbts.copy_(zm.repeat(adfs_count, 1).view(adfs_count, nobsadf))

            # sadf loop until minw, every matrix is smaller than the previous
            # zeroing i lines, observations are becomming less also
            for k in range(adfs_count): # each is smaller than previous
                # Xbt[j, k, :k, :] = 0
                # zbt[j, k, :k] = 0
                # nobt[j, k] = float(nobsadf-k-(p+3))
                Xbts.select(0, k).narrow(0, 0, k).fill_(0)
                zbts.select(0, k).narrow(0, 0, k).fill_(0)
                nobts.select(0, k).fill_(float(nobsadf-k-(p+3)))

            t = t + 1
        # TO CUDA
        Xbt_.copy_(Xbt.view(batch_size*adfs_count, nobsadf, (3+p)))
        zbt_.copy_(zbt.view(batch_size*adfs_count, nobsadf, 1))
        nobt_.copy_(nobt.view(batch_size*adfs_count))
        # principal component regression PCR
        U, S, V = th.svd(Xbt_)
        # zero eigenvalues smaller than threshold (pcrlimit)
        # S*S are the principal values
        Sk = th.where(S <= th.tensor(pcrlimit, device=dev), th.tensor(0.0, device=dev), th.tensor(1.0, device=dev))
        V = Sk.repeat(1, X.shape[1]).view(V.shape)*V # choose only the first, V_k
        W = th.bmm(Xbt_, V)
        Wt = W.transpose(1, -1)
        WtWi = th.diag_embed(1/(S*S)) 
        VWtWi = V.bmm(WtWi)
        Bhat = VWtWi.bmm(Wt.bmm(zbt_))
        Q = VWtWi.bmm(V.transpose(1,-1))
        er = zbt_ - Xbt_.bmm(Bhat)
        Bhat = Bhat.squeeze()
        s2 = (er*er).sum(1).squeeze().div(nobt_)
        #tstats = Bhat[:, 2]/th.sqrt(s2*Q[:, 2,2])
        # adfstats = Bhat[:, 2]/th.sqrt(s2*Gi[:, 2,2])
        adfstats = Bhat.select(-1, 2).div(th.sqrt(s2*Q.select(-2, 2).select(-1, 2)))

        # adfstats = torch_bmadf(Xbt.view(batch_size*adfs_count, nobsadf, (3+p)),
        #                      zbt.view(batch_size*adfs_count, nobsadf),
        #                      nobt.view(batch_size*adfs_count))

        sadf.narrow(0, t-batch_size, batch_size).copy_(adfstats.view(batch_size, adfs_count).max(-1)[0])

    # last fraction of a batch
    for j in range(lst_batch_size): # assembly batch_size sadf'ts matrixes
        Xm.copy_(X.narrow(0, t, nobsadf)) # master X for this sadft - biggest adf OLS X matrix
        zm.copy_(z.narrow(0, t, nobsadf)) # master Z for this sadft (biggest adf OLS independent term)

        Xbts = Xbt.select(0, j)
        zbts = zbt.select(0, j)
        nobts = nobt.select(0, j)
        Xbts.copy_(Xm.repeat(adfs_count, 1).view(adfs_count, nobsadf, (3+p)))
        zbts.copy_(zm.repeat(adfs_count, 1).view(adfs_count, nobsadf))

        # sadf loop until minw, every matrix is smaller than the previous
        # zeroing i lines, observations are becomming less also
        for k in range(adfs_count): # each is smaller than previous
            Xbts.select(0, k).narrow(0, 0, k).fill_(0)
            zbts.select(0, k).narrow(0, 0, k).fill_(0)
            nobts.select(0, k).fill_(float(nobsadf-k-(p+3)))

        t = t + 1

    if lst_batch_size > 0:
        # TO CUDA
        Xbt_ = Xbt_.narrow(0, 0, lst_batch_size*adfs_count)
        zbt_ = zbt_.narrow(0, 0, lst_batch_size*adfs_count)
        nobt_ = nobt_.narrow(0, 0, lst_batch_size*adfs_count)
        Xbt_.copy_(Xbt.narrow(0, 0, lst_batch_size).view(lst_batch_size*adfs_count, nobsadf, (3+p)))
        zbt_.copy_(zbt.narrow(0, 0, lst_batch_size).view(lst_batch_size*adfs_count, nobsadf, 1))
        nobt_.copy_(nobt.narrow(0, 0, lst_batch_size).view(lst_batch_size*adfs_count))

        # principal component regression PCR
        U, S, V = th.svd(Xbt_)
        # zero eigenvalues smaller than threshold (pcrlimit)
        # S*S are the principal values
        Sk = th.where(S <= th.tensor(pcrlimit, device=dev), th.tensor(0.0, device=dev), th.tensor(1.0, device=dev))
        V = Sk.repeat(1, X.shape[1]).view(V.shape)*V # choose only the first, V_k
        W = th.bmm(Xbt_, V)
        Wt = W.transpose(1, -1)
        WtWi = th.diag_embed(1/(S*S)) 
        VWtWi = V.bmm(WtWi)
        Bhat = VWtWi.bmm(Wt.bmm(zbt_))
        Q = VWtWi.bmm(V.transpose(1,-1))
        er = zbt_ - Xbt_.bmm(Bhat)
        Bhat = Bhat.squeeze()
        s2 = (er*er).sum(1).squeeze().div(nobt_)
        #tstats = Bhat[:, 2]/th.sqrt(s2*Q[:, 2,2])
        # adfstats = Bhat[:, 2]/th.sqrt(s2*Gi[:, 2,2])
        adfstats = Bhat.select(-1, 2).div(th.sqrt(s2*Q.select(-2, 2).select(-1, 2)))
        
        sadf.narrow(0, t-lst_batch_size, lst_batch_size).copy_(adfstats.view(lst_batch_size, adfs_count).max(-1)[0])

    return sadf.to('cpu').data.numpy()
