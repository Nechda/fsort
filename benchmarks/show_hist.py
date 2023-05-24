import matplotlib.pyplot as plt
import re

# Read input file
lines = []
with open('input.txt', 'r') as f: lines = [li.strip() for li in f]
lines = [re.sub(r'#.*', '', li) for li in lines] # remove comments
lines = [re.sub(' +', ' ', li) for li in lines] # remove doubled spaces
lines = [re.sub(r'(\d)\s+(\d)', r'\1\2', li) for li in lines] # normalize numbers

# Parse readed info
value = []
total = len(lines)
dropped = 0
for i in range(total):
    v = int(lines[i].split()[1])
    value.append(v)

mean = list(sorted(value))[len(value)//2]
avg = int(sum(value) / len(value))

# Gen hist
fig, axs = plt.subplots(1, 1, sharey=True, tight_layout=True)
axs.set_title(f'iTLB load misses, {total} runs ( {dropped} dropped )\n mean = {mean}, avg = {avg}')
axs.hist(value, 50)
plt.xlabel('iTLB-load-misses, Millions')
plt.show()
