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

	vector<Time> responseTimes;
	vector<Time> serviceTimes;

	Simulation& sim;

    int good = 0;
    int bad = 0;

	int numReqs = 0; 

	int totalInServer = 0;
	int numCoresActive = 0;
	Time lastEvent;
	Time totalActiveTime;
	Time sumTotalInServer;

	// string file_name; // change
	ofstream log_file;

public:

	Server(int id, ServerConfig _cfg, Simulation& _sim, unsigned seed);
	~Server() {
		log_file.close();
	}

	double serviceTime() {
		if(cfg.serviceDist == ServerConfig::CONST)
			return cfg.serviceMin;
		if(cfg.serviceDist == ServerConfig::UNIFORM)
			return rng.uniform(cfg.serviceMin, cfg.serviceMin+cfg.serviceVar);
		return cfg.serviceMin + rng.exponential(1.0/cfg.serviceVar);
	}

	void processEvent(const Event& e);

	int getCompleted() const { return good+bad;}
	int getGood() const { return good;}
	int getBad() const { return bad;}

	int getVisits() const { return numReqs;}

	double getAvgResponseTime() const {
		return accumulate(responseTimes.begin(), responseTimes.end(), 0.0) / (responseTimes.size() + 1e-9);
	}

	double getAvgServiceTime() const {
		return accumulate(serviceTimes.begin(), serviceTimes.end(), 0.0) / (serviceTimes.size() + 1e-9);
	}

	double getUtilisation() const {return totalActiveTime / (cfg.numCores * sim.simTime());}
	double getAvgInServer() const {return sumTotalInServer / (sim.simTime());}

private:
	void EnterServer(Request& req);
	void CtxSwitchIn(Request& req, const Event& e);
	void CtxSwitchOut(Request& req, const Event& e);
	void FinishService(Request& req, const Event& e);

	// void ServerTimeout(Request& req, int core);

	void updateMetrics(int calling_event);   // Updates time averages indicators
	void updateMetrics(const Event& e);   // fix (<-noun, not a verb) option 2

	void assignThread(int core)
	{
		if(threadQueue.size() > 0)
		{
			int newReq = threadQueue.front(); threadQueue.pop();
			Request& req = sim.getRequest(newReq);
			// req.coreStartTimes[serverId] = sim.simTime();
			coreQueue[core].push(newReq);
		}

		if(coreQueue[core].size() > 0)
		{
			// Schedule CTX_SWITCH_IN
			sim.schedule(sim.simTime() + cfg.contextSwitchTime, 
						CTX_SWITCH_IN, 
						serverId, 
						coreQueue[core].front(),
						core);

		}
	}

	void s_timeout(Request& req, const Event& e);
};