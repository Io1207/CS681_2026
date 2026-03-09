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
	rng(seed) {
	log_file.open("server_" + to_string(id));
	threadInfo.resize(cfg.threadBuffer);
	transform(coreQC.begin(), coreQC.end(), coreQC.begin(), [](char c, int mQ = cfg.maxQueue, int cores = cfg.numCores){return (char) (int) (mQ / cores);});
	for (int i = 0; i < cfg.maxQueue%cfg.numCores; i++) {
		coreQC[i] = (char) ((int) coreQC[i] + 1);
	}
}

void Server::processEvent(const Event& e) {
	Request& req = sim.getRequest(e.reqId);
	int calling_event = -1;
	switch(e.type)
	{
		case ENTER_SERVER: EnterServer(req); updateMetrics(); break;
		case FINISH_SERVICE: FinishService(e); updateMetrics(); break;

		case CTX_SWITCH_IN:	CtxSwitchIn(e); updateMetrics(); break;
		case CTX_SWITCH_OUT: CtxSwitchOut(e); updateMetrics(); break;

		// case SERVER_TIMEOUT: ServerTimeout(req); updateMetrics(); break;
		// SERVER_TIMEOUT is handled in timeout function, no sparate case for it
	}
	// update metrics depends on which event calls it, right? if we want to trace the path throughout the system,
	// then we need reqId and event type to know which metric to update. For example, if it's a finish service event, then we know the request is leaving the server, so we can update the active time and in server time accordingly. If it's a ctx switch in or out, then we know the request is still in the server, so we can update the active time and in server time accordingly. If it's an enter server event, then we know the request is entering the server, so we can update the active time and in server time accordingly.
	// updateMetrics(e);
	updateMetrics(calling_event);
}

void Server::EnterServer(Request& req) {
	// if(req.timedOut)	return;
	numReqs++;
	req.serviceStartTimes[serverId] = sim.simTime();
	req.activeServer = serverId;
	req.remService = serviceTime();
	req.serciveTime = req.remService;

	int maxPerCore = cfg.maxThreads / cfg.numCores;
	int idealCore = -1; 

	/*
		the hell is happening here?
	*/
	for(int i=0;i<cfg.numCores;i++)
	{
		if(coreQueue[i].size() < maxPerCore )
		{
			// maybe initialise ideal core to 0? when would we even enter this condition
			if(idealCore >= 0 && coreQueue[i].size() < coreQueue[idealCore].size())
			{
				idealCore = i;
			}
		}
	}

	// Schedule SERVER_TIMEOUT
	/*
		AS:
		I think this is a bad way to implement server timeouts, it would be better 
		to handle this in the same way as client timeouts: if the amt of time a core has
		been kept busy by this request exceeds the timeout, then mark the request as timed 
		out, call finish service, ctx switch out, schedule client enter event
	*/
	// sim.schedule(sim.simTime() + cfg.timeout, 
	// 			SERVER_TIMEOUT, 
	// 			serverId, 
	// 			req.id,
	// 			-1);

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
			req.setReturnStatus(DROP);
			// bad++; // according to problem statement, badput is rate of completion of timed out reqs 
				   // so it makes sense to increment only in client timeout case
			drops++;

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
		// req.coreStartTimes[serverId] = sim.simTime();
		// Assign to a thread 
		coreQueue[idealCore].push(req.id);

		// Possibly call switch in
		if(coreQueue[idealCore].size() == 1)
		{
			// Schedule CTX_SWITCH_IN
			// sim.schedule(sim.simTime(), 
			// 			CTX_SWITCH_IN, 
			// 			serverId, 
			// 			req.id, 
			// 			idealCore);

			processEvent(Event(sim.simTime(), CTX_SWITCH_IN, serverId, req.id, idealCore));
			// sim.schedule(/*add args*/);
		}
	}
}

