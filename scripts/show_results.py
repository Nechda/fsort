#! /bin/python3

import collections
import matplotlib.pyplot as plt
import re
import math
import PSP
import matplotlib
import matplotlib.pyplot as plt
import numpy as np


def geomean(arr, to_int = True):
    n = len(arr)
    su = sum(map(math.log, arr))
    if to_int:
        return int(math.exp(su/n))
    return math.exp(su/n)

results = [
        {'name' : 'gcc-12.2.0',
         'van' : 'perf-results/gcc-npt.txt', 'opt' : 'perf-results/gcc-opt.txt'},
        {'name' : 'clang-16',
         'van' : 'perf-results/clang-npt.txt', 'opt' : 'perf-results/clang-opt.txt'},
        {'name' : 'cmake-3.24.4',
         'van' : 'perf-results/cmake-npt.txt', 'opt' : 'perf-results/cmake-opt.txt'},
        {'name' : 'cpython-3.11',
         'van' : 'perf-results/python-npt.txt', 'opt' : 'perf-results/python-opt.txt'},
]

def create_plot(data_getter, out_filename, title_text, units):
    labels = []
    not_optimized = []
    optimized = []
    for it in results:
        labels.append(it['name'])
        not_optimized.append(data_getter(it['van']))
        optimized.append(data_getter(it['opt']))

    N = len(results)
    x = np.arange(len(labels))  # the label locations
    width = 0.35  # the width of the bars

    fig, ax = plt.subplots()
    ax.set_ylim([0, 150])
    rects1 = ax.bar(x - width/2, [100.0] * N, width, label='Vanilla')
    rects2 = ax.bar(x + width/2, [100.0 * optimized[i]/not_optimized[i] for i in range(N)], width, label='Optimized')

    ax.set_ylabel(f'% of {title_text} over vanilla')
    ax.set_title(title_text + ', [{}]'.format(units))
    ax.set_xticks(x)
    ax.set_xticklabels(labels)
    ax.legend()


    def autolabel(rects, labels):
        """Attach a text label above each bar in *rects*, displaying its height."""
        for i, rect in enumerate(rects):
            height = rect.get_height()
            fmt = '{:.2f}' if type(labels[i]) == float else '{}'
            ax.annotate(fmt.format(labels[i]),
                        xy=(rect.get_x() + rect.get_width() * 1/2, height),
                        xytext=(0, 3),  # 3 points vertical offset
                        textcoords="offset points", fontsize=6,
                        ha='center', va='bottom')


    autolabel(rects1, not_optimized)
    autolabel(rects2, optimized)

    fig.tight_layout()
    plt.savefig(out_filename, dpi=450)

def field_getter(filename, field_name):
    events, time_elapsed = PSP.parse(filename)
    if 'time' in field_name: return geomean(time_elapsed, False)
    return geomean(events[field_name])

itlb_load_getter = lambda f : field_getter(f, 'iTLB-loads') 
itlb_misses_getter = lambda f : field_getter(f, 'iTLB-load-misses') 
time_getter = lambda f : field_getter(f, 'time')

create_plot(itlb_load_getter, 'plots/itlb-loads.png', 'iTLB-loads', 'amount')
create_plot(itlb_misses_getter, 'plots/itlb-load-misses.png', 'iTLB-load-misses', 'amount')
create_plot(time_getter, 'plots/time.png', 'Elapsed time', 'S')
