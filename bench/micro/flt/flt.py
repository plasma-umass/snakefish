from math import sin, cos, sqrt
from timeit import default_timer as timer
import porpoise


class Point(object):
    __slots__ = ('x', 'y', 'z')

    def __init__(self, i):
        self.x = x = sin(i)
        self.y = cos(i) * 3
        self.z = (x * x) / 2

    def __repr__(self):
        return "<Point: x=%s, y=%s, z=%s>" % (self.x, self.y, self.z)

    def normalize(self):
        x = self.x
        y = self.y
        z = self.z
        norm = sqrt(x * x + y * y + z * z)
        self.x /= norm
        self.y /= norm
        self.z /= norm

    def maximize(self, other):
        self.x = self.x if self.x > other.x else other.x
        self.y = self.y if self.y > other.y else other.y
        self.z = self.z if self.z > other.z else other.z
        return self


def maximize(points):
    next = points[0]
    for p in points[1:]:
        next = next.maximize(p)
    return next

def porp_maximize(points, thr_count, size):
        portion = int(size/thr_count)
        thr_lst = []
        arr_s = 0
        arr_e = portion
        for n in range(thr_count-1):
            thr_lst.append(porpoise.Thread(maximize, points[arr_s:arr_e]))
            arr_s = arr_e
            arr_e += portion

        res_lst = [maximize(points[arr_s:size])]

        for thr in reversed(thr_lst):
            res_lst.append(thr.join())

        return maximize(res_lst)

def main(thr_count, loops, file, series):
    NMIN = 12
    NMAX = 24
    for y in range(NMIN, NMAX+1):
        size = 2**y
        num_time = timer()
        num_time -= num_time
        porp_time = num_time
        for i in range(0,loops):

            points = [None] * size
            for i in range(size):
                points[i] = Point(i)
            for p in points:
                p.normalize()


            start = timer()
            porp_maximize(points, thr_count, size)

            porp_time += timer() - start

            start = timer()
            maximize(points)

            num_time += timer() - start

        res = [series, ", flt, ", thr_count, ", ", size, ", ", loops, ", ", round(porp_time, 4), ", ", round(num_time,4), "\n"]
        file.write("".join(str(s) for s in res))
        file.flush()
