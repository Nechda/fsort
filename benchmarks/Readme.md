# Как построить график
## Сбор данных pref stat
```bash
timeout 600 sudo setarch -R taskset 0x02 perf stat -r 0 \
-o report.txt -per-core -a -e \
cycles:u,iTLB-load-misses:u,iTLB-loads:u,L1-icache-misses:u,LLC-loads:u,LLC-load-misses:u \
taskset 0x1 nice -n -19 \
<path-to-g++> -w tramp3d-v4.cpp -o /dev/null
```
## Обработка данных
После выполнения, файл report.txt будет содержать строки следующего вида:
```text
S0-D0-C0           2     30 426 868 109      cycles:u                                                      (66,67%)
S0-D0-C0           2          3 046 061      iTLB-load-misses          #   12,62% of all iTLB cache accesses  (66,68%)
S0-D0-C0           2         24 129 692      iTLB-loads                                                    (66,68%)
...
```
Нужно узнать, на каком именно ядре происходило выполение бенчмарка, в моем случае это было `S0-D0-C0`. Поэтому
 сгенерируем файл `input.txt` следующим образом:
```bash
grep 'S0-D0-C0' report.txt | grep -i iTLB-load-misses > input.txt
```
## Генерация гистограммы
```bash
python3 show_hist.py
```
