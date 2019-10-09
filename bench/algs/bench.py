from sys import argv
from sys import path
import time
import calendar

script, porpoise_path = argv
path.append(porpoise_path)

import closest_pair.closest_pair_of_points as closest_pair_of_points
import max_sum.max_subarray_sum as max_subarray_sum
import edge_detection.canny as canny

loops = 10
threads = [64]
programs = [
            #('closest pair', closest_pair_of_points.main),
            #('max subarray sum', max_subarray_sum.main),
            ('edge detection', canny.main)
            ]

def bench():
    series = calendar.timegm(time.gmtime())
    print(series)
    with open('./bench_res.txt', 'a') as f:
        for thread_count in threads:
            for prog in programs:
                print("Running ", prog[0], "with ", thread_count, "threads on ", loops ,"loops.")
                prog[1](thread_count, loops, f, series)

if __name__=="__main__":
    bench()
