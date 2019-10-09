# import numpy as np
import time
import random

from sys import argv
script, porpoise_path = argv
from sys import path
path.append(porpoise_path)

import porpoise



count = [128, 256, 512]
loops = 3

#C = [list(range(n*y+1,n+(n*y+1))) for y in range(n)]

# print(A)
def matmul(rows, matrix):
    return np.matmul(rows, matrix)

def matmul_slow(result, rows, matrix, row_count):
    for i in range(row_count):
        for j in range(len(rows[i])):
            for k in range(len(matrix)):
                result[i][j] += rows[i][k] + matrix[k][j]

    return result[0:row_count]

for n in count:
    # A = [list(np.random.rand(n)) for y in range(n)]
    # B = [list(np.random.rand(n)) for y in range(n)]
    # C = [list(np.zeros(shape=(n,1))) for y in range(n)]
    A = [[random.randint(0, 1000) for y in range(n)] for x in range(n)]
    B = [[random.randint(0, 1000) for y in range(n)] for x in range(n)]
    C = [[random.randint(0, 1000) for y in range(n)] for x in range(n)]
    python_time = 0
    porp_time = 0
    for loop in range(loops):
        # A = np.random.rand(n,n)
        # B = np.random.rand(n,n)
        # C = np.zeros(shape=(n,n))

        python_start = time.time()
        if True:
            for i in range(n):
                for j in range(n):
                    for k in range(n):
                        C[i][j] += A[i][k] * B[k][j]
        python_time += time.time() - python_start

        # numpy_start = time.time()
        # C = np.matmul(A,B)
        # numpy_end = time.time()
        #
        # A = np.random.rand(n,n)
        # B = np.random.rand(n,n)
        # C = np.zeros(shape=(n,n))
        thr_count = 16

        porp_start = time.time()
        portion = int(n/thr_count)
        thr_lst = []
        for i in range(thr_count-1):
            thr_lst.append(porpoise.Thread(matmul_slow, C, A[portion*i:portion*(i+1)], B, portion))

        C[portion*(thr_count-1):n] = matmul_slow(C, A[portion*(thr_count-1):n], B, n-portion*(thr_count-1))
        for thr in thr_lst:
            C[portion*i:portion*(i+1)] = thr.join()
        porp_time += time.time() - porp_start

    print("Python version: " + str(python_time / (loops)) + " seconds")
    print("SnakeFish version: " + str(porp_time / (loops)) + " seconds")



# print("Numpy version: " + str((numpy_end - numpy_start) / (10**9)) + " seconds")
# print("SnakeFish + Numpy version: " + str((porp_num_end - porp_num_start) / (10**9)) + " seconds")
