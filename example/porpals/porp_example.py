### Example Global Policy
"""
porpals: An example of porpoise's global handling.
Does some simple processing on a number of builtin types and merges different versions at join time.
Run with `python3 porp_example.py [directory of porpoise src files]`
"""

import hashlib
import string
from sys import argv


script, porpoise_path = argv
from sys import path
path.append(porpoise_path)

import porpals
import porpoise

### Example class
class Example():
    # Set up a bunch of built in methods to show that it works
    def __init__(self, copy=None):
        if copy is None:
            self.int = 0
            self.tuple = (0,1)
            self.frozenset = frozenset([0,1,2])
            self.list = [0,1,2,3]
            self.set = set([0,1,2,3,4,5])
            self.dict = {0:1, 1:2, 2:3, 3:4, 4:5, 5:6}
        else:
            self.int = copy.int
            self.tuple = copy.tuple
            self.frozenset = copy.frozenset
            self.list = copy.list
            self.set = copy.set
            self.dict = copy.dict

    # Return a method to observe that it is wrapped
    def return_method(self):
        return self.porpal_merge(None, None)

    def porpal_merge(self, old_self, joined):
        self.int += joined.int - old_self.int
        self.set.update(joined.set)
        self.dict.update(joined.dict)

def print_state(obj, state):
    print("In state",  state, "the global example has: \nint : ", obj.int, "\nset: ", obj.set, "\ndict: ", obj.dict)
    pass


def count_append_update(x):
    example.int += x
    example.set.update(list(range(x)))
    example.dict[x] = x+1
    print_state(example, "count_append_update")


#example = porpals.porpal_log(Example(), pre, post, aset)
example = porpals.porpal(Example())

t0 = porpoise.Thread(count_append_update, [25])

count_append_update(15)

d0 = t0.join()

print_state(example, "end")
