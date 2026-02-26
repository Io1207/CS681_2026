#include "sim.h"
#include "servers.h"

Server::Server(int id, ServerConfig _cfg, Simulation& _sim, unsigned seed) : 
	serverId(id),
	cfg(_cfg), 
	sim(_sim), 
	coreQueue(_cfg.numCores),
	coreLastSwitchIn(_cfg.numCores, -1),
	totalActiveTime(0),
	lastEvent(0),
	sumTotalInServer(0),
	rng(seed)
{}

void Server::processEvent(const Event& e)
{
	Request& req = sim.getRequest(e.reqId);
	switch(e.type)
	{
		case ENTER_SERVER: EnterServer(req); break;
		case CTX_SWITCH_IN:	CtxSwitchIn(req, e); break;
		case CTX_SWITCH_OUT: CtxSwitchOut(req, e); break;
		case FINISH_SERVICE: FinishService(req, e); break;
	}
	updateMetrics();
}

void Server::EnterServer(Request& req)
{
	// if(req.timedOut)	return;
	req.serverVisitCounts[serverId]++;
	req.serviceStartTimes[serverId] = sim.simTime();
	req.activeServer = serverId;
	req.remService = serviceTime();

	int maxPerCore = cfg.maxThreads / cfg.numCores;
	int idealCore = -1; 
	for(int i=0;i<cfg.numCores;i++)
	{
		if(coreQueue[i].size() < maxPerCore )
		{
			if(idealCore >= 0 && coreQueue[i].size() < coreQueue[idealCore].size())
			{
				idealCore = i;
			}
		}
	}
	if(idealCore == -1)
	{
		// All threads busy
		// Put in thread queue
		if(threadQueue.size() < cfg.maxQueue)
		{
			// Schedule SERVER_TIMEOUT 
			threadQueue.push(req.id);
		}
		else 
		{
			// Drop the request
			dropped++;
			//Schedule ENTER_CLIENT
		}
	}
	else 
	{
		// Assign to a thread 
		coreQueue[idealCore].push(req.id);

		// Possibly call switch in
		if(coreQueue[idealCore].size() == 1)
		{
			sim.schedule(/*add args*/);
		}
	}
}

void Server::CtxSwitchIn(Request& req, const Event& e)
{
	// if(req.timedOut) {
	// 	return;
	// }
	if(req.remService < cfg.roundRobinTimeout)
	{
		// Schedule Finish Service
	}	
	else 
	{
		// Schedule ctx switch out
	}
}

void Server::CtxSwitchOut(Request& req, const Event& e)
{
	int core = e.coreId;
	if(core < 0 || coreQueue[core].front() != req.id)
		throw "Mismatching Event and Core Queue";
	req.remService -= cfg.roundRobinTimeout;
	if(coreQueue[core].size() > 1) {
		coreQueue[core].pop();
		coreQueue[core].push(req.id);

		// Schedule CTX_SWITCH_IN after ctx_switch time
	}
	else 
	{
		// Schedule CTX_SWITCH_OUT after RRTime 
	}
}

void Server::FinishService(Request& req, const Event& e)
{
	int core = e.coreId;
	if(core < 0 || coreQueue[core].front() != req.id)
		throw "Mismatching Event and Core Queue";
	
	req.remService = 0;
	req.serviceResponseTimes[serverId] += e.time - req.serviceStartTimes[serverId];

	// Find next Server
	int nextServer = -1;
	
	double rnd = rng.uniform(0,1);
	for(int i=0;i<cfg.outProbs.size();i++)
	{
		if(rnd <= cfg.outProbs[i])
		{
			nextServer = i;
			break;
		}
		else 
			rnd -= cfg.outProbs[i];
	}

	if(nextServer == -1)
		throw "No Next Server";

	req.activeServer = nextServer;

	coreQueue[core].pop();
	// Schedule ENTER_SERVER

	if(threadQueue.size() > 0)
	{
		int newReq = threadQueue.front(); threadQueue.pop();
		coreQueue[core].push(newReq);
	}

	if(coreQueue[core].size() > 0)
	{
		// Schedule CTX_SWITCH_IN
	}
}

void Server::updateMetrics()
{
	totalActiveTime += numCoresActive * (sim.simTime() - lastEvent);
	sumTotalInServer += totalInServer * (sim.simTime() - lastEvent);

	numCoresActive = 0;
	totalInServer = 0;
	for(int i=0;i<cfg.numCores;i++)
	{
		if(coreQueue[i].size() > 0)
			numCoresActive++;
		totalInServer += coreQueue[i].size();
	}
	totalInServer += threadQueue.size();
}

