#!/usr/bin/env zsh

set -e

: ${SRC=${HOME}/tc}
export PATH=$SRC/llvm-prov/scripts:$SRC/llvm/build/Release/bin:$PATH
export LOOM_PREFIX=$SRC/loom/build/Release
export LOOM_LIB=$LOOM_PREFIX/lib/LLVMLoom.so
export LLVM_PROV_LIB=$SRC/llvm-prov/build/Release/lib/LLVMProv.so

cd $SRC/llvm
[[ -d build/Release ]] || mkdir -p build/Release
cd build/Release
cmake -G Ninja -D CMAKE_BUILD_TYPE=Release ../..
ninja
cd -

cd $SRC/loom
[[ -d build/Release ]] || mkdir -p build/Release
cd build/Release
cmake -G Ninja -D CMAKE_BUILD_TYPE=Release ../..
ninja
cd -


cd $SRC/llvm-prov
[[ -d build/Release ]] || mkdir -p build/Release
cd build/Release
cmake -G Ninja -D CMAKE_BUILD_TYPE=Release -D LOOM_PREFIX=${LOOM_PREFIX} ../..
ninja
cd -

cd $SRC/freebsd
#time nice env INSTRUMENT_EVERYTHING=true llvm-prov-make -j32 buildworld buildkernel installworld installkernel KERNCONF=CADETS
time nice env llvm-prov-make -j32 buildworld buildkernel installworld installkernel KERNCONF=CADETS
cd -
