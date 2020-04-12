from itertools import repeat, starmap
from math import sqrt
from sys import argv


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


def multiply_AtAv(u):
    r = range(len(u))

    tmp = list(starmap(
        A_sum,
        zip(repeat(u), r)
    ))
    return list(starmap(
        At_sum,
        zip(repeat(tmp), r)
    ))


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
