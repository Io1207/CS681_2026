#pragma once 

#include <iostream>
#include <vector>
#include <queue>
#include <random>
#include <cmath>
#include <numeric>
#include <algorithm>

using namespace std;


struct ServerConfig {
	using Time = double;

	unsigned int numCores;
	unsigned int maxThreads;
	unsigned int maxQueue;
    enum Dist { CONST, UNIFORM, EXP } serviceDist = EXP;


	Time contextSwitchTime;
	Time roundRobinTimeout;
	Time serviceMin = 0.005;
    Time serviceVar = 0.01;
    Time timeout;
	vector<double> outProbs;

};

struct Event;
class Simulation;
class Request;

class Server {
	using Time = double;
	
	ServerConfig cfg;
	RNG rng;

	int serverId;
	
	queue<int> threadQueue;	
	vector<queue<int>> coreQueue;

	vector<Time> coreLastSwitchIn;

	Simulation& sim;

	int completed = 0;
    int good = 0;
    int bad = 0;
    int dropped = 0;

	int totalInServer = 0;
	int numCoresActive = 0;
	Time lastEvent;
	Time totalActiveTime;
	Time sumTotalInServer;

	Server(int id, ServerConfig _cfg, Simulation& _sim, unsigned seed);

	double serviceTime() {
		if(cfg.serviceDist == Config::CONST)
			return cfg.serviceMin;
		if(cfg.serviceDist == Config::UNIFORM)
			return rng.uniform(cfg.serviceMin, cfg.serviceMin+cfg.serviceVar);
		return cfg.serviceMin + rng.exponential(1.0/cfg.serviceVar);
	}


	void processEvent(const Event& e);

	void EnterServer(Request& req);
	void CtxSwitchIn(Request& req, const Event& e);
	void CtxSwitchOut(Request& req, const Event& e);
	void FinishService(Request& req, const Event& e);

	void updateMetrics();   // Updates time averages indicators

};

