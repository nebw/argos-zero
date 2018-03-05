import numpy as np
import mxnet as mx
from mxnet import nd, autograd
import sys
import h5py

'''
valnet_data = h5py.File('/home/zehha/Studies/sem7/go/valnet-data-mounted/data-val-all-combined.h5', 'r')
polnet_data = h5py.File('/home/zehha/Studies/sem7/go/polnet-data-mounted/data-pol-gogod-tygem-combined.h5', 'r')

pX = polnet_data['X']
pY = polnet_data['Y']

vX = valnet_data['X']
vY = valnet_data['Y']

pX = np.random.rand(10000, 60, 19, 19)
vX = np.random.rand(10000, 60, 19, 19)

pY = np.random.rand(10000,).astype(np.int64)
vY = np.random.rand(10000,)
'''


#viter = mx.io.NDArrayIter(vX, vY, batch_size=batch_size, last_batch_handle='roll_over')
#data = {'data1': pX, 'data2': vX }
#label = {'label1': pY, 'label2': vY }
#trainIt = mx.io.NDArrayIter(data, label, batch_size=batch_size, last_batch_handle='roll_over')

ctx = mx.cpu()
batch_size = 128
piter = mx.io.NDArrayIter(data={'data':mx.nd.ones((10000, 60, 19, 19))}, label={'softmax_label':mx.nd.ones((10000,))}, batch_size=25)

data = mx.sym.Variable('data')
label = mx.sym.Variable('softmax_label')

fullc = mx.sym.FullyConnected(data=data, num_hidden=1)
output = mx.sym.SoftmaxOutput(data=fullc, label=label)

mod = mx.mod.Module(output, data_names=['data'], label_names=['softmax_label'])

print("start training")

mod.fit(piter, num_epoch=2)


'''
num_filters = 64
num_blocks = 4


data2 = mx.sym.Variable('data2')
net = mx.sym.Concat(data1, data2)
net = mx.sym.Convolution(data=data,kernel=(3,3),num_filter=num_filters, pad=(1,1))
net = mx.sym.LeakyReLU(data=net, slope=0.3)

net = mx.sym.BatchNorm(data=net)
net = mx.sym.LeakyReLU(data=net,slope=0.3)
net = mx.sym.Convolution(data=net,kernel=(3,3), num_filter=num_filters)
net = mx.sym.BatchNorm(data=net)
net = mx.sym.LeakyReLU(data=net,slope=0.3)
net = mx.sym.Convolution(data=net,kernel=(3,3), num_filter=num_filters, stride=(1,1))

net = mx.sym.Convolution(data=net,kernel=(3,3), num_filter=num_filters, stride=(1,1), pad=(1,1))
net = mx.sym.LeakyReLU(data=net,slope=0.3)

value = mx.sym.Convolution(data=net, kernel=(3,3), num_filter=2,pad=(1,1))
value = mx.sym.LeakyReLU(data=value, slope=0.3)
value = mx.sym.Flatten(data=value)
value = mx.sym.FullyConnected(data=value, num_hidden=num_filters)
value = mx.sym.LeakyReLU(data=value, slope=0.3)
value = mx.sym.FullyConnected(data=value, num_hidden=1)
output2 = mx.sym.LogisticRegressionOutput(data=value, name='output2')

policy = mx.sym.Convolution(data=net,kernel=(3,3), num_filter=2, pad=(1,1))
policy = mx.sym.LeakyReLU(data=policy,slope=0.3)
policy = mx.sym.Flatten(data=policy)
policy = mx.sym.FullyConnected(data=policy,num_hidden=(19*19+1)*2)
policy = mx.sym.LeakyReLU(data=policy,slope=0.3)
policy = mx.sym.FullyConnected(data=policy,num_hidden=19*19+1)

model = mx.mod.Module(
    context            = ctx,
    symbol             = output,
    label_names        = ['output1_label', 'output2_label'],
    data_names         = ['data1', 'data2']
    )

model.bind(data_shapes=trainIt.provide_data, label_shapes=trainIt.provide_label)

model.fit(trainIt, num_epoch = 1000)

    train_data         = trainIt,
    eval_metric        = mx.metric.Accuracy(),
    num_epoch          = 1000,
    optimizer_params   = (('learning_rate', 0.01), ('momentum', 0.9), ('wd', 0.00001)),
    initializer        = mx.init.Xavier(factor_type="in", magnitude=2.34)
    )
    #batch_end_callback = mx.callback.Speedometer(batch_size, 50))



def rmean(series, win=1000):
    return np.mean(series[-win:])

closses = []
vlosses = []
plosses = []
vaccs = []
paccs = []

sigmoid = gluon.nn.Activation('sigmoid')

def wrap_iter(it):
    while True:
        try:
            yield it.next()
        except StopIteration:
            it.reset()

for i, (pbatch, vbatch) in enumerate(zip(wrap_iter(piter), wrap_iter(viter))):
    # use AlphaGo Zero feature planes
    vx = mx.ndarray.concat(vbatch.data[0][:, :4, :, :], vbatch.data[0][:, 37:45, :, :], dim=1).as_in_context(ctx)
    vy = vbatch.label[0].as_in_context(ctx)

    px = mx.ndarray.concat(pbatch.data[0][:, :4, :, :], pbatch.data[0][:, 37:45, :, :], dim=1).as_in_context(ctx)
    py = pbatch.label[0].as_in_context(ctx)

    with autograd.record():
        _, _, _, vp = net(vx)
        vloss = value_loss(vp, vy)

        _, _, pp, _ = net(px)
        ploss = policy_loss(pp, py)

        combined_loss = (vloss.mean() + ploss.mean())
        combined_loss.backward()

    closses.append(combined_loss.as_in_context(mx.cpu()).asnumpy()[0])
    vlosses.append(vloss.as_in_context(mx.cpu()).asnumpy()[0])
    plosses.append(ploss.as_in_context(mx.cpu()).asnumpy()[0])
    paccs.append((pp.argmax(axis=-1) == py).mean().as_in_context(mx.cpu()).asnumpy()[0] * 100)
    vaccs.append(((sigmoid(vp) > .5).flatten()[:, 0] == vy).mean().as_in_context(mx.cpu()).asnumpy()[0] * 100)

    trainer.step(batch_size)

    sys.stdout.write('\r{}: C:{:.3f}, V:{:.3f} ({:.1f}%), P:{:.3f} ({:.1f}%)'.format(
        i, rmean(closses), rmean(vlosses), rmean(vaccs), rmean(plosses), rmean(paccs)))

    if i == 1000000:
        break

net.export('/home/ben/tmp/combined_small.json')
'''
