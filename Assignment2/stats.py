import numpy as np
import pandas as pd
from run import welch_cutoff
from run_welch import consolidate

main_cols = ['time','good','bad','dropped','completed','resp']

def get_sim_stats(sim_dir, num_runs, window = 40, cutoff = None):
	if (cutoff is None):
		m = consolidate(sim_dir, num_runs)
		cutoff = welch_cutoff(f"{sim_dir}/consolidated.tsv", num_runs, m, 40)
	
	runs = []
	for i in range(1,num_runs+1):
		dir = f"{sim_dir}/run_{i}"
		df = pd.read_csv(f"{dir}/main.log", sep=',',names=main_cols,header=None)
		steady = df.iloc[cutoff:]
		av_resp = steady['resp'].mean()
		testTime = steady['time'].iloc(-1) - steady['time'].iloc(0)
		goodput = (steady['good'].iloc(-1) - steady['good'].iloc(0)) / testTime
		badput = (steady['bad'].iloc(-1) - steady['bad'].iloc(0)) / testTime
		throughput = (steady['completed'].iloc(-1) - steady['completed'].iloc(0)) / testTime
		runs.append({
			"resp" : av_resp,
			"gput" : goodput,
			"bput": badput,
			"tput": throughput
		})
	# To Be Completed



