import sys
import math
from wrappers import Thread
from os import cpu_count


def make_tree(d):

    if d > 0:
        d -= 1
        return (make_tree(d), make_tree(d))
    return (None, None)


def check_tree(node):

    (l, r) = node
    if l is None:
        return 1
    else:
        return 1 + check_tree(l) + check_tree(r)


def make_check(itde, make=make_tree, check=check_tree):

    i, d = itde
    return check(make(d))


def get_argchunks(i, d, chunksize=5000):

    assert chunksize % 2 == 0
    chunk = []
    for k in range(1, i + 1):
        chunk.extend([(k, d)])
        if len(chunk) == chunksize:
            yield chunk
            chunk = []
    if len(chunk) > 0:
        yield chunk


def thread_func(jobs):
    return [make_check(j) for j in jobs]


def main(n, min_depth=4):

    max_depth = max(min_depth + 2, n)
    stretch_depth = max_depth + 1

    print('stretch tree of depth {0}\t check: {1}'.format(
          stretch_depth, make_check((0, stretch_depth))))

    long_lived_tree = make_tree(max_depth)

    mmd = max_depth + min_depth
    for d in range(min_depth, stretch_depth, 2):
        i = 2 ** (mmd - d)
        cs = 0

        for argchunk in get_argchunks(i,d):
            jobs_per_thread = math.ceil(len(argchunk) / cpu_count())
            threads = []
            for k in range(0, len(argchunk), jobs_per_thread):
                jobs = argchunk[k:(k+jobs_per_thread)]
                t = Thread(target=thread_func, args=(jobs,))
                t.start()
                threads.append(t)

            results = []
            while len(threads) != 0:
                for k in range(len(threads)):
                    if threads[k].join(0):
                        assert (threads[k].get_exit_status() == 0)
                        results.extend(threads[k].get_result())
                        threads.pop(k)
                        break

            cs += sum(results)

        print('{0}\t trees of depth {1}\t check: {2}'.format(i, d, cs))

    print('long lived tree of depth {0}\t check: {1}'.format(
          max_depth, check_tree(long_lived_tree)))


if __name__ == '__main__':
    main(int(sys.argv[1]))
