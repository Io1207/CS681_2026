import numpy as np
import matplotlib.pyplot as plt
import os
import io
import math
import sys
from stats import *

exp_dir = ""
num_runs = 10

def make_change_config(f : io.TextIOWrapper, num_servers):
    print("How many change events do you want to add?")
    n = int(input().strip())
    f.write(str(n) + "\n")
    
    print ("To enter a config change event enter in the following format:")
    print ("Time of change, type of change (described below)")
    print ("If both routing and server settings change enter \"2\" first")
    print ("If only routing changes enter \"1\" first")
    print ("If only server settings change enter \"0\" first")
    print ("for adjacency matrix change just write in the same format as before")
    print ("for server setting change write like this (for every single server regardless of whether or not it's settings are being changed): ")
    print ("sId numCores maxThreads maxQueue serviceMin serviceVar")
    for i in range(n):
        print("Change event", i+1)
        time, changeType = input().strip().split()
        f.write(time + " " + changeType + "\n")
        if changeType == "2":
            # num of lines written = num_servers + 1 (for the adjacency matrix) + num_servers (for the server settings)
            make_adjacency_matrix(num_servers+1, f)
            for i in range(num_servers):
                print("Enter server settings for server", i+1, "like this: ")
                print("sId numCores maxThreads maxQueue serviceMin serviceVar")
                x = input().strip()
                print(x)
                f.write(x + "\n")
        elif changeType == "1":
            # num of lines written = num_servers + 1 (for the adjacency matrix)
            make_adjacency_matrix(num_servers+1, f)
        elif changeType == "0":
            # number of lines written = num_servers
            for i in range(num_servers):
                print("Enter server settings for server", i+1, "like this: ")
                print("sId numCores maxThreads maxQueue serviceMin serviceVar")
                x = input().strip()
                print(x)
                f.write(x + "\n")
    

def get_server_config(num_servers, f : io.TextIOWrapper):
    # now will ask for server details one by one
    for i in range(num_servers):
        print(f"Enter details for server {i+1} like this: ")
        print(f"numCores maxThreads maxQueue dist serviceMin")
        print(f"serviceVar contextSwitchTime roundRobinTimeout timeout")
        f.write(str(input()).strip() + "\n")
        f.write(str(input()).strip() + "\n")

def make_adjacency_matrix(num_servers, f : io.TextIOWrapper):
    # I am lazy and would prefer to not have to write the adjacency matrix myself
    print("enter the outProbs for the each server after it's id is printed like this: ")
    print("sId1 outProbcurr->sId1 outProbcurr->sId2 ... outProbcurr->sIdn")
    adj_matrix = np.zeros((num_servers, num_servers), dtype = float)
    for j in range(num_servers):
        print("Emmission matrix for server", j)
        response = str(input()).strip()
        parts = response.split()
        n = int(len(parts)/2)
        for i in range(n):
            adj_matrix[j][int(parts[i*2])] = parts[i*2 + 1]

    for i in range(num_servers):
        for j in range(num_servers):
            f.write(str(adj_matrix[i][j]) + " ")
        f.write("\n")

def make_config():
    print("Will now ask for info to make the configuration file for the simulation")
    print("The number of runs is 10 by default; if you want to change this, go change it in sim.h and run.py right now. Go!!")

    with open(f"{exp_dir}/config.txt", "w") as f:
        f.write(str(input("Number of delays: ")).strip() + "\n")
        f.write(str(input("Enter number of users: ")).strip() + "\n")
        f.write(str(input("Ease in time: ")).strip() + "\n")
        f.write(str(input("Min Client timeout: ")).strip() + "\n")
        f.write(str(input("Client timeout Variance: ")).strip() + "\n")
        f.write(str(input("Think time min: ")).strip() + "\n")
        f.write(str(input("Think time variance: ")).strip() + "\n")
        num_servers = int(input("Number of servers: ").strip())
        f.write(str(num_servers) + "\n")
        # for i in range(num_servers):
        #     os.system(f"touch {exp_dir}/server_{i+1}")
        print("Now will be asking for server details one by one")
        make_adjacency_matrix(num_servers+1,f)
        get_server_config(num_servers,f)
    
    f.close()
    # os.system(f"cd ../../")

