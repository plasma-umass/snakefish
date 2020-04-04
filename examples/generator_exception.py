import random
from typing import *  # use type hints to make signatures clear

import snakefish

a = 1  # global variable 'a'


# the function that will be executed on a snakefish generator
def f() -> float:
    global a
    a = [[42]]

    yield random.random()
    raise Exception("triggered")


# the function that will extract shared global variables
def extract(globals_dict: Dict[str, Any]) -> Dict[str, Any]:
    return {'a': globals_dict['a']}  # extract a


# the function that will merge shared global variables
def merge(old_globals: Dict[str, Any], new_globals: Dict[str, Any]) -> None:
    old_globals.update(new_globals)  # simply overwrite the old globals


# 'a' before update
print("global 'a' was", a)
print()

# spawn a snakefish generator
g = snakefish.Generator(f, extract, merge)
g.start()

# get values and check for exception
while True:
    try:
        print("next:", g.next(True))
    except StopIteration:
        print("generator exhausted, stop now")
        break
    except Exception as e:
        print("exception: ", type(e))
print()

# join the generator and check exit status
g.join()
assert (g.get_exit_status() == 0)
print("generator exit status:", g.get_exit_status())
print("global 'a' is", a)  # 'a' after update
