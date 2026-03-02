import numpy as np
import matplotlib.pyplot as plt
import os
import io

def get_server_config(num_servers, f : io.TextIOWrapper):
    # now will ask for server details one by one
    for i in range(num_servers):
        print(f"Enter details for server {i+1} like this: ")
        print(f"numCores maxThreads maxQueue dist serviceMin")
        print(f" serviceVar contextSwitchTime roundRobinTimeout timeout")
        f.write(str(input()).strip() + "\n")
        f.write(str(input()).strip() + "\n")

def make_adjacency_matrix(num_servers, f : io.TextIOWrapper):
    # I am lazy and would prefer to not have to write the adjacency matrix myself
    adj_matrix = np.zeros((num_servers, num_servers), dtype = float)
    for j in range(num_servers+1):
        response = str(input()).strip()
        parts = response.split()
        n = len(parts)/2
        for i in range(n):
            adj_matrix[0][parts[i*2]] = parts[i*2 + 1]

    for i in range(len(num_servers)):
        for j in range(len(num_servers)):
            f.write(str(adj_matrix[i][j]) + " ")
        f.write("\n")

def make_config():
    print("Will now ask for info to make the configuration file for the simulation")
    print("The number of runs is 10 by default; if you want to change this, go change it in sim.h right now. Go!!")

    with open("config.txt", "w") as f:
        f.write(str(input("Number of delays: ")).strip() + "\n")
        f.write(str(input("Enter number of users: ")).strip() + "\n")
        f.write(str(input("Ease in time: ")).strip() + "\n")
        f.write(str(input("Client timeout: ")).strip() + "\n")
        f.write(str(input("Think time min: ")).strip() + "\n")
        f.write(str(input("Think time variance: ")).strip() + "\n")
        num_servers = int(input("Number of servers: ").strip())
        f.write(str(num_servers) + "\n")
        print("Now will be asking for server details one by one")
        make_adjacency_matrix(num_servers+1,f)
        get_server_config(num_servers,f)
    
    f.close()

    os.system(f"cd ../../")

def run_sim():
    os.system(f"cd src && make")
    os.system(f"cd ..")
    for i in range(10):
        os.system(f"touch {i}_log")
    os.system(f"./src/sim")

def welch_cutoff(consolidated_file_name):
    with open(consolidated_file_name, "r") as f:
        lines = f.readlines()
        response_times = []
        for line in lines:
            

if __name__ == "__main__":
    test_name = str(input("Enter the test name: "))
    os.system(f"mkdir -p results/{test_name}")
    os.system(f"touch results/{test_name}/config.txt")
    os.system(f"mkdir -p results/{test_name}/runs")

    os.system(f"cd results/{test_name}")
    make_config()
