# run from inside x64 developer tools of ms visual studio 2019
# cmake must be installed
# cuda for the same version of libtorch/pytorch must be installed
# cudnn must be installed according to tutorial (copy dll, lib, include files to CUDA installation)
cmake -DCMAKE_PREFIX_PATH=D:\Projects\libtorch -build=Release -A x64 ..