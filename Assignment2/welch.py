from run import welch_cutoff
import sys
import os



def consolidate(dirname, num_runs):
    file = open(f"{dirname}/consolidated.tsv", "w")
    handles = [open(f"{dirname}/run_{i}/main.log") for i in range(1,num_runs+1)]
    lines = 0
    while True:
        values = []
        for h in handles:
            line = h.readline()
            if not line:
                break
            values.append(line.strip().split(",")[5])

        if len(values) != len(handles):
            break
        lines+=1
        file.write(" ".join(values) + "\n")
    for h in handles:
        h.close()
    file.close()
    return lines


if __name__ == "__main__":
    dirname = sys.argv[1]
    if(len(sys.argv) > 2):
        num_runs = int(sys.argv[2])
    else:
        num_runs = 10

    m = consolidate(dirname, num_runs)
    co = welch_cutoff(f"{dirname}/consolidated.tsv", num_runs, m, 40)
    print("Calculated Welch Cutoff:", co)