void Server::CtxSwitchIn(const Event& e) {
	int core = e.coreId;
	Request& req = sim.getRequest(e.reqId);
	if(coreQueue[core].front() != req.id)
		throw "Mismatching Event and Core Queue";

	// is this not a redundant check?
	// if(req.serverTimedOut) {
	// 	bad++;
	// 	req.timedOut = true;
	// 	sim.schedule(sim.simTime(), 
	// 					ENTER_CLIENT, 
	// 					0, 
	// 					req.user, 
	// 					-1);

	// 	coreQueue[core].pop();
	// 	assignThread(core);
	// 	return;
	// }

	timeout(e);
	if (req.ret == S_TIMEOUT) return;
	if(req.remService < cfg.roundRobinTimeout) // request will complete in the current RR slice
	{
		// Schedule Finish Service
		// sim.schedule(sim.simTime() + req.remService, 
		// 			FINISH_SERVICE, 
		// 			serverId, 
		// 			req.id, 
		// 			e.coreId);

		processEvent(Event(sim.simTime(), FINISH_SERVICE, serverId, req.id, e.coreId));
	}	
	else // round robin timeout b4 service completes
	{
		// Schedule ctx switch out
		// if (sim.simTime() - req.serviceStartTimes[serverId] > cfg.timeout) {
		// 	req.timedOut = true;
		// 	bad++;
		// 	req.setReturnStatus(S_TIMEOUT);
		// 	processEvent(Event(sim.simTime(), FINISH_SERVICE, serverId, req.id, e.coreId));
		// 	return;
		// }
		// sim.schedule(sim.simTime() + cfg.roundRobinTimeout, 
		// 			CTX_SWITCH_OUT, 
		// 			serverId, 
		// 			req.id, 
		// 			e.coreId);
		
		processEvent(Event(sim.simTime() + cfg.roundRobinTimeout, CTX_SWITCH_OUT, serverId, req.id, e.coreId));
	}
}

void Server::CtxSwitchOut(const Event& e) {
	Request& req = sim.getRequest(e.reqId);
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
		// sim.schedule(sim.simTime() + cfg.contextSwitchTime, 
		// 			CTX_SWITCH_IN, 
		// 			serverId, 
		// 			coreQueue[core].front(),
		// 			e.coreId);

		processEvent(Event(sim.simTime() + cfg.contextSwitchTime, CTX_SWITCH_IN, serverId, coreQueue[core].front(), e.coreId));
	}
	else 
	{
		// Schedule CTX_SWITCH_OUT after RRTime 
		// sim.schedule(sim.simTime() + cfg.roundRobinTimeout, 
		// 			CTX_SWITCH_OUT, 
		// 			serverId, 
		// 			req.id, 
		// 			e.coreId);
		// processEvent(Event(sim.simTime() + cfg.roundRobinTimeout, CTX_SWITCH_OUT, serverId, req.id, e.coreId));
		if (sim.getRequest(e.reqId).remService > 0) {
			processEvent()
		}
	}
}

void Server::FinishService(const Event& e)
{
	Request& req = sim.getRequest(e.reqId);
	int core = e.coreId;
	if(core < 0 || coreQueue[core].front() != req.id)
		throw "Mismatching Event and Core Queue";
	if(req.ret == S_TIMEOUT) {
		assert(req.timedOut);
		drops++;
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
	Time serv = e.time - req.serviceTimes[serverId]; // ???
	// Time serv = req.serciveTime;

	req.serviceResponseTimes[serverId] += resp;
	req.serviceTimes[serverId] += serv;

	if (req.ret == C_TIMEOUT) bad++;
	else good++;
	responseTimes.push_back(resp);
	serviceTimes.push_back(serv);

	// Find next Server

	req.activeServer = routeNext();

	coreQueue[core].pop();
	if(req.activeServer == 0)
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
					req.activeServer, 
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


void Server::timeout(const Event& e) {
	Request& req = sim.getRequest(e.reqId);

	if ((req.remService>cfg.roundRobinTimeout ? cfg.roundRobinTimeout : req.remService) + sim.simTime() - req.serviceStartTimes[serverId] > req.c_timeout) {
		req.timedOut = true;
		req.setReturnStatus(C_TIMEOUT);
		// bad++;
		updateMetrics(e);
		sim.schedule(
			req.serviceStartTimes[serverId] + req.c_timeout,
			CLIENT_TIMEOUT,
			serverId,
			req.id);
	}

	else if (sim.simTime() - req.serviceStartTimes[serverId] > cfg.timeout){
		req.timedOut = true;
		req.serviceTimes[serverId-1] += sim.simTime() - req.serviceStartTimes[serverId];
		req.setReturnStatus(S_TIMEOUT);
		updateMetrics(e);
		processEvent(Event(sim.simTime(), FINISH_SERVICE, serverId, req.id, e.coreId));
	}
	return;
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

	log_file <<  getAvgServiceTime() << "," << (char)getAvgResponseTime() << "," << getGood() << "," << getBad() << "," << getVisits() << "," << endl;
	return;
}

void Server::updateMetrics(const Event& e)
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

	log_file << e.reqId << "," << getAvgServiceTime() << "," << (char)getAvgResponseTime() << "," << getGood() << "," << getBad() << "," << getVisits() << "," << endl;
	return;
}