def make_exp_config():
    print("Will now ask for info to make the configuration file for the simulation")
    print("The number of runs is 10 by default; if you want to change this, go change it in sim.h and run.py right now. Go!!")

    with open(f"{exp_dir}/config.txt", "w") as f:
        f.write(str(input("Number of delays: ")).strip() + "\n")
        f.write(str(input("Enter number of users [min, max, step]: ")).strip() + "\n")
        f.write(str(input("Ease in time: ")).strip() + "\n")
        f.write(str(input("Min Client timeout: ")).strip() + "\n")
        f.write(str(input("Client timeout Variance: ")).strip() + "\n")
        f.write(str(input("Think time min: ")).strip() + "\n")
        f.write(str(input("Think time variance: ")).strip() + "\n")
        num_servers = int(input("Number of servers: ").strip())
        f.write(str(num_servers) + "\n")
        # for i in range(num_servers):
        #     os.system(f"touch {exp_dir}/server_{i+1}")
        print("Now will be asking for server details one by one")
        make_adjacency_matrix(num_servers+1,f)
        get_server_config(num_servers,f)
        print("wtf")
        print("Any change to the config files? (y/n)")
        if (str(input()).strip() == "y"):
            f.write("1\n")
            make_change_config(f, num_servers)
        else:
            f.write("0\n")
    
    f.close()
    # os.system(f"cd ../../")

def run_sim(sim_dir):
    print(sim_dir)
    # os.system(f"cd ..")
    # for i in range(num_runs):
        # os.system(f"touch main_{i}")
    for i in range (1,11):
        os.system(f"mkdir -p {sim_dir}/run_{i}")
    seed = np.random.randint(0, 10000)
    
    os.system(f"./src/sim ./{sim_dir}/ {seed}")
    

def gen_sim_cfg(lines, users, file):
    lines[1] = f"{users}\n"
    with open(file, "w") as f:
        # f.write(lines)
        f.writelines(lines)


def sim_cfg_from_expt(dirname):
    exp_cfg = f"{dirname}/config.txt"
    exp_file = open(exp_cfg, "r")
    lines = exp_file.readlines()
    start, stop, step = map(int, lines[1].strip().split())
    dirs = []
    for u in range(start,stop,step):
        os.system(f"mkdir -p {dirname}/{u}_users")
        gen_sim_cfg(lines, u, f"{dirname}/{u}_users/config.txt")
        dirs.append(f"{dirname}/{u}_users")
    return dirs, range(start, stop, step)


def run_exp(exp_dir):
    print(exp_dir)
    sims, users = sim_cfg_from_expt(exp_dir)
    for s in sims:
        run_sim(s)

    stats = get_exp_stats(exp_dir, users, num_runs)
    plot_stats(exp_dir, users, stats)




def welch_cutoff(consolidated_file_name, r, m, win_size, tol = 0.0025):
    '''
        reads from a file which has the response times of all the runs, and calculates the cutoff time using welch's method, and returns it
    '''
    with open(consolidated_file_name, "r") as f:
        response_times = [0] * m
        m_ = 0
        window_means = [0] * (m - win_size + 1)
        for line in f:
            # each line has r response times one for each run
            parts = line.strip().split()
            # convert all elements of parts to float
            parts = [float(x) for x in parts]
            response_times[m_] = np.mean(parts[:r])
            if m_ < win_size:
                m_ +=1
                continue
            window_means[m_ - win_size] = np.mean(response_times[m_ - win_size : m_])
            m_ += 1
        # now we have the window means, we can calculate the cutoff time using welch's method
        cutoff_time = 0
        plt.plot(window_means)
        plt.title("Window Means (for Welch)")
        plt.show()
        plt.close()
        for i in range(m - win_size + 1 - 10):
            if abs(window_means[i] - window_means[i + 10]) < tol:
                cutoff_time = i + win_size
                break
    return cutoff_time


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Enter Argument: ")
        exit()
    os.system(f"cd src && make")
    if sys.argv[1] == "expt":
        test_name = str(input("Enter the test name: "))
        exp_dir = f"results/{test_name}"
        os.system(f"mkdir -p {exp_dir}")
        os.system(f"touch {exp_dir}/config.txt")
        # for i in range (1,11):
        #     os.system(f"mkdir -p {exp_dir}/run_{i}")
        # os.system(f"cd results/{test_name}")
        make_exp_config()
        run_exp(exp_dir)
    elif sys.argv[1] == "sim":
        test_name = str(input("Enter the test name: "))
        exp_dir = f"results/{test_name}"
        os.system(f"mkdir -p {exp_dir}")
        os.system(f"touch {exp_dir}/config.txt")
        
        # os.system(f"cd results/{test_name}")
        make_config()
        run_sim(exp_dir)
    elif sys.argv[1] == "rerun_expt":
        exp_dir = sys.argv[2]
        run_exp(exp_dir)
    elif sys.argv[1] == "rerun_sim":
        exp_dir = sys.argv[2]
        run_sim(exp_dir)
    else:
        print("Invalid arg1")
        exit()
    