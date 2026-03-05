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
{
	file_name = "server_" + (char) id; // change
}

void Server::processEvent(const Event& e)
{
	Request& req = sim.getRequest(e.reqId);
	int calling_event;
	switch(e.type)
	{
		// case ENTER_SERVER: EnterServer(req); calling_event = 0; break;
		// case CTX_SWITCH_IN:	CtxSwitchIn(req, e); calling_event = 1; break;
		// case CTX_SWITCH_OUT: CtxSwitchOut(req, e); calling_event = 2; break;
		// case FINISH_SERVICE: FinishService(req, e); calling_event = 3; break;
		case ENTER_SERVER: EnterServer(req); break;
		case CTX_SWITCH_IN:	CtxSwitchIn(req, e); break;
		case CTX_SWITCH_OUT: CtxSwitchOut(req, e); break;
		case FINISH_SERVICE: FinishService(req, e); break;
	}
	// update metrics depends on which event calls it, right?
	updateMetrics();
}

void Server::EnterServer(Request& req)
{
	// if(req.timedOut)	return;
	numReqs++;
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

	// Schedule SERVER_TIMEOUT
	sim.schedule(sim.simTime() + cfg.timeout, 
				SERVER_TIMEOUT, 
				serverId, 
				req.id,
				-1);
	if(idealCore == -1)
	{
		// All threads busy
		// Put in thread queue
		if(threadQueue.size() < cfg.maxQueue)
		{
			
			threadQueue.push(req.id);
		}
		else 
		{
			// Drop the request
			bad++;
			//Schedule ENTER_CLIENT -- reqId of event is userId of req
			sim.schedule(sim.simTime(), 
						ENTER_CLIENT, 
						0, 
						req.user, 
						-1);
			
		}
	}
	else 
	{
		req.coreStartTimes[serverId] = sim.simTime();
		// Assign to a thread 
		coreQueue[idealCore].push(req.id);

		// Possibly call switch in
		if(coreQueue[idealCore].size() == 1)
		{
			// Schedule CTX_SWITCH_IN
			sim.schedule(sim.simTime(), 
						CTX_SWITCH_IN, 
						serverId, 
						req.id, 
						idealCore);
			// sim.schedule(/*add args*/);
		}
	}
}

void Server::CtxSwitchIn(Request& req, const Event& e)
{
	int core = e.coreId;
	if(coreQueue[core].front() != req.id)
		throw "Mismatching Event and Core Queue";
	if(req.serverTimedOut) {
		bad++;
		req.timedOut = true;
		sim.schedule(sim.simTime(), 
						ENTER_CLIENT, 
						0, 
						req.user, 
						-1);

		coreQueue[core].pop();
		assignThread(core);
		return;
	}
	if(req.remService < cfg.roundRobinTimeout)
	{
		// Schedule Finish Service
		sim.schedule(sim.simTime() + req.remService, 
					FINISH_SERVICE, 
					serverId, 
					req.id, 
					e.coreId);
	}	
	else 
	{
		// Schedule ctx switch out
		sim.schedule(sim.simTime() + cfg.roundRobinTimeout, 
					CTX_SWITCH_OUT, 
					serverId, 
					req.id, 
					e.coreId);
	}
}

void Server::CtxSwitchOut(Request& req, const Event& e)
{
	int core = e.coreId;
	if(core < 0 || coreQueue[core].front() != req.id)
		throw "Mismatching Event and Core Queue";
	if(req.serverTimedOut) {
		req.timedOut = true;
		bad++;
		sim.schedule(sim.simTime(), 
						ENTER_CLIENT, 
						0, 
						req.user, 
						-1);

		coreQueue[core].pop();
		assignThread(core);
		return;
	}
	req.remService -= cfg.roundRobinTimeout;
	if(coreQueue[core].size() > 1) {
		coreQueue[core].pop();
		coreQueue[core].push(req.id);

		// Schedule CTX_SWITCH_IN after ctx_switch time
		sim.schedule(sim.simTime() + cfg.contextSwitchTime, 
					CTX_SWITCH_IN, 
					serverId, 
					coreQueue[core].front(),
					e.coreId);
	}
	else 
	{
		// Schedule CTX_SWITCH_OUT after RRTime 
		sim.schedule(sim.simTime() + cfg.roundRobinTimeout, 
					CTX_SWITCH_OUT, 
					serverId, 
					req.id, 
					e.coreId);
	}
}

void Server::FinishService(Request& req, const Event& e)
{
	int core = e.coreId;
	if(core < 0 || coreQueue[core].front() != req.id)
		throw "Mismatching Event and Core Queue";
	if(req.serverTimedOut) {
		req.timedOut = true;
		bad++;

		// AS: should we not make the timeout time the response time for the request?
		// req.completion = sim.simTime();

		sim.schedule(sim.simTime(), 
						ENTER_CLIENT, 
						0, 
						req.user, 
						-1);

		coreQueue[core].pop();
		assignThread(core);
	}
	req.remService = 0;

	Time resp = e.time - req.serviceStartTimes[serverId];
	Time serv = e.time - req.serviceTimes[serverId];

	req.serviceResponseTimes[serverId] += resp;
	req.serviceTimes[serverId] += serv;

	responseTimes.push_back(resp);
	serviceTimes.push_back(serv);

	good++;

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
	if(nextServer == 0)
	{
		req.completed = true;
		req.completion = sim.simTime();
		// Schedule ENTER_CLIENT -- reqId of event is userId of req
		sim.schedule(sim.simTime(), 
					ENTER_CLIENT, 
					0, 
					req.user, 
					-1);
	}
	else 
	{
		// Schedule ENTER_SERVER
		sim.schedule(sim.simTime(), 
					ENTER_SERVER, 
					nextServer, 
					req.id, 
					-1);
	}


	assignThread(core);
	// if(threadQueue.size() > 0)
	// {
	// 	int newReq = threadQueue.front(); threadQueue.pop();
	// 	coreQueue[core].push(newReq);
	// }

	// if(coreQueue[core].size() > 0)
	// {
	// 	// Schedule CTX_SWITCH_IN
	// 	sim.schedule(sim.simTime() + cfg.contextSwitchTime, 
	// 				CTX_SWITCH_IN, 
	// 				serverId, 
	// 				coreQueue[core].front(),
	// 				core);

	// }
}

void Server::ServerTimeout(Request& req)
{
	if(req.activeServer == serverId)
		req.serverTimedOut = true;
}

void Server::updateMetrics(int calling_event = 0)
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

	ofstream f(file_name);
	f <<  getAvgServiceTime() << "," << (char)getAvgResponseTime() << "," << getGood() << "," << getBad() << "," << getVisits() << "," << endl;
	return;
}