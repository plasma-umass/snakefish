from sys import stdin
from re import sub, findall
from multiprocessing import Pool

output_file = open("bench_output-regexredux_bg.txt", mode="w")

def var_find(f):
    return len(findall(f, seq))

def main():
    with open("bench_output-fasta_bg.txt", mode="r") as f:
        global seq
        seq = f.read()
    ilen = len(seq)

    seq = sub('>.*\n|\n', '', seq)
    clen = len(seq)

    pool = Pool()

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
    for f in zip(variants, pool.imap(var_find, variants)):
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
