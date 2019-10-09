## Example Generator
"""
generator: An example of porpoise's generator handling.
Runs a generator in parallel.
Run with `python3 generator.py [directory of porpoise src files]`.
"""
from sys import argv


script, porpoise_path = argv
from sys import path
path.append(porpoise_path)

import porpoise

def regen(x):
    count = 0
    while count < x:
        yield count
        count += 1


t0 = porpoise.Thread(regen, 16)

for x in range(18):
    y = t0.join()
    print(y)

t0.terminate()
