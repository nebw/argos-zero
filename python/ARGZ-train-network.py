import capnp
import sys
import glob
import os
import numpy as np
sys.path.append('/Users/valentinwolf/Documents/Studium/Machine_Learning/SoftwareProjekt')
import CapnpGame_capnp

import mxnet as mx
from mxnet import nd, autograd, gluon
import sys

# path to the games_combined data
games_combined_path = '/Users/valentinwolf/Documents/Studium/Machine_Learning/SoftwareProjekt/games_combined/*'

#Selected games for test set
inputs_t = []
probs_t = []
winners_t = []

# Selected games for validation set
inputs_v = []
probs_v = []
winners_v = []

val_prob = 0.05

for file in glob.glob(games_combined_path):
    f = open(file, 'rb')
    g = CapnpGame_capnp.Game.from_bytes_packed(f.read())

    if np.random.sample(1)[0] > valprob:
        inputs, probs, winners = inputs_t, probs_t, winners_t
    else:
        inputs, probs, winners = inputs_v, probs_v, winners_v

    for i in range(len(g.stateprobs)):
        inputs.append(np.array(g.stateprobs[i].state).reshape((4 + 8 , 9, 9)))
        probs.append(np.array(g.stateprobs[i].probs))
        winners.append(g.result)

# convert lists to np.array
inputs_t = np.array(inputs_t)
probs_t = np.array(probs_t)
winners_t = np.array(winners_t)

inputs_v = np.array(inputs_v)
probs_v = np.array(probs_v)
winners_v = np.array(winners_v)

# permute training data
p = np.random.permutation(len(inputs_t))

inputs_t = inputs_t[p]
probs_t = probs_t[p]
winners_t = winners_t[p]

# concatenate probs & winners and then delete the old lists
labels_t = np.concatenate((probs_t, winners_t[:, None]), axis=1)
labels_v = np.concatenate((probs_v, winners_v[:, None]), axis=1)

del probs_t
del probs_v
del winners_t
del winners_v

ctx = mx.cpu()
batch_size = 128

diter = mx.io.NDArrayIter(inputs_t, labels_t, batch_size=batch_size, last_batch_handle='roll_over')
val_set_x = mx.nd.array(inputs_v).as_in_context(mx.cpu)
val_set_y = mx.nd.array(labels_v).as_in_context(mx.cpu)

# Net
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
            self.policy.add(gluon.nn.Dense((9 * 9 + 1) * 2))
            self.policy.add(gluon.nn.LeakyReLU(alpha=0.3))
            self.policy.add(gluon.nn.Dense(9 * 9 + 1))

    def hybrid_forward(self, F, x):
        x = self.convs(x)
        p = self.policy(x)
        v = self.value(x)

        return F.softmax(p), F.sigmoid(v), p, v

net = CombinedNet(64, 3)
net.collect_params().initialize(mx.init.MSRAPrelu(), ctx=ctx)
net.hybridize()

policy_loss = gluon.loss.SoftmaxCrossEntropyLoss(sparse_label=False)
policy_loss.hybridize()

value_loss = gluon.loss.SigmoidBCELoss()
value_loss.hybridize()

trainer = gluon.Trainer(net.collect_params(), 'NAG', {'learning_rate': .1, 'momentum': .9, 'wd': 1e-4})

def rmean(series, win=1000):
    return np.mean(series[-win:])

closses = []
vlosses = []
plosses = []
vaccs = []
paccs = []

val_closses = []
val_vlosses = []
val_plosses = []

def pred_val_set(x,y):
    # Predict Validation set and append losses
    py = y[:, :(9*9+1)]
    vy = y[:, (9*9+1)]

    _,_,pp,vp = net(x)
    vloss = value_loss(vp, vy)
    ploss = policy_loss(pp, py)
    closs = (0.1 * vloss.mean() + ploss.mean())

    val_closses.append(combined_loss.as_in_context(mx.cpu()).asnumpy()[0])
    val_vlosses.append(vloss.as_in_context(mx.cpu()).asnumpy()[0])
    val_plosses.append(ploss.as_in_context(mx.cpu()).asnumpy()[0])

# pred validation set once so we have a baseline
pred_val_set(val_set_x,val_set_y)

def wrap_iter(it):
    while True:
        try:
            yield it.next()
        except StopIteration:
            pred_val_set(val_set_x,val_set_y)
            it.reset()

def early_stopping(last = 3):
    """retruns true if the combined validation loss has been rising over the
    last 3 epochs"""
    if len(val_closses) >= 3:
        return val_closses[-3]<val_closses[-2] and val_closses[-2]<val_closses[-1]
    return False

sigmoid = gluon.nn.Activation('sigmoid')

# training loop
for i, batch in enumerate(wrap_iter(diter)):
    x = batch.data[0].as_in_context(ctx)
    y = batch.label[0].as_in_context(ctx)

    py = y[:, :(9*9+1)]
    vy = y[:, (9*9+1)]

    with autograd.record():
        _, _, pp, vp = net(x)
        vloss = value_loss(vp, vy)
        ploss = policy_loss(pp, py)

        combined_loss = (0.1 * vloss.mean() + ploss.mean())
        combined_loss.backward()

    closses.append(combined_loss.as_in_context(mx.cpu()).asnumpy()[0])
    vlosses.append(vloss.as_in_context(mx.cpu()).asnumpy()[0])
    plosses.append(ploss.as_in_context(mx.cpu()).asnumpy()[0])
    paccs.append((pp.argmax(axis=-1) == py.argmax(axis=-1)).mean().as_in_context(mx.cpu()).asnumpy()[0] * 100)
    vaccs.append(((sigmoid(vp) > .5).flatten()[:, 0] == vy).mean().as_in_context(mx.cpu()).asnumpy()[0] * 100)

    trainer.step(batch_size)

    sys.stdout.write('\r{}: C:{:.3f}, V:{:.3f} ({:.1f}%), P:{:.3f} ({:.1f}%), ValC {:.3f}'.format(
        i, rmean(closses), rmean(vlosses), rmean(vaccs), rmean(plosses), rmean(paccs),val_closses[-1]))

    if early_stopping() or i == 1000000:
        print("Training stopped")
        break

# export net
net.export('/home/ben/tmp/combined_small.json')
