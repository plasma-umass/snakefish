import numpy as np
import numexpr as ne
from timeit import default_timer as timer
from sys import path

import porpoise

scaler = -1J
def job(arr, r):
    for k in range(r):
        res = ne.evaluate('exp(scaler * arr)')

    return res

def njob(arr, r):
    for k in range(r):
        res = np.exp(scaler * arr)

    return res

def main(thr_count, loops, file, series):
    MIN = 6
    MAX = 12
    times = 2048

    for y in range(MIN, MAX+1):

        size = 2**y
        portion = int(size/thr_count)
        num_time = timer()
        num_time -= num_time
        porp_time = num_time
        for x in range(loops):

            arr = np.random.rand(size, size)

            # Get things running
            job(arr, times)

            start = timer()
            thr_lst = []
            arr_s = 0
            arr_e = portion
            for n in range(thr_count-1):
                thr_lst.append(porpoise.Thread(njob, arr[arr_s:arr_e], times))
                arr_s = arr_e
                arr_e += portion

            mcexp = njob(arr[arr_s:size], times)


            for thr in reversed(thr_lst):
                mcexp = np.vstack([thr.join(), mcexp])

            porp_time += (timer() - start)

            # Get things running
            cexp = job(arr, times)

            start = timer()
            cexp = job(arr, times)
            num_time += (timer() - start)

        res = [series, ", vml, ", thr_count, ", ", size, ", ", loops, ", ", round(porp_time, 4), ", ", round(num_time,4), "\n"]
        file.write("".join(str(s) for s in res))
        file.flush()
