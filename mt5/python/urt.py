# Unit root Tests using Pytorch
# adf augmented dickey fuller test

import torch as th
from matplotlib import pyplot as plt

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
    X = th.zeros((batch, n-p-1, 3+p), device=dev, dtype=th.float64)
    if verbose:
        print(batch, p, n)
    y = th.tensor(data, device=dev, dtype=th.float64)
    diffilter = th.tensor([-1., 1.], device=dev, dtype=th.float64).reshape(1, 1, 2)
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
    Gi = th.inverse(th.matmul(Xt, X)) # ( X^T . X ) ^-1
    Bhat = th.matmul(Gi, th.matmul(Xt, z))
    er = z - th.matmul(X, Bhat)
    s2 = th.matmul(er.transpose(dim0=1, dim1=-1), er)/(nobs-(p+3))
    s2 = s2.view(-1)
    Bhat = Bhat.squeeze(2)
    tstat = Bhat[:, 2]/th.sqrt(s2*Gi[:,2,2])
    return tstat.to('cpu').data.numpy()

#using float32
def torch_sadftb(indata, maxw, minw, p=30, dev=th.device('cpu'),verbose=False):
    """batched version size=max-minw (sadf max number points)
    sent at once to GPU"""
    th.set_grad_enabled(False)
    n = indata.size
    nobs = n-p-1 # data used for regression
    X = th.zeros((n-p-1, 3+p), device=dev, dtype=th.float32)
    if verbose:
        print(p, n)
    y = th.tensor(indata, device=dev, dtype=th.float32)
    diffilter = th.tensor([-1., 1.], device=dev, dtype=th.float32).view(1, 1, 2)
    y = y.view(1, 1, -1)
    dy = th.conv1d(y, diffilter).view(-1)
    y = y.view(-1)
    z = dy[p:].clone()
    y.shape, y.shape, X.shape, z.shape
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

    #sadf loop until minw, every matrix is smaller than the previous
    # zeroing i lines, observations are becomming less also
    batch_size = maxw-minw
    Xbt = X.repeat(batch_size, 1).view(batch_size, X.shape[0], X.shape[1])
    zbt = z.repeat(batch_size, 1).view(batch_size, X.shape[0])
    nobt = th.zeros(batch_size, dtype=th.float32)
    for i in range(batch_size): # each is smaller than previous
        Xbt[i, :i, :] = 0
        zbt[i, :i] = 0
        nobt[i] = float(n-i-p-1-(p+3))

    # compute batch_size adf's
    tstats = torch_bmadf(Xbt, zbt, nobt)

    return th.max(tstats).item()

def torch_bmadf(Xbt, zbt, nobt):
    """given the ordinary least squares problem with
    matrix, vector already assembled and number of observations
        Xbt : batch of X matrixes
        zbt : batch of z (independent vector)
        nobt : number of observations for each test in batch
    compute the given batch ammount of augmented dickey fuller tests
    return test statistics torch array
    """
    # compute batch_size adf's
    zbt = zbt.unsqueeze(dim=-1) # additonal dim for matrix*vector mult.
    Xt = Xbt.transpose(dim0=1, dim1=-1)
    Gi = th.inverse(th.bmm(Xt, Xbt)) # ( X^T . X ) ^-1
    Bhat = th.bmm(Gi, th.bmm(Xt, zbt))
    er = zbt - th.bmm(Xbt, Bhat)
    Bhat = Bhat.squeeze()
    s2 = (er*er).sum(1).squeeze()/nobt
    tstats = Bhat[:, 2]/th.sqrt(s2*Gi[:, 2,2])
    return tstats
