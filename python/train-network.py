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

'''
pX = np.random.rand(10000, 60, 19, 19)
vX = np.random.rand(10000, 60, 19, 19)

pY = np.random.rand(10000,).astype(np.int64)
vY = np.random.rand(10000,)
'''

ctx = mx.cpu()
batch_size = 128

piter = mx.io.NDArrayIter(pX, pY, batch_size=batch_size, last_batch_handle='roll_over')
viter = mx.io.NDArrayIter(vX, vY, batch_size=batch_size, last_batch_handle='roll_over')

def _conv3x3(channels, stride, in_channels, kernel_size, groups, padding):
    return gluon.nn.Conv2D(channels, kernel_size=kernel_size, strides=stride, padding=padding,
                     use_bias=False, in_channels=in_channels, groups=groups)

class BasicBlockV2(gluon.HybridBlock):
    def __init__(self, channels, stride=1, in_channels=0, **kwargs):
        super(BasicBlockV2, self).__init__(**kwargs)
        self.convs = gluon.nn.HybridSequential()
        self.convs.add(gluon.nn.BatchNorm())
        self.convs.add(gluon.nn.LeakyReLU(alpha=0.3))
        self.convs.add(_conv3x3(channels, stride, in_channels, 3, 1, 1))
        self.convs.add(gluon.nn.BatchNorm())
        self.convs.add(gluon.nn.LeakyReLU(alpha=0.3))
        self.convs.add(_conv3x3(channels, 1, channels, 3, 1, 1))

    def hybrid_forward(self, F, x):
        residual = x
        x = self.convs(x)
        return x + residual

class CombinedNet(gluon.HybridBlock):
    def __init__(self, num_filters, num_blocks, **kwargs):
        super(CombinedNet, self).__init__(**kwargs)

        with self.name_scope():
            self.convs = gluon.nn.HybridSequential()
            self.convs.add(gluon.nn.Conv2D(num_filters, 3, padding=1))
            self.convs.add(gluon.nn.LeakyReLU(alpha=0.3))

            for _ in range(num_blocks):
                self.convs.add(BasicBlockV2(num_filters))

            self.convs.add(gluon.nn.Conv2D(num_filters, 3, padding=1))
            self.convs.add(gluon.nn.LeakyReLU(alpha=0.3))

            self.value = gluon.nn.HybridSequential()
            self.value.add(gluon.nn.Conv2D(2, 3, padding=1))
            self.value.add(gluon.nn.LeakyReLU(alpha=0.3))
            self.value.add(gluon.nn.Flatten())
            self.value.add(gluon.nn.Dense(num_filters))
            self.value.add(gluon.nn.LeakyReLU(alpha=0.3))
            self.value.add(gluon.nn.Dense(1))

            self.policy = gluon.nn.HybridSequential()
            self.policy.add(gluon.nn.Conv2D(2, 3, padding=1))
            self.policy.add(gluon.nn.LeakyReLU(alpha=0.3))
            self.policy.add(gluon.nn.Flatten())
            self.policy.add(gluon.nn.Dense((19 * 19 + 1) * 2))
            self.policy.add(gluon.nn.LeakyReLU(alpha=0.3))
            self.policy.add(gluon.nn.Dense(19 * 19 + 1))

    def hybrid_forward(self, F, x):
        x = self.convs(x)
        p = self.policy(x)
        v = self.value(x)

        return F.softmax(p), F.sigmoid(v), p, v

net = CombinedNet(64, 4)
net.collect_params().initialize(mx.init.MSRAPrelu(), ctx=ctx)
net.hybridize()

policy_loss = gluon.loss.SoftmaxCrossEntropyLoss()
policy_loss.hybridize()

value_loss = gluon.loss.SigmoidBCELoss()
value_loss.hybridize()

trainer = gluon.Trainer(net.collect_params(), 'NAG', {'learning_rate': .1, 'momentum':.9, 'wd': 1e-10})

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
