## Example Password Cracker
"""
pass_cracker: An example of porpoise's return value handling.
Finds a bunch of passwords given via an input file in parallel. 
Run with `python3 pass_cracker.py [directory of porpoise src files] [input file]`.
"""

import hashlib
import string
from sys import argv


script, porpoise_path, input_file = argv
from sys import path
path.append(porpoise_path)

import porpoise

def findPass(l):
    d = dict()
    for a0 in l:
        for a1 in string.ascii_lowercase:
            for a2 in string.ascii_lowercase:
                for a3 in string.ascii_lowercase:
                    for a4 in string.ascii_lowercase:
                        h = hashlib.md5((a0+a1+a2+a3+a4).encode('utf-8')).hexdigest()
                        if h in hashes:
                            d[hashes[h]] = a0+a1+a2+a3+a4
    return d


hashes = {}
with open(input_file, "r") as f:
    for line in f:
        separated = line.split(' ')
        hashes[separated[1].strip()] = separated[0]



t0 = porpoise.Thread(findPass, ('a','b','c','d','e','f'))
t1 = porpoise.Thread(findPass, ('g','h','i','j','k','l','m'))
t2 = porpoise.Thread(findPass, ('n','o','p','q','r','s','t'))
t3 = porpoise.Thread(findPass, ('y','v','w','x','y','z'))

# One by One Join
d0 = t3.join()
d1 = t2.join()
d2 = t1.join()
d3 = t0.join()
dFinal = {**d0, **d1, **d2, **d3}
print(dFinal)
print("--------------------")
t0 = t0.recreate()
t1 = t1.recreate()
t2 = t2.recreate()
t3 = t3.recreate()

# Group Join
# pool = [t1, t3]
# d0, pool = porpoise.joinAny(pool)
# pool.append(t0)
# d1, pool = porpoise.joinAny(pool)
# pool.append(t2)
# d2, pool = porpoise.joinAny(pool)
# d3, pool = porpoise.joinAny(pool)

# Alternate Group Join for Efficiency Measurements
pool = [t0, t1, t2, t3]
d0, pool = porpoise.Thread.join_any(pool)
d1, pool = porpoise.Thread.join_any(pool)
d2, pool = porpoise.Thread.join_any(pool)
d3, pool = porpoise.Thread.join_any(pool)

dFinal = {**d0, **d1, **d2, **d3}
print(dFinal)
print("--------------------")
t0 = t0.recreate()
t1 = t1.recreate()
t2 = t2.recreate()
t3 = t3.recreate()

# Universal Join
results = porpoise.Thread.join_all()
dFinal = {}
for d in results:
    dFinal.update(d)

print (dFinal)
