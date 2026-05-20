import subprocess
import random
import csv
import time

def parse_output(output):
    pass

def run_experiment(config):
    # 1. start docker
    subprocess.run(["php", "dockerStart.php", str(config["cpu"]), str(config["mem"])])

    # 2. run httperf
    result = subprocess.run([
        "httperf",
        "--server=localhost",
        f"--num-conns={config['users']}",
        "--rate=..."
    ], capture_output=True, text=True)

    # 3. parse response time
    response_time = parse_output(result.stdout)

    # 4. stop docker
    subprocess.run(["php", "dockerStop.php"])

    return response_time