import os
import subprocess
import sys

targets = [
    "benchmarksgame",
    "multiprocessing",
    "sequential",
    "snakefish"
]

benchmarks = [
    ("binary_tree", "10"),
    ("fannkuch", "7"),
    ("fasta", "1000"),
    ("knucleotide", ""),
    ("mandelbrot", "200"),
    ("regexredux", ""),
    ("spectralnorm", "100")
]


def run_and_get_output(target, benchmark, arg):
    args = ["python3", "%s.py" % benchmark, arg]

    result = subprocess.run(
        args,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        encoding="utf-8",
        cwd=target)

    return result.stdout


def validate_output(target, benchmark, output):
    # read expected output
    with open("output/%s-output.txt" % benchmark, mode="rb") as f:
        expected = f.read()

    # if actual output is written to a file
    if len(output) == 0:
        output_path = None
        for file in os.listdir(target):
            if file.startswith("bench_output-%s" % benchmark):
                output_path = "%s/%s" % (target, file)
                break

        if output_path is None:
            print("can't find output file of %s-%s" % (target, benchmark))
            sys.exit(1)

        with open(output_path, mode="rb") as f:
            output = f.read()
    else:
        # if actual output is a string, convert to bytes
        output = output.encode()

    # check output
    if output == expected:
        print("%s-%s passed" % (target, benchmark))
    else:
        print("%s-%s failed" % (target, benchmark))

        dump_to = "failed_%s-%s" % (target, benchmark)
        with open(dump_to, mode="wb") as f:
            f.write(output)

        sys.exit(1)


for target in targets:
    for benchmark, arg in benchmarks:
        out = run_and_get_output(target, benchmark, arg)
        validate_output(target, benchmark, out)
