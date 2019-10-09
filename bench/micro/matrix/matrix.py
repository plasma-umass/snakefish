import numpy.matlib as mat
import numpy as np
from timeit import default_timer as timer
import porpoise

def power(x, y):
    z = x**y
    z = z/y
    z = z**y
    z = z+1
    return z


def main(thr_count, loops, file, series):
    POWER = 1024
    NMIN = 6
    NMAX = 12
    for y in range(NMIN, NMAX+1):
        size = 2**y
        num_time = timer()
        num_time -= num_time
        porp_time = num_time
        for i in range(0,loops):
            matrices = []
            for x in range(thr_count):
                matrices.append(np.random.rand(size,size))

            power(matrices[0], POWER)

            start = timer()

            thr_lst = []
            for m in range(thr_count - 1):
                thr_lst.append(porpoise.Thread(power, matrices[m], POWER))

            porp_res = power(matrices[thr_count-1], POWER)

            for thr in thr_lst:
                porp_res += thr.join()

            porp_time += timer() - start

            power(matrices[0], POWER)

            nump_res = np.zeros((size, size))

            start = timer()

            for m in matrices:
                nump_res += power(m, POWER)

            num_time += timer() - start

        res = [series, ", matrix, ", thr_count, ", ", size, ", ", loops, ", ", round(porp_time, 4), ", ", round(num_time,4), "\n"]
        file.write("".join(str(s) for s in res))
        file.flush()
