import numpy as np
import matplotlib.pyplot as plt
import os
import io

def get_server_config(num_servers, f : io.TextIOWrapper):
    # now will ask for server details one by one
    for i in range(num_servers):
        print(f"Enter details for server {i+1} like this: ")
        print(f"numCores maxThreads threadBuffer maxQueue dist serviceMin")
        print(f" serviceVar contextSwitchTime roundRobinTimeout timeout")
        f.write(str(input()).strip() + "\n")
        f.write(str(input()).strip() + "\n")

def make_adjacency_matrix(num_servers, f : io.TextIOWrapper):
    # I am lazy and would prefer to not have to write the adjacency matrix myself
    print("enter the outProbs for the each server after it's id is printed like this: ")
    print("sId1 outProbcurr->sId1 outProbcurr->sId2 ... outProbcurr->sIdn")
    adj_matrix = np.zeros((num_servers, num_servers), dtype = float)
    for j in range(num_servers+1):
        print("Emmission matrix for server", j)
        response = str(input()).strip()
        parts = response.split()
        n = int(len(parts)/2)
        for i in range(n):
            adj_matrix[j][parts[i*2]] = parts[i*2 + 1]

    for i in range(len(num_servers)):
        for j in range(len(num_servers)):
            f.write(str(adj_matrix[i][j]) + " ")
        f.write("\n")

def make_config():
    print("Will now ask for info to make the configuration file for the simulation")
    print("The number of runs is 10 by default; if you want to change this, go change it in sim.h and run.py right now. Go!!")

    with open("config.txt", "w") as f:
        f.write(str(input("Number of delays: ")).strip() + "\n")
        f.write(str(input("Enter number of users: ")).strip() + "\n")
        f.write(str(input("Ease in time: ")).strip() + "\n")
        f.write(str(input("Client timeout: ")).strip() + "\n")
        f.write(str(input("Think time min: ")).strip() + "\n")
        f.write(str(input("Think time variance: ")).strip() + "\n")
        num_servers = int(input("Number of servers: ").strip())
        f.write(str(num_servers) + "\n")
        for i in range(num_servers):
            os.system(f"touch server_{i+1}")
        print("Now will be asking for server details one by one")
        make_adjacency_matrix(num_servers+1,f)
        get_server_config(num_servers,f)
    
    f.close()
    os.system(f"cd ../../")

def run_sim(num_runs):
    os.system(f"cd src && make")
    os.system(f"cd ..")
    for i in range(num_runs):
        os.system(f"touch main_{i}")
    os.system(f"./src/sim")

def welch_cutoff(consolidated_file_name, r, m, win_size, tol = 0.0025):
    '''
        reads from a file which has the response times of all the runs, and calculates the cutoff time using welch's method, and returns it
    '''
    with open(consolidated_file_name, "r") as f:
        lines = f.readlines()
        response_times = ['_'] * m
        m_ = 0
        window_means = ['_'] * (m - win_size + 1)
        for line in lines:
            # each line has r response times one for each run
            parts = line.strip().split()
            # convert all elements of parts to float
            parts = [float(x) for x in parts]
            response_times[m_] = np.mean(parts[:r])
            if m_ < win_size:
                m+=1
                continue
            window_means[m_ - win_size] = np.mean(response_times[m_ - win_size - 1 : m_ + 1])
            m_ += 1
        # now we have the window means, we can calculate the cutoff time using welch's method
        cutoff_time = 0
        for i in range(m - win_size + 1):
            if abs(window_means[i] - window_means[i + m * 0.05]) < tol:
                cutoff_time = i + win_size
                break

    f.close()
    return cutoff_time


if __name__ == "__main__":
    test_name = str(input("Enter the test name: "))
    os.system(f"mkdir -p results/{test_name}")
    os.system(f"touch results/{test_name}/config.txt")
    os.system(f"mkdir -p results/{test_name}/runs")

    os.system(f"cd results/{test_name}")
    make_config()
