"""
The algorithm finds distance between closest pair of points
in the given n points.
Approach used -> Divide and conquer
The points are sorted based on Xco-ords and
then based on Yco-ords separately.
And by applying divide and conquer approach,
minimum distance is obtained recursively.

>> Closest points can lie on different sides of partition.
This case handled by forming a strip of points
whose Xco-ords distance is less than closest_pair_dis
from mid-point's Xco-ords. Points sorted based on Yco-ords
are used in this step to reduce sorting time.
Closest pair distance is found in the strip of points. (closest_in_strip)

min(closest_pair_dis, closest_in_strip) would be the final answer.

Time complexity: O(n * log n)
"""
import numpy.random as nprand
from timeit import default_timer as timer
import porpoise

def euclidean_distance_sqr(point1, point2):
    return (point1[0] - point2[0]) ** 2 + (point1[1] - point2[1]) ** 2


def column_based_sort(array, column = 0):
    return sorted(array, key = lambda x: x[column])


def dis_between_closest_pair(points, points_counts, min_dis = float("inf")):
    """ brute force approach to find distance between closest pair points

    Parameters :
    points, points_count, min_dis (list(tuple(int, int)), int, int)

    Returns :
    min_dis (float):  distance between closest pair of points

    """

    for i in range(points_counts - 1):
        for j in range(i+1, points_counts):
            current_dis = euclidean_distance_sqr(points[i], points[j])
            if current_dis < min_dis:
                min_dis = current_dis
    return min_dis


def dis_between_closest_in_strip(points, points_counts, min_dis = float("inf")):
    """ closest pair of points in strip

    Parameters :
    points, points_count, min_dis (list(tuple(int, int)), int, int)

    Returns :
    min_dis (float):  distance btw closest pair of points in the strip (< min_dis)

    """

    for i in range(min(6, points_counts - 1), points_counts):
        for j in range(max(0, i-6), i):
            current_dis = euclidean_distance_sqr(points[i], points[j])
            if current_dis < min_dis:
                min_dis = current_dis
    return min_dis


def par_closest_pair_of_points_sqr(thr_count, points_sorted_on_x, points_sorted_on_y, points_counts):
    """ divide and conquer approach

    Parameters :
    points, points_count (list(tuple(int, int)), int)

    Returns :
    (float):  distance btw closest pair of points

    """

    # base case
    if points_counts <= 3:
        return dis_between_closest_pair(points_sorted_on_x, points_counts)

    if thr_count == 1:
        return closest_pair_of_points_sqr(points_sorted_on_x, points_sorted_on_y, points_counts)

    # recursion
    mid = points_counts//2
    thr_mid = thr_count//2
    t0 = porpoise.Thread(par_closest_pair_of_points_sqr, thr_mid, points_sorted_on_x,
                                                 points_sorted_on_y[:mid],
                                                 mid)
    closest_in_right = par_closest_pair_of_points_sqr(thr_count - thr_mid,
                                                     points_sorted_on_x,
                                                     points_sorted_on_y[mid:],
                                                     points_counts - mid)
    closest_in_left = t0.join()
    closest_pair_dis = min(closest_in_left, closest_in_right)

    """ cross_strip contains the points, whose Xcoords are at a
    distance(< closest_pair_dis) from mid's Xcoord
    """

    cross_strip = []
    for point in points_sorted_on_x:
        if abs(point[0] - points_sorted_on_x[mid][0]) < closest_pair_dis:
            cross_strip.append(point)

    closest_in_strip = dis_between_closest_in_strip(cross_strip,
                     len(cross_strip), closest_pair_dis)
    return min(closest_pair_dis, closest_in_strip)


def par_closest_pair_of_points(thr_count, points, points_counts):
    t0 = porpoise.Thread(column_based_sort, points, column = 1)
    points_sorted_on_x = column_based_sort(points, column = 0)
    points_sorted_on_y = t0.join()
    return (par_closest_pair_of_points_sqr(thr_count, points_sorted_on_x,
                                       points_sorted_on_y,
                                       points_counts)) ** 0.5


def closest_pair_of_points_sqr(points_sorted_on_x, points_sorted_on_y, points_counts):
    """ divide and conquer approach

    Parameters :
    points, points_count (list(tuple(int, int)), int)

    Returns :
    (float):  distance btw closest pair of points

    """

    # base case
    if points_counts <= 3:
        return dis_between_closest_pair(points_sorted_on_x, points_counts)

    # recursion
    mid = points_counts//2
    closest_in_left = closest_pair_of_points_sqr(points_sorted_on_x,
                                                 points_sorted_on_y[:mid],
                                                 mid)
    closest_in_right = closest_pair_of_points_sqr(points_sorted_on_x,
                                                  points_sorted_on_y[mid:],
                                                  points_counts - mid)
    closest_pair_dis = min(closest_in_left, closest_in_right)

    """ cross_strip contains the points, whose Xcoords are at a
    distance(< closest_pair_dis) from mid's Xcoord
    """

    cross_strip = []
    for point in points_sorted_on_x:
        if abs(point[0] - points_sorted_on_x[mid][0]) < closest_pair_dis:
            cross_strip.append(point)

    closest_in_strip = dis_between_closest_in_strip(cross_strip,
                     len(cross_strip), closest_pair_dis)
    return min(closest_pair_dis, closest_in_strip)


def closest_pair_of_points(points, points_counts):
    points_sorted_on_x = column_based_sort(points, column = 0)
    points_sorted_on_y = column_based_sort(points, column = 1)
    return (closest_pair_of_points_sqr(points_sorted_on_x,
                                       points_sorted_on_y,
                                       points_counts)) ** 0.5

def main(thr_count, loops, file, series):
    MIN = 6
    MAX = 12
    for y in range(MIN,MAX+1):
        size = 2**y
        num_time = timer()
        num_time -= num_time
        porp_time = num_time
        for x in range(loops):
            points = [tuple([nprand.randint(-4096,4096) for r in range(2)]) for i in range(size)]
            start = timer()

            par_closest_pair_of_points(thr_count, points, len(points))

            porp_time += timer() - start

            start = timer()

            closest_pair_of_points(points, len(points))

            num_time += timer() - start

        res = [series, ", closest_pair, ", thr_count, ", ", size, ", ", loops, ", ", round(porp_time, 4), ", ", round(num_time,4), "\n"]
        file.write("".join(str(s) for s in res))
        file.flush()
