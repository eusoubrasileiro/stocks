# Unit root Tests using Pytorch
# adf augmented dickey fuller test

import torch as th
from matplotlib import pyplot as plt
import util
import numpy as np

# Batched Cholesky decomp nograd_cholesky
def cholesky(A, dev):
    L = th.zeros_like(A)
    s = th.zeros(A.shape[0], dtype=th.float32, device=dev);

    for i in range(A.shape[-1]):
        for j in range(i+1):
            s.fill_(0);
            for k in range(j):
                s.add_(L.select(1, i).select(-1, k) * L.select(1, j).select(-1, k))
                #s = s + L[:,i,k] * L[:,j,k]
            if i==j:
                L.select(1, i).select(-1, j).copy_(th.sqrt(A.select(1, i).select(-1, i) - s))
            else:
                L.select(1, i).select(-1, j).copy_(1.0 / L.select(1, j).select(-1, j) * (A.select(1, i).select(-1, j) - s))
#         L[...,i,j] = th.sqrt(A[...,i,i] - s) if (i == j) else \
#                   (1.0 / L[...,j,j] * (A[...,i,j] - s))
    return L


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


from torch.utils.data import Dataset
import gc

class SADFtMatrices(Dataset):
    """
    Supremum Augmented dickey fuller test OLS matrices as a Dataset
    One sample is composed of all OLS's of the ADF's to calculate one sadf(t) point
    """

    def __init__(self, indata, maxw, minw, p=30, verbose=False):
        """
        *indata : numpy array
            will be converted to np.float32

        """
        th.set_grad_enabled(False)
        indata = indata.astype(np.float32)
        self.verbose = verbose
        n = indata.size
        nobs = n-p-1 # data used for regression
        # Mather X OLS matrix, all OLS adf matrices are inside this
        self.X = th.zeros((nobs, 3+p), dtype=th.float32)
        y = th.tensor(indata, dtype=th.float32)
        diffilter = th.tensor([-1., 1.], dtype=th.float32).view(1, 1, 2)
        dy = th.conv1d(y.view(1, 1, -1), diffilter).view(-1)
        y = y.view(-1)
        self.z = dy[p:].clone()
        # X matrix
        self.X[:, 0] = 1 # drift
        self.X[:, 1] = th.arange(p+1, n) # deterministic trend
        self.X[:, 2] = y[p:-1]# regression data (nobs)
        # fill in columns, max lagged serial correlations
        for i in range(1, p+1):
            self.X[:, 2+i] = dy[p-i:-i]
        if verbose:
            print(len(self.z), nobs, p, n, self.X.shape)

        self.adfs_count = maxw-minw # number of adfs for one sadf t
        nadf = maxw # data used is (maximum - first adf)
        self.nobsadf = nadf-p-1 # number of observations (maximum - first adf)
        self.nsadft = n-maxw # number of sadf t's to calculate the entire SADF
        self.p = p

        if verbose:
            print('ADF tests calculated for one sadf(t) ', self.adfs_count)
            print('number of sadf(t) points ', self.nsadft)

    def __len__(self):
        return self.nsadft

    def __getitem__(self, idx):
        """
        idx  :
            index of a specific sadf(t) point calculation
            same as start line for master main X OLS matrix/ z vector
            returns all statistic tests elements of all ADF's to calculate this sadf(t) point
            tuple(OLS X, OLS z)
        """
        th.set_grad_enabled(False)
        # master X for a sadft (biggest adf OLS X matrix)
        Xm = self.X.narrow(0, idx, self.nobsadf) # master X for this sadft - biggest adf OLS X matrix
        zm = self.z.narrow(0, idx, self.nobsadf) # master Z for this sadft (biggest adf OLS independent term)
        # Xbt[j, :, :, :] = Xm.repeat(adfs_count, 1).view(adfs_count, nobsadf, (3+p))
        # zbt[j, :, :] = zm.repeat(adfs_count, 1).view(adfs_count, nobsadf)
        Xbts = Xm.repeat(self.adfs_count, 1).view(self.adfs_count, self.nobsadf, (3+self.p))
        zbts = zm.repeat(self.adfs_count, 1).view(self.adfs_count, self.nobsadf)
        # sadf loop until minw, every matrix is smaller than the previous
        # zeroing i lines, observations are becomming less also
        for k in range(self.adfs_count): # each is smaller than previous
            # Xbt[j, k, :k, :] = 0
            # zbt[j, k, :k] = 0
            # nobt[j, k] = float(nobsadf-k-(p+3))
            Xbts.select(0, k).narrow(0, 0, k).fill_(0)
            zbts.select(0, k).narrow(0, 0, k).fill_(0)
        # dont need to clone because to(device) creates a new tensor on desired device
        # except if the device is the same
        # Xbts, nbts comes from repeat() so dont need clone
        return Xbts, zbts

