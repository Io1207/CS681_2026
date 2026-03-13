import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
from run import welch_cutoff
from run_welch import consolidate


main_cols = ['time','good','bad','dropped','completed','resp']

def get_sim_stats(sim_dir, num_runs, window = 40, cutoff = None):
	if (cutoff is None):
		m = consolidate(sim_dir, num_runs)
		cutoff = welch_cutoff(f"{sim_dir}/consolidated.tsv", num_runs, m, window)
	
	runs = []
	for i in range(1,num_runs+1):
		dir = f"{sim_dir}/run_{i}"
		df = pd.read_csv(f"{dir}/main.log", sep=',',names=main_cols,header=None)
		steady = df.iloc[cutoff:]
		av_resp = steady['resp'].mean()
		testTime = steady['time'].iloc[-1] - steady['time'].iloc[0]
		goodput = (steady['good'].iloc[-1] - steady['good'].iloc[0]) / testTime
		badput = (steady['bad'].iloc[-1] - steady['bad'].iloc[0]) / testTime
		throughput = (steady['completed'].iloc[-1] - steady['completed'].iloc[0]) / testTime
		drops = 100 * (steady['dropped'].iloc[-1] - steady['dropped'].iloc[0]) / len(steady)
		
		runs.append([
			av_resp,
			goodput,
			badput,
			throughput,
			drops
		])
	runs = np.asarray(runs)
	metr = [0,0,0,0,0]
	for i in range(5):
		metr[i] = np.mean(runs[:,i])
	return {
		"resp" : metr[0],
		"gput" : metr[1],
		"bput" : metr[2],
		"tput" : metr[3],
		"drate": metr[4],
	}

def get_exp_stats(dirname, users, num_runs, window = 40, cutoff = None):
	resps = []
	gputs = []
	bputs = []
	tputs = []
	drates= []

	for u in users:
		stats = get_sim_stats(f"{dirname}/{u}_users/", num_runs, window, cutoff)
		resps.append(stats["resp"])
		gputs.append(stats["gput"])
		bputs.append(stats["bput"])
		tputs.append(stats["tput"])
		drates.append(stats["drate"])

	return {
		"resps" : resps,
		"gputs" : gputs,
		"bputs" : bputs,
		"tputs" : tputs,
		"drate" : drates
	}

def plot_stats(dirname, users, stats):
	plt.plot(users, stats["resps"])
	plt.title("Response Times vs Num Users")
	plt.ylabel("Response Times (s)")
	plt.xlabel("Num Users")
	plt.savefig(f"{dirname}/resp.png")
	plt.close()

	plt.plot(users, stats["gputs"])
	plt.title("Goodput vs Num Users")
	plt.ylabel("GoodPut (req/s)")
	plt.xlabel("Num Users")
	plt.savefig(f"{dirname}/gput.png")
	plt.close()

	plt.plot(users, stats["bputs"])
	plt.title("Badput vs Num Users")
	plt.ylabel("BadPut (req/s)")
	plt.xlabel("Num Users")
	plt.savefig(f"{dirname}/bput.png")
	plt.close()

	plt.plot(users, stats["tputs"])
	plt.title("Throughput vs Num Users")
	plt.ylabel("ThroughPut (req/s)")
	plt.xlabel("Num Users")
	plt.savefig(f"{dirname}/tput.png")
	plt.close()

	plt.plot(users, stats["drate"])
	plt.title("Drop Rate vs Num Users")
	plt.ylabel("Drop Rate (%)")
	plt.xlabel("Num Users")
	plt.savefig(f"{dirname}/drate.png")
	plt.close()



