#pragma once

#include "common.h"

using namespace std;

struct ServerConfig
{
	unsigned int numCores;
	unsigned int maxThreads;
	unsigned int maxQueue;
	unsigned int threadBuffer;
	enum Dist
	{
		CONST,
		UNIFORM,
		EXP
	} serviceDist = EXP;

	Time contextSwitchTime = 0.000005;
	Time roundRobinTimeout;
	Time serviceMin = 0.005;
	Time serviceVar = 0.01;
	Time timeout;
	vector<double> outProbs;
};

struct Thread
{
	int reqId;
	int coreId = -1;
};

class Server
{
	ServerConfig cfg;
	RNG rng;
	int serverId;

	queue<int> threadQueue;
	vector<queue<int>> coreQueue;

	vector<Time> coreLastSwitchIn;

	vector<Time> responseTimes;
	vector<Time> serviceTimes;

	Simulation* sim;

	int good = 0;
	int bad = 0;
	int drops = 0;
	int numReqs = 0;

	int totalInServer = 0;
	int numCoresActive = 0;
	Time lastEvent = 0.0;
	Time totalActiveTime;
	Time sumTotalInServer;

	ofstream log_file;
	vector<Thread> threadInfo;
	vector<int> coreQC;

public:
	Server(int id, ServerConfig _cfg, Simulation *_sim, unsigned seed, string dirname);

	Server(const Server &) = delete;
	Server(Server &&) = default;
	Server &operator=(const Server &) = delete;
	Server &operator=(Server &&) = default;

	~Server()
	{
		log_file.close();
	}

	double serviceTime()
	{
		if (cfg.serviceDist == ServerConfig::CONST)
			return cfg.serviceMin;
		if (cfg.serviceDist == ServerConfig::UNIFORM)
			return rng.uniform(cfg.serviceMin, cfg.serviceMin + cfg.serviceVar);
		return cfg.serviceMin + rng.exponential(1.0 / cfg.serviceVar);
	}

	void processEvent(const Event &e);

	int getCompleted() const { return good + bad; }
	int getGood() const { return good; }
	int getBad() const { return bad; }
	int getVisits() const { return numReqs; }

	double getAvgResponseTime() const
	{
		return accumulate(responseTimes.begin(), responseTimes.end(), 0.0) / (responseTimes.size() + 1e-9);
	}

	double getAvgServiceTime() const
	{
		return accumulate(serviceTimes.begin(), serviceTimes.end(), 0.0) / (serviceTimes.size() + 1e-9);
	}

	double getUtilisation() const;
	double getAvgInServer() const;

private:
	void EnterServer(Request &req);
	void CtxSwitchIn(const Event &e);
	void CtxSwitchOut(const Event &e);
	void FinishService(const Event &e);
	void ServerTimeout(const Event &e);

	int routeNext()
	{
		int nextServer = -1;
		double rnd = rng.uniform(0, 1);
		for (int i = 0; i < cfg.outProbs.size(); i++)
		{
			if (rnd <= cfg.outProbs[i])
			{
				nextServer = i;
				break;
			}
			else
				rnd -= cfg.outProbs[i];
		}

		if (nextServer == -1)
			throw "No Next Server";
		return nextServer;
	}

	void updateMetrics(int calling_event); // Updates time averages indicators
	void updateMetrics(const Event &e);	   // fix (<-noun, not a verb) option 2

	void assignThread(int core);

	void timeout(const Event &e);

	void appendLog(Time resp_time, Time serv_time);
	
};