def torch_sadf(indata, maxw, minw, p=30, dev=th.device('cpu'),
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
        print('ADF tests calculated for one sadf(t) ', adfs_count)
        print('number of sadf(t) points ', nsadft)
        print('sadft size (GB) ', sadft_GB)
        print('batch size ', batch_size)
        print('number of batchs ', nbatchs)
        print('last batch', lst_batch_size)

    # master X for a sadft (biggest adf OLS X matrix)
    # batch version
    Xmb = th.zeros(batch_size, nobsadf, 3+p, device=th.device('cpu'), dtype=th.float32)
    # master Z for a sadft (biggest adf OLS independent term)
    zmb = th.zeros(batch_size, nobsadf, device=th.device('cpu'), dtype=th.float32)
    # CUDA
    Xcu = th.zeros(batch_size*adfs_count, nobsadf, (3+p),  device=dev, dtype=th.float32)
    zcu = th.zeros(batch_size*adfs_count, nobsadf, 1, device=dev, dtype=th.float32)
    sadf = th.zeros(nsadft, device=dev, dtype=th.float32)
    eye = th.eye(p+3, device=dev)
    # number of observations for the t statistic test is allways the
    # the same based on the adf sequence inside a SADF(t) calculation
    nobs = (maxw-p-1-(p+3)-th.arange(maxw-minw))
    nobsc = nobs.repeat(batch_size).to(dev)
    #ejc = ej_.repeat(batch_size*adfs_count).view(batch_size*adfs_count, -1).unsqueeze(-1)

    t = 0 # start line for master main X OLS matrix/ z vector
    for i in util.progressbar(range(nbatchs)):
        # copy the batch_size first SADF(t) points ADF bigger matrices
        for j in range(batch_size):
            Xmb.select(0, j).copy_(X.narrow(0, t, nobsadf))
            zmb.select(0, j).copy_(z.narrow(0, t, nobsadf))
            t = t+1
        # repeat those matrix for the number of ADF's of each SADF(t) point
        Xcpu = Xmb.repeat([1, adfs_count, 1]).view(batch_size, adfs_count, nobsadf, p+3)
        zcpu = zmb.repeat([1, adfs_count]).view(batch_size, -1, nobsadf)
        # sadf loop until minw, every matrix is smaller than the previous
        # zeroing i lines, observations are becomming less also
        for k in range(adfs_count): # each is smaller than previous
            # Xbt[j, k, :k, :] = 0
            # zbt[j, k, :k] = 0
            # nobt[j, k] = float(nobsadf-k-(p+3))
            Xcpu.select(1, k).narrow(1, 0, k).fill_(0)
            zcpu.select(1, k).narrow(1, 0, k).fill_(0)

        # TO CUDA
        Xcu.copy_(Xcpu.view(batch_size*adfs_count, nobsadf, (3+p)))
        zcu.copy_(zcpu.view(batch_size*adfs_count, nobsadf, 1))

        # RuntimeError: cholesky_cuda: For batch 12142: U(33,33) is zero, singular U.
        XcuT = Xcu.transpose(1, -1)
        L = cholesky(XcuT.bmm(Xcu), dev)
        Gi =  th.cholesky_solve(eye, L) # ( X^T . X ) ^-1
        xtz = th.bmm(XcuT, zcu)
        Bhat = th.cholesky_solve(xtz, L)
        er = zcu - th.bmm(Xcu, Bhat)
        Bhat = Bhat.squeeze()
        s2 = th.matmul(er.transpose(1, -1), er).view(-1)/nobsc
        adfstats = Bhat.select(-1, 2).div(th.sqrt(s2*Gi[:,2,2]))
        #adfstats[th.isnan(adfstats)] = -3.4e+38 # in case something wrong like colinearity
        adfstats.index_fill_(0, th.nonzero(th.isnan(adfstats)).view(-1), -3.4e+38)

        sadf.narrow(0, t-batch_size, batch_size).copy_(adfstats.view(batch_size, adfs_count).max(-1)[0])

    if lst_batch_size > 0:
        # last fraction of a batch
        Xmb = Xmb.narrow(0, 0, lst_batch_size)
        zmb = zmb.narrow(0, 0, lst_batch_size)
        # copy the batch_size first SADF(t) points ADF bigger matrices
        for j in range(lst_batch_size):
            Xmb.select(0, j).copy_(X.narrow(0, t, nobsadf))
            zmb.select(0, j).copy_(z.narrow(0, t, nobsadf))
            t = t+1
        # repeat those matrix for the number of ADF's of each SADF(t) point
        Xcpu = Xmb.repeat([1, adfs_count, 1]).view(lst_batch_size, adfs_count, nobsadf, p+3)
        zcpu = zmb.repeat([1, adfs_count]).view(lst_batch_size, -1, nobsadf)
        # sadf loop until minw, every matrix is smaller than the previous
        # zeroing i lines, observations are becomming less also
        for k in range(adfs_count): # each is smaller than previous
            # Xbt[j, k, :k, :] = 0
            # zbt[j, k, :k] = 0
            # nobt[j, k] = float(nobsadf-k-(p+3))
            Xcpu.select(1, k).narrow(1, 0, k).fill_(0)
            zcpu.select(1, k).narrow(1, 0, k).fill_(0)

        # TO CUDA
        nobsc = nobs.repeat(lst_batch_size).to(dev)
        Xcu = Xcu.narrow(0, 0, lst_batch_size*adfs_count)
        zcu = zcu.narrow(0, 0, lst_batch_size*adfs_count)
        Xcu.copy_(Xcpu.view(lst_batch_size*adfs_count, nobsadf, (3+p)))
        zcu.copy_(zcpu.view(lst_batch_size*adfs_count, nobsadf, 1))

        # RuntimeError: cholesky_cuda: For batch 12142: U(33,33) is zero, singular U.
        XcuT = Xcu.transpose(1, -1)
        L = cholesky(XcuT.bmm(Xcu), dev)
        Gi =  th.cholesky_solve(eye, L) # ( X^T . X ) ^-1
        xtz = th.bmm(XcuT, zcu)
        Bhat = th.cholesky_solve(xtz, L)
        er = zcu - th.bmm(Xcu, Bhat)
        Bhat = Bhat.squeeze()
        s2 = th.matmul(er.transpose(1, -1), er).view(-1)/nobsc
        adfstats = Bhat.select(-1, 2).div(th.sqrt(s2*Gi[:,2,2]))
        #adfstats[th.isnan(adfstats)] = -3.4e+38 # in case something wrong like colinearity
        adfstats.index_fill_(0, th.nonzero(th.isnan(adfstats)).view(-1), -3.4e+38)

        sadf.narrow(0, t-lst_batch_size, lst_batch_size).copy_(adfstats.view(lst_batch_size, adfs_count).max(-1)[0])

    return sadf.to('cpu').data.numpy()
