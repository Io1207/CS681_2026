#! /usr/bin/python3
def do_stuff(filename="stats.log"):
    logfile = filename

    response_times = []
    think_times = []

    total_snd = 0
    total_rcv = 0
    success_rcv = 0
    dropped_packets = 0
    error = 0

    first_ts = None
    last_ts = None

    with open(logfile) as f:
        num_users = int(f.readline().strip())

        snd_times = [[] for _ in range(num_users)]
        last_rcv_ts = [None] * num_users   # per-user last successful rcv

        for line in f:
            # print(line,",",end="")
            parts = line.strip().split()

            uid = int(parts[0])
            typ = parts[1]

            if typ == "snd":
                # print(1)
                ts = int(parts[2])

                # compute think time if previous rcv exists
                if last_rcv_ts[uid] is not None:
                    think_times.append(ts - last_rcv_ts[uid])
                    last_rcv_ts[uid] = None

                snd_times[uid].append(ts)
                total_snd += 1

                if first_ts is None or ts < first_ts:
                    first_ts = ts

            elif typ == "rcv":
                # print(2)
                ts = int(parts[2])
                http_code = int(parts[3])
                total_rcv += 1

                if last_ts is None or ts > last_ts:
                    last_ts = ts

                if snd_times[uid]:
                    snd_ts = snd_times[uid].pop(0)
                    latency = ts - snd_ts

                    if http_code == 200:
                        success_rcv += 1
                        response_times.append(latency)
                        last_rcv_ts[uid] = ts
                    else:
                        dropped_packets += 1
                else:
                    dropped_packets += 1
            else:
                # print(3)
                error += 1

    # unmatched sends → dropped
    for l in snd_times:
        dropped_packets += len(l)

    duration_sec = (
        (last_ts - first_ts) / 1000.0
        if first_ts is not None and last_ts is not None else 0
    )

    avg_response_time = (
        sum(response_times) / len(response_times)
        if response_times else 0
    )

    avg_think_time = (
        sum(think_times) / len(think_times)
        if think_times else 0
    )

    avg_throughput = (
        success_rcv / duration_sec
        if duration_sec > 0 else 0
    )

    print(f"first_ts - {first_ts}, last_ts - {last_ts}")

    print(f"Users                    : {num_users}")
    print(f"Packets sent             : {total_snd}")
    print(f"Successful responses     : {success_rcv}")
    print(f"Packets dropped          : {dropped_packets}")
    print(f"Error packets            : {error}")
    print(f"Avg response time (ms)   : {avg_response_time:.2f}")
    print(f"Avg think time (ms)      : {avg_think_time:.2f}")
    print(f"Avg throughput (pkt/s)   : {avg_throughput:.2f}")

    # Little's Law check
    N_est = avg_throughput * ((avg_response_time + avg_think_time) / 1000.0)
    print(f"Little's Law estimate N  : {N_est:.2f}")

if __name__ == "__main__":
    do_stuff()
