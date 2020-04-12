from itertools import repeat
from math import sqrt
from snakefish import Thread
from sys import argv
import os
import math


def extract(_globals_dict):
    return {}


def merge(_old_globals, _new_globals):
    pass


def eval_A(i, j):
    ij = i + j
    return 1.0 / (ij * (ij + 1) / 2 + i + 1)


def A_sum(u, i):
    x = 0
    for j, u_j in enumerate(u):
        x += eval_A(i, j) * u_j
    return x


def At_sum(u, i):
    x = 0
    for j, u_j in enumerate(u):
        x += eval_A(j, i) * u_j
    return x


def thread_func(f, u, r):
    return [f(u, i) for i in r]


def multiply_AtAv(u):
    r = range(len(u))

    threads = []
    jobs_per_thread = math.ceil(len(u) / os.cpu_count())
    for i in range(0, len(u), jobs_per_thread):
        r_slice = r[i:(i+jobs_per_thread)]
        t = Thread(lambda: thread_func(A_sum, u, r_slice), extract, merge)
        t.start()
        threads.append(t)

    tmp = []
    while len(threads) != 0:
        for i in range(len(threads)):
            if threads[i].try_join():
                assert (threads[i].get_exit_status() == 0)
                tmp.extend(threads[i].get_result())
                threads[i].dispose()
                threads.pop(i)
                break

    for i in range(0, len(u), jobs_per_thread):
        r_slice = r[i:(i+jobs_per_thread)]
        t = Thread(lambda: thread_func(At_sum, tmp, r_slice), extract, merge)
        t.start()
        threads.append(t)

    results = []
    while len(threads) != 0:
        for i in range(len(threads)):
            if threads[i].try_join():
                assert (threads[i].get_exit_status() == 0)
                results.extend(threads[i].get_result())
                threads[i].dispose()
                threads.pop(i)
                break

    return results


def main():
    n = int(argv[1])
    u = [1] * n

    for _ in range(10):
        v = multiply_AtAv(u)
        u = multiply_AtAv(v)

    vBv = vv = 0

    for ue, ve in zip(u, v):
        vBv += ue * ve
        vv  += ve * ve

    result = sqrt(vBv/vv)
    print("{0:.9f}".format(result))


if __name__ == '__main__':
    main()
