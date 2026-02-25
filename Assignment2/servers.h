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
    Time timeoutMin = 0.05;
    Time timeoutVar = 0.05;
	vector<double> outProbs;

};

struct Event;
class Simulation;
class Request;

class Server {

	using Time = double;

	ServerConfig cfg;
	// queue<int> threadQueue;	
	vector<queue<int>> coreQueue;

	vector<Time> coreLastSwitchIn;

	Simulation& sim;

	int completed = 0;
    int good = 0;
    int bad = 0;
    int dropped = 0;

	Time totalActiveTime;
	int totalInServer = 0;
	Time lastEvent;
	Time sumTotalInServer;

	Server(ServerConfig _cfg, Simulation& _sim) : cfg(_cfg), sim(_sim)
	{}

	void processEvent(Event e);

	void EnterServer(Request& req);
	void CtxSwitchIn(Request& req);
	void CtxSwitchOut(Request& req);
	void FinishService(Request& req);

	void updateMetrics();   // Updates time averages indicators

};

