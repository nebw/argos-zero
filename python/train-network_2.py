import numpy as np
import mxnet as mx
from mxnet import nd, autograd, gluon
import sys
import h5py

valnet_data = h5py.File('/home/zehha/Studies/sem7/go/valnet-data-mounted/data-val-all-combined.h5', 'r')
polnet_data = h5py.File('/home/zehha/Studies/sem7/go/polnet-data-mounted/data-pol-gogod-tygem-combined.h5', 'r')

pX = polnet_data['X']
pY = polnet_data['Y']

vX = valnet_data['X']
vY = valnet_data['Y']

ctx = mx.cpu()
batch_size = 64
num_batches = 10

vXred = np.empty((num_batches*batch_size, 12, 19, 19))
vYred = np.empty((num_batches*batch_size,))

pXred = np.empty((num_batches*batch_size, 12, 19, 19))
pYred = np.empty((num_batches*batch_size,))

piter = mx.io.NDArrayIter(pX, pY, batch_size=batch_size, last_batch_handle='roll_over')
viter = mx.io.NDArrayIter(vX, vY, batch_size=batch_size, last_batch_handle='roll_over')

def wrap_iter(it):
    while True:
        try:
            yield it.next()
        except StopIteration:
            it.reset()

for i, (pbatch, vbatch) in enumerate(zip(wrap_iter(piter), wrap_iter(viter))):
    if (i == num_batches):
        break

    vXred[i*batch_size:(i+1)*batch_size , :, :, :] = mx.ndarray.concat(vbatch.data[0][:, :4, :, :], vbatch.data[0][:, 37:45, :, :], dim=1).as_in_context(ctx).asnumpy()
    vYred[i*batch_size:(i+1)*batch_size,] = vbatch.label[0].as_in_context(ctx).asnumpy()

    pXred[i*batch_size:(i+1)*batch_size, :, :, :] = mx.ndarray.concat(pbatch.data[0][:, :4, :, :], pbatch.data[0][:, 37:45, :, :], dim=1).as_in_context(ctx).asnumpy()
    pYred[i*batch_size:(i+1)*batch_size,] = pbatch.label[0].as_in_context(ctx).asnumpy()


data = {'data1': pXred, 'data2': vXred }
label = {'label1': pYred, 'label2': vYred }
trainIt = mx.io.NDArrayIter(data, label, batch_size=batch_size, last_batch_handle='roll_over')


class CombinedNet():
    def __init__(self, num_filters, num_blocks, **kwargs):
        super(CombinedNet, self).__init__(**kwargs)

        data1 = mx.sym.Variable('data1')
        data2 = mx.sym.Variable('data2')
        label1 = mx.sym.Variable('label1')
        label2 = mx.sym.Variable('label2')

        net = mx.sym.Concat(data1, data2)
        net = mx.sym.Convolution(data=net,kernel=(3,3),num_filter=num_filters,pad=(1,1))
        net = mx.sym.LeakyReLU(data=net,slope=0.3)

        #for _ in range(num_blocks):
        #    self.convs.add(BasicBlockV2(num_filters))

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
        output2 = mx.sym.LogisticRegressionOutput(data=value, label=label2, name='output2')
        self.value = output2

        policy = mx.sym.Convolution(data=net,kernel=(3,3), num_filter=2, pad=(1,1))
        policy = mx.sym.LeakyReLU(data=policy,slope=0.3)
        policy = mx.sym.Flatten(data=policy)
        policy = mx.sym.FullyConnected(data=policy,num_hidden=(19*19+1)*2)
        policy = mx.sym.LeakyReLU(data=policy,slope=0.3)
        policy = mx.sym.FullyConnected(data=policy,num_hidden=19*19+1)
        output1 = mx.sym.SoftmaxOutput(data=policy, label=label1, name='output1')
        self.policy = output1


net = CombinedNet(64, 4)
output= mx.symbol.Group([net.policy, net.value])

model = mx.mod.Module(
    context            = ctx,
    symbol             = output,
    label_names        = ['label1', 'label2'],
    data_names         = ['data1', 'data2']
    )

print("start training")


model.fit(
    train_data         = trainIt,
    eval_metric        = mx.metric.Accuracy(),
    num_epoch          = 1000,
    optimizer_params   = (('learning_rate', 0.01), ('momentum', 0.9), ('wd', 0.00001)),
    initializer        = mx.init.Xavier(factor_type="in", magnitude=2.34),
    batch_end_callback = mx.callback.Speedometer(batch_size, 1)
    )


'''
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
