from sys import stdin
from re import sub, findall
from wrappers import Thread

output_file = open("bench_output-regexredux_mp.txt", mode="w")

def var_find(f):
    return len(findall(f, seq))

def main():
    with open("bench_output-fasta_mp.txt", mode="r") as f:
        global seq
        seq = f.read()
    ilen = len(seq)

    seq = sub('>.*\n|\n', '', seq)
    clen = len(seq)

    variants = (
          'agggtaaa|tttaccct',
          '[cgt]gggtaaa|tttaccc[acg]',
          'a[act]ggtaaa|tttacc[agt]t',
          'ag[act]gtaaa|tttac[agt]ct',
          'agg[act]taaa|ttta[agt]cct',
          'aggg[acg]aaa|ttt[cgt]ccct',
          'agggt[cgt]aa|tt[acg]accct',
          'agggta[cgt]a|t[acg]taccct',
          'agggtaa[cgt]|[acg]ttaccct')

    threads = []
    for i, variant in enumerate(variants):
        t = Thread(target=var_find, args=(variant,))
        t.start()
        threads.append((i, t))

    results = [None for i in range(len(variants))]
    while len(threads) != 0:
        for i in range(len(threads)):
            j, t = threads[i]
            if t.join(0):
                assert (t.get_exit_status() == 0)
                results[j] = (t.get_result())
                threads.pop(i)
                break

    for f in zip(variants, results):
        output_file.write("%s %s\n" % (f[0], f[1]))

    subst = {
          'tHa[Nt]' : '<4>', 'aND|caN|Ha[DS]|WaS' : '<3>', 'a[NSt]|BY' : '<2>',
          '<[^>]*>' : '|', '\\|[^|][^|]*\\|' : '-'}
    for f, r in list(subst.items()):
        seq = sub(f, r, seq)

    output_file.write("\n")
    output_file.write("%s\n" % ilen)
    output_file.write("%s\n" % clen)
    output_file.write("%s\n" % len(seq))

if __name__=="__main__":
    main()
