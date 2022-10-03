# Build utility
```bash
git clone https://github.com/Nechda/fsort.git fsort_src
cd fsort_src && export FSORT_SRC="$(pwd)" && cd -
mkdir fsort_build && cd fsort_build
cmake -GNinja ../fsort_src
```
# How to run utility
```bash
./fsort --binary=<path to binary file> --output=<path to output file> --runs=<number of profile recording runs> --delta=<period delta> --command=<command for benchmark running>

#Example
./fsort --binary='~/patched-gcc/gcc-install-final/libexec/gcc/x86_64-pc-linux-gnu/9.4.0/cc1plus' --output=sorted.out --runs=512 --delta=1 --command='~/patched-gcc/gcc-install-final/bin/g++ -w ~/benchmarks/tramp3d-v4.cpp -o /dev/null'
```

# How to apply reorder
Assume that we have already built pached gcc and plugin for reordering.
```bash
cd ~/patched-gcc/gcc-install/bin && export PATH="$(pwd):$PATH" && cd ../../build
bash -x $FSORT_SRC/build_cc1plus.sh
```
