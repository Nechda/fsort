import collections
import re

def parse(filename:str):
    # Read input file
    lines = []
    with open(filename, 'r') as f: lines = [li.strip() for li in f]
    lines = [re.sub(r'#.*', '', li) for li in lines] # remove comments
    lines = [re.sub(' +', ' ', li) for li in lines] # remove doubled spaces
    lines = [re.sub(r'-C(\d)\s+(\d)', r'-C\1_', li) for li in lines] #remove second row
    lines = [re.sub(r'(\d)\s+(\d)', r'\1\2', li) for li in lines] # normalize numbers
    lines = [re.sub(r'\(.*\)', '', li) for li in lines] # remove persentages
    lines = [li for li in lines if len(li.strip()) > 0] # remove empty strings

    # Skip lines if they dont contain infomation about events or time
    skip = lambda s: not s.startswith('S0-D0-C0') and not 'time elapsed' in s
    lines = [li for li in lines if not skip(li)]

    lines = [re.sub(r',', '.', li) for li  in lines] # Replace , -> .

    events = collections.defaultdict(list)
    elapsed_times = []
    is_time_event = lambda s: 'time elapsed' in s

    for li in lines:
        if is_time_event(li): elapsed_times.append(float(li.split()[0])); continue

        _, value, event_name = li.strip().split()
        value = int(value)
        events[event_name].append(value)

    return events, elapsed_times
