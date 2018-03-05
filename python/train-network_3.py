import numpy as np
import mxnet as mx
from mxnet import nd, autograd
import sys


nd_iter = mx.io.NDArrayIter(data={'data':mx.nd.ones((100,10))}, label={'softmax_label':mx.nd.ones((100,))}, batch_size=25)

data = mx.sym.Variable('data')
label = mx.sym.Variable('softmax_label')
fullc = mx.sym.FullyConnected(data=data, num_hidden=1)
loss = mx.sym.SoftmaxOutput(data=fullc, label=label)
mod = mx.mod.Module(loss, data_names=['data'], label_names=['softmax_label'])
mod.fit(nd_iter, num_epoch=20)

print("Done")
