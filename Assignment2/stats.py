import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
from run import welch_cutoff
from run_welch import consolidate
import math
import os


main_cols = ['time','good','bad','dropped','completed','resp']
serv_cols = ['time','good','bad','dropped','resp','serv','active','num']

def get_sim_stats(sim_dir, num_runs, window = 40, cutoff = None):
	if (cutoff is None):
		m = consolidate(sim_dir, num_runs)
		cutoff = welch_cutoff(f"{sim_dir}/consolidated.tsv", num_runs, m, window)
	print(sim_dir)
	runs = []
	for i in range(1,num_runs+1):
		dir = f"{sim_dir}/run_{i}"
		df = pd.read_csv(f"{dir}/main.log", sep=',',names=main_cols,header=None)
		steady = df.iloc[cutoff:]
		av_resp = steady['resp'].mean()
		# print(len(steady))
		testTime = steady['time'].iloc[-1] - steady['time'].iloc[0]
		goodput = (steady['good'].iloc[-1] - steady['good'].iloc[0]) / testTime
		badput = (steady['bad'].iloc[-1] - steady['bad'].iloc[0]) / testTime
		comp = (steady['completed'].iloc[-1] - steady['completed'].iloc[0])
		drop = (steady['dropped'].iloc[-1] - steady['dropped'].iloc[0])
		throughput =comp / testTime
		drops =  100 * drop / (drop + comp)
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

	#95% confidence intervals
	resps = runs[:,0]
	# print(resps)
	std = np.std(resps) 
	ci = 1.96 * std / math.sqrt(len(resps))
	# print(ci)

	return {
		"resp" : metr[0],
		"ci"   : ci,
		"gput" : metr[1],
		"bput" : metr[2],
		"tput" : metr[3],
		"drate": metr[4],
	}

def get_serv_stats(dir, serv_id, num_runs, cutoff=100):
	runs = []
	for i in range(num_runs):
		file = f"{dir}/run_{i+1}/server_{serv_id}.log"
		df = pd.read_csv(file, sep=',',names=serv_cols,header=None)
		active = df['active'].iloc[-1] - df["active"].iloc[0]
		num_in = df['num'].iloc[-1] - df['num'].iloc[0]
		time = df['time'].iloc[-1] - df['time'].iloc[0]
		throughput = (df['good'].iloc[-1] - df['bad'].iloc[0]) / time
		throughput += (df['good'].iloc[-1] - df['good'].iloc[0]) / time
		runs.append([
			active / time,
			num_in / time,
			throughput
		])

	metr = [0,0,0]
	runs = np.asarray(runs)
	for i in range(3):
		metr[i] = np.mean(runs[:,i])
	return {
		"util" : metr[0],
		"num"  : metr[1],
		"tput" : metr[2],
	}



def get_exp_stats(dirname, users, num_runs, num_servers = 1, window = 40, cutoff = None):
	resps = []
	cis   = []
	gputs = []
	bputs = []
	tputs = []
	drates= []
	servers = []

	for u in users:
		stats = get_sim_stats(f"{dirname}/{u}_users/", num_runs, window, cutoff)
		serv_stats = []
		for i in range(num_servers):
			serv_stats.append(get_serv_stats(f"{dirname}/{u}_users",i+1,num_runs,cutoff))
		servers.append(serv_stats)
		resps.append(stats["resp"])
		cis.append(stats["ci"])
		gputs.append(stats["gput"])
		bputs.append(stats["bput"])
		tputs.append(stats["tput"])
		drates.append(stats["drate"])

	return {
		"resps" : resps,
		"cis"   : cis,
  		"gputs" : gputs,
		"bputs" : bputs,
		"tputs" : tputs,
		"drate" : drates,
		"servers": servers
	}

def gen_multiple_tput(dirlist, lbl_list, save_file, users, num_runs, window = 40, cutoff = None):
	stats_list = []
	for dir in dirlist:
		s = get_exp_stats(dir, users, num_runs, window, cutoff)
		stats_list.append(s)

	for i in range(len(stats_list)):
		plt.plot(users, stats_list[i]["tputs"], label = lbl_list[i])
	plt.title("Throughput vs Num Users")
	plt.ylabel(" Throughput (req/s)")
	plt.xlabel("Num Users")
	plt.legend()
	plt.savefig(save_file)
	plt.close()

def gen_multiple_resp(dirlist, lbl_list, save_file, users, num_runs, window = 40, cutoff = None):
	stats_list = []
	for dir in dirlist:
		s = get_exp_stats(dir, users, num_runs, window, cutoff)
		stats_list.append(s)

	for i in range(len(stats_list)):
		plt.plot(users, stats_list[i]["resps"], label = lbl_list[i])
	plt.title("Response Times vs Num Users")
	plt.ylabel("Response Times (req/s)")
	plt.xlabel("Num Users")
	plt.legend()
	plt.savefig(save_file)
	plt.close()
	
def plot_stats(dirname, users, stats):
	# plt.plot(users, stats["resps"])
	plt.errorbar(users, stats["resps"], yerr=stats["cis"],color="blue",ecolor="orange", lw=0.5)
	plt.title("Response Times vs Num Users")
	plt.ylabel("Response Times (s)")
	plt.xlabel("Num Users")
	plt.savefig(f"{dirname}/resp.png")
	plt.close()

	plt.plot(users, stats["gputs"], label = "GoodPut", color='green')
	plt.plot(users, stats["bputs"], label = "BadPut", color='red')
	plt.plot(users, stats["tputs"], label = "ThroughPut", linestyle = '--', color='orange')

	plt.title("Throughputs vs Num Users")
	plt.xlabel("Num Users")
	plt.ylabel("Throughputs (req/s)")
	plt.legend()
	plt.savefig(f"{dirname}/throughputs.png")
	plt.close()

	plt.plot(users, stats["drate"])
	plt.title("Drop Rate vs Num Users")
	plt.ylabel("Drop Rate (%)")
	plt.xlabel("Num Users")
	plt.savefig(f"{dirname}/drate.png")
	plt.close()

	servers = stats["servers"]
	# print(servers)
	if(len(servers) > 0):
		for i in range(len(servers[0])):
			os.system(f"mkdir -p {dirname}/server_{i+1}")
			utils = []
			nums  = []
			tputs = []
			for u_stat in servers:
				# print(u_stat)
				# print(i)
				utils.append(u_stat[i]["util"])
				nums.append (u_stat[i]["num"])
				tputs.append(u_stat[i]["tput"])
			plt.title(f"Server {i+1}, Utilization vs Users")
			plt.ylabel("Utilization")
			plt.xlabel("Users")
			plt.plot(users, utils)
			plt.savefig(f"{dirname}/server_{i+1}/util.png")
			plt.close()

			plt.title(f"Server {i+1}, Throughput vs Users")
			plt.ylabel("Throughput (req/s)")
			plt.xlabel("Users")
			# print(len(users))
			# print(len(utils))
			# print(len(nums))
			# print(len(tputs))

			plt.plot(users, tputs)
			plt.savefig(f"{dirname}/server_{i+1}/tput.png")
			plt.close()

			plt.title(f"Server {i+1}, Number of Requests in System vs Users")
			plt.ylabel("N")
			plt.xlabel("Users")
			plt.plot(users, nums)
			plt.savefig(f"{dirname}/server_{i+1}/nums.png")
			plt.close()

			plt.title(f"Server {i+1}, Throughput vs Utilization")
			plt.ylabel("Throughput")
			plt.xlabel("Utilization")
			plt.plot(utils, tputs)
			plt.savefig(f"{dirname}/server_{i+1}/tpus_util.png")
			plt.close()









