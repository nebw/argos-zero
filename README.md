# Argos Zero

This is an attempt to replicate [Mastering the game of Go without human knowledge](https://www.nature.com/articles/nature24270). The board representation is based on the 'library of efficient go routines'(https://github.com/lukaszlew/libego) by Łukasz Lew.

## Build instructions

Requirements:
* MXNet
* CMake
* Git

Optional requirements:
* CUDA / cuDNN
* Intel MKL
* NNPACK

How to build:
* (Install optional depedencies)
* Install MXNet ([official instructions](http://mxnet.incubator.apache.org/install/index.html))
  * `git clone --recursive --branch 1.1.0 https://github.com/apache/incubator-mxnet`
  * Configure dependencies in make/config.mk file, `USE_CPP_PACKAGE` should be set to `1`
  * `make -j NUM_CPUS` (replace NUM_CPUS by the number of CPUs of you computer)
* Install MXNet python package
  * Either: `cd python; pip install --user -e .`
  * Or: Install precompiled [pypi package](https://pypi.python.org/pypi/mxnet/1.1.0)
* Download a [pretrained model](https://drive.google.com/open?id=1gAxR_jJXmT2DhSOllJYW8Yng3jV07lTI):
* Compile Argos Zero:
  * Clone the repository: `git clone https://github.com/nebw/argos-zero.git`
  * Edit the config file `argos-zero/src/argos/Config.h`
  * Create build folder: `mkdir argos-zero-build; cd argos-zero-build`
  * Run CMake: `cmake -DCMAKE_BUILD_TYPE=Release -DMXNET_PATH=/your/local/mxnet/path /your/local/argos/zero/path`
  * `make -j NUM_CPUS` (replace NUM_CPUS by the number of CPUs of you computer)

Finally, you can try the selfplay mode by running: `argos-zero-build/src/selfplay`. You can play against the program using any Go GUI that supports the GTP protocol using the `argos-zero-build/src/gtp` binary. A modern Go GUI is [Sabaki](http://sabaki.yichuanshen.de/).
