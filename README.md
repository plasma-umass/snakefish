# Porpoise

## Outline of Repo

### bench
Includes the benchmark programs. More on `Current State of Things` --> `Benchmarks`.

### example
A few dummy and toy programs to work with Porpoise.

- generator: An example of porpoise's generator handling. Runs a generator in parallel. Run with `python3 generator.py [directory of porpoise src files]`.
- pass_cracker: An example of porpoise's return value handling. Cracks a bunch of passwords given via an input file in parallel. Run with `python3 pass_cracker.py [directory of porpoise src files] [input file]`.
- porpals: An example of porpoise's global handling. Does some simple processing on a number of builtin types and merges different versions at join time. Run with `python3 porp_example.py [directory of porpoise src files]`
- write: An examples of porpoies's I/O handling. Two different threads write seperately to a file. Porpoise merges the conflicts according to policy. Run with `python3 write.py [directory of porpoise src files]`


### labratory
A directory of experiements. For information look at `Current State of Things` --> `Lab`.
- globals: porpal exeperiements.
- io: file I/O experiements.
- porpoise: experiments directly related to porpoise. Includes pybind11 code to do the processing and the implemented file I/O dynamic library.

### policy
A number of text files outlining our policies for porpoise features (such as globals and I/O).

### src
Where the src code and a number of fake versions of porpoise (they implement the API, but not the underlying mechanism).

## Current State of Things

### Reflections
Porpise doesn't have a lot of moving components, which makes it extremely simple. I guess the simplicity is kinda the point. Most of the programs just need a simple parallelization via a function call and done. I guess porpoise is also advocating for a new mindset when it comes to parallel programming.

Development took a lot of experimentation and understanding python. Unfortunately the nature of the language limits a lot of complex additions to the project. The main issue with adding complex features was the we were unsure what would be useful and where. I recommend that in the future, first find reasonable examples where something is needed and then implement a feature for it, espcially if Python is not your primary language and you don't have a lot of knowledge about it.

### Lab

#### File I/O
It is not perfect. The policy is semi implemented. File prints work. Streams, not so much. Future work could include implmenting file system support or stream support. Sharing the FS table could also be beneficial.

#### Porpals
Similar to concurrent revisions. We could try to improve logs.

#### Return Values
I think we can make improvements in this are by reducing pickling & depickling cost + transfer cost by doing some memory management tricks. It is quite fast though.

#### C Backend
A c backend could be added to improve performance within the library. Make sure the c backend does not break anything.

#### Map/Automatic loop?
Could work. Suject to debate if it fits within the system. Could be an example easy development for porpoise.

### Benchmarks

#### What is needed?
Need some real programs. Like, really really real ones. And parallelize them.Also a program that uses porpals and file I/O. regex dna could use porpals, but find something more substantial.

For each set of benchmarks, run `python3 bench.py [directory of porpoise src files]`

#### Micro Benchmarks
- vml. Takes a matrix and updates all of its components via some calculations. Comparison between numpy, which is single threaded, made parallel via SnakeFish and numexpr, which is parallel on its own. Return value is semi large (about 1 GB in total) per run. Speed up anywhere between 5.2x (0.115 seconds on SnakeFish) to 1.05x (3.6 seconds on SnakeFish). Varies based on vector size.
- sum. Does some evaluations on a number of matrices and then sums up all the components. In this case, the comparison is between only numexpr and numexpr + SnakeFish. Return values are just a single integer. Slowdown anywhere between 3x (.17 on SnakeFish) to 1.4x (7.4 seconds on SnakeFIsh).
- matrix. Just a simple matrix multiply. Comparison between numpy and numpy parallelized via SnakeFish. Return values are at most 8 GB per run. This bench mark is also good for recording cost of pickling and depickling data. Speedup anywhere between 6.1x (21.3 second SnakeFish) to 1.2x (0.1 seconds on SnakeFish).
- flt. A synthetic benchmark that does a bunch of flt operations on a number of points and then returns the maximum x, y, & z among the points. Return values are quite small. For runs that are shorter than 0.18 seconds, we get slowdowns upto 90x (normal program takes 0.0015 seconds). For anything larger, we have speedups between 1x (0.18 seconds on SnakeFish) to 1.7x (3.4 seconds on SnakeFish).

#### Algorithms Made Parallel

- Closest pair of points. Divide and Conquer parallelized. Return value is just a pair of points. Speedup anywhere between 1x (0.025 seconds on SnakeFish) to 27x (0.67 seconds on SnakeFish).
- Maximum subarray sum. Divide and Conquer parallelized. Return value is just a sum. For runs that are shorter than 0.021 seconds, we get slowdowns upto 174x (normal program takes 0.00012 seconds). For anything larger, we have speedups between 1.5x (0.02 seconds on SnakeFish) to 12x (0.24 seconds on SnakeFish).
- Edge Detection. An image edge detection algorithm. No clue how it works, I just parallelized all the loops that I could. Return values are entire array slices. For all images, there is an speedup for these, anywhere from 3.2x (0.93 seconds on SnakeFish) to 5x (4 seconds on SnakeFish).


#### Pyperformance

- regex_v8. This benchmark is generated by loading 50 of the most popular pages on the web and logging all regexp operations performed. Each operation is given a weight that is calculated from an estimate of the popularity of the pages where it occurs and the number of times it is executed while loading each page. Finally the literal letters in the data are encoded using ROT13 in a way that does not affect how the regexps match their input. This could not be parallelized to 64 cores (max to 11), but we had speed up of 1.6x (0.17 seconds on SnakeFish).
- regex_dna. regex DNA benchmark using “fasta” to generate the test case. Return values are small lists. Speedup anywhere from 1.7x (0.17 seconds on SnakeFish) to 8.5x (0.83 seconds on SnakeFish).
- 2to3. Some weird library bug that won't let me run it in a modified library.
