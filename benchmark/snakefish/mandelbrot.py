from contextlib import closing
from itertools import islice
from os import cpu_count
from sys import argv, stdout
from snakefish import Generator
import math

def extract(_globals_dict):
    return {}

def merge(_old_globals, _new_globals):
    pass

def pixels(y, n, abs):
    range7 = bytearray(range(7))
    pixel_bits = bytearray(128 >> pos for pos in range(8))
    c1 = 2. / float(n)
    c0 = -1.5 + 1j * y * c1 - 1j
    x = 0
    while True:
        pixel = 0
        c = x * c1 + c0
        for pixel_bit in pixel_bits:
            z = c
            for _ in range7:
                for _ in range7:
                    z = z * z + c
                if abs(z) >= 2.: break
            else:
                pixel += pixel_bit
            c += c1
        yield pixel
        x += 8

def compute_row(p):
    y, n = p

    result = bytearray(islice(pixels(y, n, abs), (n + 7) // 8))
    result[-1] &= 0xff << (8 - n % 8)
    return y, result

def ordered_rows(rows, n):
    order = [None] * n
    i = 0
    j = n
    while i < len(order):
        if j > 0:
            row = next(rows)
            order[row[0]] = row
            j -= 1

        if order[i]:
            yield order[i]
            order[i] = None
            i += 1

def generator_compute_rows(jobs):
    for j in jobs:
        yield compute_row(j)

def compute_rows(n, f):
    row_jobs = list(((y, n) for y in range(n)))
    jobs_per_generator = math.ceil(n / cpu_count())

    generators = []
    for i in range(0, n, jobs_per_generator):
        jobs = list(row_jobs[i:(i+jobs_per_generator)])
        def f():
            yield from generator_compute_rows(jobs)
        g = Generator(f, extract, merge)
        g.start()
        generators.append(g)

    unordered_rows = []
    while len(generators) != 0:
        for i in range(len(generators)):
            try:
                unordered_rows.append(generators[i].next(False))
            except IndexError:
                pass
            except StopIteration:
                generators[i].join()
                assert (generators[i].get_exit_status() == 0)
                generators[i].dispose()
                generators.pop(i)
                break
    yield from ordered_rows(iter(unordered_rows), n)

def mandelbrot(n):
    with open("bench_output-mandelbrot_sf.bmp", mode="wb") as f:
        with closing(compute_rows(n, compute_row)) as rows:
            f.write("P4\n{0} {0}\n".format(n).encode())
            for row in rows:
                f.write(row[1])

if __name__ == '__main__':
    mandelbrot(int(argv[1]))
