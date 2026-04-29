#! /usr/bin/python3

from datetime import datetime
from ml_side.lg_stats import *
import os
import sys

if __name__ == "__main__":
    THINK_TIME = sys.argv[1]
    NUM_USERS  = sys.argv[2]
    URL = sys.argv[3]
    TEST_DURATION = sys.argv[4]
    DIR = datetime.now().strftime("%d%m%y%H%M")
    
    os.system(f"mkdir -p {DIR}")
    os.system(f"./lgen {NUM_USERS} {THINK_TIME} {TEST_DURATION} {URL} {DIR}/")

    think_times = []
    response_times = []

    logfile = DIR + "/stats.log"
    os.system(f"echo {NUM_USERS} > {logfile}")
    os.system(f"cat {DIR}/userlog* >> {logfile}")
    do_stuff(logfile)