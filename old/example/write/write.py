### Example File Policy
"""
write: An examples of porpoies's I/O handling.
Two different threads write seperately to a file.
Porpoise merges the conflicts according to policy.
Run with `python3 write.py [directory of porpoise src files]`
"""

import hashlib
import string
from sys import argv


script, porpoise_path = argv
from sys import path
path.append(porpoise_path)

import porpoise

def do(f, txt):
    f.write(txt)
    f.write(txt)
    f.flush()
    # 0 is SEEK_SET
    f.seek(2, 0)
    s = f.read(9)
    print(s)

    # f.flush()
    return 0

f = open("test.txt", "w+")
print("opened here")


t0 = porpoise.Thread(do, f, "hello1\n")

f.write("hello0dasfadsfad\n")
f.flush()
f.seek(2, 0)
s = f.read(9)
print(s)
d0 = t0.join()

f.seek(2, 0)
s = f.read(9)
print(s)

f.close()
