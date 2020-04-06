import multiprocessing
import random
import time
from typing import *  # use type hints to make signatures clear

import wrappers

multiprocessing.set_start_method('fork')  # this is needed for merging to work
a = 1  # global variable 'a'


# the function that will be executed on a thread
def f() -> List[int]:
    global a
    a = [[42]]

    if random.random() < 0.9:
        raise Exception("triggered")
    else:
        return [42]


# the function that will extract shared global variables
def extract(globals_dict: Dict[str, Any]) -> Dict[str, Any]:
    return {'a': globals_dict['a']}  # extract a


# the function that will merge shared global variables
def merge(old_globals: Dict[str, Any], new_globals: Dict[str, Any]) -> None:
    old_globals.update(new_globals)  # simply overwrite the old globals


# 'a' before update
print("global 'a' was", a)
print()

# spawn a thread
t = wrappers.Thread(target=f, extract=extract, merge=merge, shared_vars=globals())
t.start()

# try to join the thread and time it
start = time.time()
while True:
    t.join(0)
    if t.get_exit_status() is not None:
        print("join() successful")
        break
    else:
        print("join() unsuccessful, sleep for 100ms")
        time.sleep(0.1)
print("join() took %s sec" % (time.time() - start))

# check exit status
assert (t.get_exit_status() == 0)
print("thread exit status:", t.get_exit_status())

# get the return value and check for exception
try:
    print("result of f():", t.get_result())
except Exception as e:
    print("exception from f(): ", type(e))
print("global 'a' is", a)  # 'a' after update
