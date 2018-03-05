import numpy as np
import mxnet as mx
from mxnet import nd, autograd
import sys
import h5py

pX = np.random.rand(1, 2, 2, 1)
pY = mx.nd.ones((1,2,2,1))
print(pX)
print("_-----------------------------_")
print(pY)
