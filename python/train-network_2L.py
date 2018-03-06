import numpy as np
import mxnet as mx
from mxnet import nd, autograd, gluon
import sys
import h5py

#valnet_data = h5py.File('/home/swp/SWP/data-val-all-combined.sqfs', 'r')
#polnet_data = h5py.File('/home/swp/SWP/data-pol-gogod-tygem-combined.h5', 'r')

#pX = polnet_data['X']
#pY = polnet_data['Y']
#vX = valnet_data['X']
#vY = valnet_data['Y']

pX = np.random.rand(100, 60, 19, 19)
vX = np.random.rand(100, 60, 19, 19)

pY = np.random.rand(100,).astype(np.int64)
vY = np.random.rand(100,)

ctx = mx.cpu()
batch_size =  20 #128

#piter = mx.io.NDArrayIter(pX, pY, batch_size=batch_size, last_batch_handle='roll_over')
#viter = mx.io.NDArrayIter(vX, vY, batch_size=batch_size, last_batch_handle='roll_over')
print "daten geladen"

data = {'data1': pX, 'data2': vX }
label = {'label1': pY, 'label2': vY }
trainIt = mx.io.NDArrayIter(data, label, batch_size=batch_size, last_batch_handle='roll_over')

print "iterator erstellt"

def init_BasicBlockV2(data, num_filters):
	net = mx.sym.BatchNorm(data=data)
	net = mx.sym.LeakyReLU(data=net,slope=0.3)
	net = mx.sym.Convolution(data=net,kernel=(3,3), num_filter=num_filters)
	net = mx.sym.BatchNorm(data=net)
	net = mx.sym.LeakyReLU(data=net,slope=0.3)
	net = mx.sym.Convolution(data=net,kernel=(3,3), num_filter=num_filters, stride=(1,1))
	return net

def init_CombinedNet(num_filters, num_blocks):
	data1 = mx.sym.Variable('data1')
	data2 = mx.sym.Variable('data2')
	label1 = mx.sym.Variable('label1')
	label2 = mx.sym.Variable('label2')
	net = mx.sym.Concat(data1, data2)

	net = mx.sym.Convolution(data=net,kernel=(3,3),num_filter=num_filters,pad=(1,1))
	net = mx.sym.LeakyReLU(data=net,slope=0.3)

	for i in range(num_blocks):
		net = init_BasicBlockV2(net, num_filters)

	net = mx.sym.Convolution(data=net,kernel=(3,3), num_filter=num_filters, stride=(1,1), pad=(1,1))
	net = mx.sym.LeakyReLU(data=net,slope=0.3)

	value = mx.sym.Convolution(data=net, kernel=(3,3), num_filter=2,pad=(1,1))
	value = mx.sym.LeakyReLU(data=value, slope=0.3)
	value = mx.sym.Flatten(data=value)
	value = mx.sym.FullyConnected(data=value, num_hidden=num_filters)
	value = mx.sym.LeakyReLU(data=value, slope=0.3)
	value = mx.sym.FullyConnected(data=value, num_hidden=1)
	value = mx.sym.LogisticRegressionOutput(data=value, label=label2, name='output2')

	policy = mx.sym.Convolution(data=net,kernel=(3,3), num_filter=2, pad=(1,1))
	policy = mx.sym.LeakyReLU(data=policy,slope=0.3)
	policy = mx.sym.Flatten(data=policy)
	policy = mx.sym.FullyConnected(data=policy,num_hidden=(19*19+1)*2)
	policy = mx.sym.LeakyReLU(data=policy,slope=0.3)
	policy = mx.sym.FullyConnected(data=policy,num_hidden=19*19+1)
	policy = mx.sym.SoftmaxOutput(data=policy, label=label1, name='output1')

	CombinedOutput=mx.symbol.Group([policy,value])
	return CombinedOutput

print "init net"
net = init_CombinedNet(64, 4)

print "init model"
model = mx.mod.Module(
    context            = ctx,
    symbol             = net,
    label_names        = ['label1', 'label2'],
    data_names         = ['data1', 'data2']
    )

print "fit model"
epochs = 1	#1000000
model.fit(
    train_data         = trainIt,
    num_epoch          = epochs, 
    optimizer          = 'NAG',
    optimizer_params   = (('learning_rate', 0.1), ('momentum', 0.9), ('wd', 1e-10)),
    initializer        = mx.init.MSRAPrelu(),
    batch_end_callback = mx.callback.Speedometer(batch_size, 1))

#speichern
print "save model"
model.save_checkpoint('combined_net',epochs)

