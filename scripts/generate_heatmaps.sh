export TOP=$(pwd)
export CC1PLUS=$TOP/gcc-install-lto/libexec/gcc/x86_64-pc-linux-gnu/12.2.0/cc1plus

# Собираем парсер перфа
cd $TOP/f-src/perf-visualizer
g++ perf_parser.cpp -O2 -o pp

# Генерим хитмапу
perf record -e cycles:u,L1-icache-load-misses:u,iTLB-load-misses:u $TOP/gcc-install-lto/bin/g++ -w $TOP/f-src/benchmarks/tramp3d-v4.cpp -o /dev/null && ./pp && ./plot_gen.py

cp raw_plot.png $TOP && eog $TOP/raw_plot.png

