from sys import argv
from sys import path
import time
import calendar

script, porpoise_path = argv
path.append(porpoise_path)

import regex_v8.bm_regex_v8 as bm_regex_v8
import regex_dna.bm_regex_dna as bm_regex_dna

loops = 10
threads = [64]
programs = [
            ('regex v8', bm_regex_v8.main),
            ('regex dna', bm_regex_dna.main)
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
