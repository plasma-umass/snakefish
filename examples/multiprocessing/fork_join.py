from typing import *  # use type hints to make signatures clear

import wrappers

a = 1  # global variable 'a'


# the function that will be executed on a thread
def f() -> List[int]:
    print("f() executing")
    global a
    a = [i for i in range(10)]  # update global variable 'a'
    return [i for i in range(200)]  # return a large list


# the function that will extract shared global variables
def extract(globals_dict: Dict[str, Any]) -> Dict[str, Any]:
    return {'a': globals_dict['a']}  # extract a


# the function that will merge shared global variables
def merge(old_globals: Dict[str, Any], new_globals: Dict[str, Any]) -> None:
    old_globals.update(new_globals)  # simply overwrite the old globals


# 'a' before update
print("global 'a' was", a)
print()

# spawn a thread and join it
t = wrappers.Thread(target=f, extract=extract, merge=merge, shared_vars=globals())
print("thread alive?", t.is_alive())
t.start()
print("thread alive?", t.is_alive())
t.join()
print("thread alive?", t.is_alive())
print()

# check exit status and get the return value
assert (t.get_exit_status() == 0)
print("thread exit status:", t.get_exit_status())
print("result of f():", t.get_result())
print("global 'a' is", a)  # 'a' after update
