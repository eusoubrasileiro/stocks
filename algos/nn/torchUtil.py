import torch as th
import torch.nn.functional as F
from torch.optim import lr_scheduler
from matplotlib import pyplot as plt
from . import torchCV

def binaryTensors(X, Y, device='cpu'):
    X = th.tensor(X, device=device, dtype=th.float32)
    Y = th.tensor(Y, device=device, dtype=th.long)
    return X, Y
