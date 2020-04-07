import json
import os
import subprocess
import sys
import time

# get arguments
if len(sys.argv) < 3:
    print("insufficient number of arguments\n")
    print("Usage: python3 bench.py <bench_target> <program_path> [skip]")
    print("\t- <bench_target>: benchmark target")
    print("\t- <program_path>: where the benchmark programs are located")
    print("\t- [skip]: files to skip, separated by [,]")
    print("\nExample: python3 bench.py 'multiprocessing' ../examples/multiprocessing/ 'wrappers.py'")
    sys.exit(1)
else:
    bench_target = sys.argv[1]
    program_path = sys.argv[2]
    if len(sys.argv) >= 4:
        skip = sys.argv[3].split(',')
    else:
        skip = []

# get programs
programs = [("%s-%s" % (bench_target, "dummy"), 'dummy.py')]
for f in sorted(os.listdir(program_path)):
    if f.endswith(".py") and f not in skip:
        bench_name = "%s-%s" % (bench_target, f[:-3])
        file_path = program_path + f
        programs.append((bench_name, file_path))

# do benchmark
json_combined = []
for (bench_name, file_path) in programs:
    print("running", bench_name)
    json_file = "%s.json" % bench_name

    result = subprocess.run(
        ["hyperfine",
         "--warmup", str(5),
         "--max-runs", str(50),
         "--export-json", json_file,
         "/usr/bin/python3 %s" % file_path],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        encoding="utf-8")

    print(bench_name, "exited with", result.returncode)
    if result.returncode == 0:
        print(bench_name, "stdout:")
        print(result.stdout)
    else:
        print(bench_name, "ran with:", result.args)
        print(bench_name, "stdout:")
        print(result.stdout)
        print(bench_name, "stderr:")
        print(result.stderr)
    print("========================================")

    # merge json
    with open(json_file, 'r') as f:
        results = json.load(f)["results"]
        for result in results:
            result["bench_name"] = bench_name
        json_combined.extend(results)
    os.remove(json_file)

# dump json
with open("%s-%s.json" % (bench_target, int(time.time())), 'w') as f:
    json.dump(json_combined, f, indent=2)
