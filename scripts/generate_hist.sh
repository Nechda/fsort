export TOP=$(pwd)

sudo timeout 120 setarch -R taskset 0x02 perf stat -r 0 -o report.txt --per-core -a -e \
    cycles:u,iTLB-load-misses:u,iTLB-loads:u,L1-icache-misses:u,LLC-loads:u,LLC-load-misses:u \
    taskset 0x1 nice -n -19 \
    $TOP/gcc-install-lto/bin/g++ -w $TOP/f-src/benchmarks/tramp3d-v4.cpp -o /dev/null

grep 'S0-D0-C0' report.txt | grep -i iTLB-load-misses > $TOP/input.txt
python3 $TOP/f-src/benchmarks/show_hist.py

