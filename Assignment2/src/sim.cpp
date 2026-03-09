#include "sim.h"
#include "servers.h"
#include <map>

struct Config;

/*
    g++ -std=c++20 sim.cpp -O2 -o sim
*/

map<double, double> confidenceIntervals = {
    {0.90, 1.645},
    {0.95, 1.96},
    {0.99, 2.576},
    {0.80, 1.282},
    {0.85, 1.44}
};

// int Request::ctr = 0;

Time Simulation::sampleClientTimeout()
{
    return cfg.timeoutMin + rng.lognormal(0.0, cfg.timeoutVar);
}

Config::Config(string filename) {
    ifstream f(filename);
    if (!f.is_open()) {
        throw runtime_error("Could not open config file");
    }
    f >> numDelays;
    f >> users;
    f >> ease_in >> timeoutMin >> timeoutVar >> thinkMin >> thinkVar;
    f >> numServers;

    adjMatrix.resize(numServers+1);
    for(int i=0;i<numServers;i++) {
        adjMatrix[i].resize(numServers);
        for(int j=0;j<numServers;j++) {
            f >> adjMatrix[i][j];
        }
    }

    scfgs.resize(numServers);
    for(int i=1; i<=numServers; i++) {
        ServerConfig &s = scfgs[i];

        f >> s.numCores >> s.maxThreads >> s.threadBuffer >> s.maxQueue;
        
        int dist;
        f >> dist >> s.serviceMin >> s.serviceVar;
        if (dist == 0) s.serviceDist = ServerConfig::CONST;
        else if (dist == 1) s.serviceDist = ServerConfig::UNIFORM;
        else s.serviceDist = ServerConfig::EXP;

        f >> s.contextSwitchTime >> s.roundRobinTimeout;
        f >> s.timeout;

        s.outProbs = adjMatrix[i];
    }
}

ClientNode::ClientNode(int users, Time thinkMin, Time thinkVar, unsigned seed, vector<double> outProbs):
                 users(users), thinkMin(thinkMin), thinkVar(thinkVar), rng(seed) {
    double cumProbs = 0;
    for (int i = 0; i < outProbs.size(); i++) {
        if (outProbs[i] > 0) {
            cumProbs += outProbs[i];
            this->outProbs.push_back({i, cumProbs});
        }
    }
}

int ClientNode::route() {
    double a = rng.uniform(0,1);
    for (auto &p : outProbs) {
        if (a < p.second) return p.first;
    }
    return outProbs.back().first;
}

Time ClientNode::thinkTime() {
    return thinkMin + rng.lognormal(0.0, thinkVar);
}

Simulation::Simulation(Config c, int seed) : cfg(c), rng(seed), 
                                   client(cfg.users, cfg.thinkMin, cfg.thinkVar, cfg.run_id, cfg.adjMatrix[0])
{
    metrics.nodeVisitCounts.resize(cfg.numServers);
    metrics.serverDemands.resize(cfg.numServers);
    metrics.serverResponseTimes.resize(cfg.numServers);

    log_file.open("main_" + to_string(cfg.run_id));
    now = 0.0;
    Time lastScheduled = 0.0;
    for(int i=0;i<cfg.numServers;i++)
    {
        // Server serv;
        servers.emplace_back(i,cfg.scfgs[i],*this,rng.randInt(0,1000000000));
    }

    for(int i=0;i<cfg.users;i++) {
        lastScheduled += rng.exponential(double(cfg.users)/cfg.ease_in);
        requests.emplace_back(i+1, i, cfg.numServers, sampleClientTimeout());
        schedule(lastScheduled, LEAVE_CLIENT, client.route(), i);
    }
}

void Simulation::schedule(Time t, EventTypes type, int sId, int reqId, int coreId) {
    fel.push({t, type, sId, coreId, reqId});
}

void Simulation::scheduleThink(int user, Time base) {
    requests.emplace_back(Request::ctr, user, cfg.numServers, sampleClientTimeout());
    schedule(base+thinkTime(), LEAVE_CLIENT, client.route(), user);
    // printLog(); // ?
}

Request& Simulation::getRequest(int reqId) {
    return requests[reqId-1];
}

Time Simulation::simTime() {
    return now;
}

Time Simulation::thinkTime() {
    return cfg.thinkMin + rng.lognormal(0.0,cfg.thinkVar);    
}

void Simulation::run() {
    Event e;
    while (!fel.empty()) {
        if (client.visitCount >= cfg.numDelays) {
            fel = {};
            break;
        }
        e = fel.top();
        fel.pop();
        now = e.time;
        int lastReqId = requests.size();
        switch (e.type) {
            case LEAVE_CLIENT: 
                servers[e.serverId].processEvent(Event(e.time, ENTER_SERVER, e.serverId, lastReqId+1));
                schedule(e.time, ENTER_SERVER, e.serverId, lastReqId+1);
                client.visitCount++;
                requests.emplace_back(lastReqId+1, e.serverId, cfg.numServers, sampleClientTimeout()); 
                requests.back().start = simTime();
                break;
            case ENTER_CLIENT: 
                handleEnterClient(e, getRequest(e.reqId));
                break;
            case ENTER_SERVER: 
                servers[e.serverId].processEvent(e); 
                break;
            case CLIENT_TIMEOUT: 
                clientTimeout(getRequest(e.reqId)); 
                scheduleThink(e.reqId, e.time); 
                break;
            default: 
                servers[e.serverId].processEvent(e); 
                break;
            // case SERVER_TIMEOUT: servers[e.serverId].processEvent(e); break;
        }
    }
}

void Simulation::handleEnterClient(Event& e, Request& req)
{
    req.completion = simTime();
    scheduleThink(req.id, e.time); 
    switch(req.ret)
    {
        case C_TIMEOUT:
            bad++;
            break;
        case GOOD:
            good++;
            break;
        case S_TIMEOUT:
            dropped++;
            break;
        case DROP:
            dropped++;
            break;
    }
    // log
}

void Simulation::clientTimeout(Request& req) {
    // This is called when a request times out at the client node. We need to schedule the 
    // next think time for this user.
    req.timedOut = true;
    req.setReturnStatus(C_TIMEOUT);
    // bad++;
    // scheduleThink(req.user, simTime()+client.timeoutOverhead);
    schedule(simTime(), ENTER_CLIENT, -1, req.id);
}

SimMetrics Simulation::results() {
    // double effTime = simTime() - cfg.warmup;
    SimMetrics m{};
    int totalCompleted = 0;
    for (int i = 0; i<cfg.numServers; i++) {
        totalCompleted += servers[i].getCompleted();
        m.nodeVisitCounts[i] = servers[i].getVisits();
        m.serverDemands[i] = servers[i].getAvgServiceTime();
        m.serverDemands[i] = servers[i].getAvgResponseTime();
    }
    int totalVisits = accumulate(m.nodeVisitCounts.begin(), m.nodeVisitCounts.end(), 0);
    
    transform(m.nodeVisitCounts.begin(), m.nodeVisitCounts.end(), m.nodeVisitCounts.begin(), [totalVisits] (int x) {return x/totalVisits;});

    for (int i = 0; i<cfg.numServers; i++) {
        m.serverResponseTimes[i] = m.serverResponseTimes[i] / m.nodeVisitCounts[i];

    }

    return SimMetrics();
}

pair<double,double> meanCI(vector<double> &v, double confidence = 0.95) {
    int n=v.size();
    double m=accumulate(v.begin(),v.end(),0.0)/n;
    double s=0;
    for(double x:v) s+=(x-m)*(x-m);
    s=sqrt(s/(n-1));
    double h=confidenceIntervals[confidence]*s/sqrt(n);
    return {m,h};
}

// SimMetrics getTheoreticalResults(Config sim_config, vector<ServerConfig> sfgs) {
//     SimMetrics t_results = SimMetrics();
    
//     // getting theoretical relative visits

//     vector<double> visits(sim_config.numServers + 1);
//     visits[0] = 1;
//     vector<double> curr_out(sim_config.numServers + 1);
//     for (int i = 0; i <= sim_config.numServers; i++) {
        
//     }
// }

int main(int argc, char* argv[]) {
    Config cfg = Config("config.txt");
    vector<double> resp;
    vector<double> thr;
    vector<double> good;
    vector<double> bad;
    vector<double> drop;
    vector<double> util;

    int seed;
    if(argc <= 1)
        seed = 0;
    else 
        seed = stoi(argv[1]);

    for(int r=0;r<cfg.runs;r++) {
        cfg.run_id = r + 1;
        Simulation sim(cfg, seed);
        sim.run();
        auto m = sim.results();
        // resp.push_back(m.avgResp);
        thr.push_back(m.throughput);
        good.push_back(m.goodput);
        bad.push_back(m.badput);
        drop.push_back(m.dropRate);
        // util.push_back(m.util);
    }

    auto [meanR,ciR] = meanCI(resp);
    cout<<"Avg Response Time: "<<meanR<<" +/- "<<ciR<<"\n";
    cout<<"Throughput: "<<accumulate(thr.begin(),thr.end(),0.0)/thr.size()<<"\n";
    cout<<"Goodput: "<<accumulate(good.begin(),good.end(),0.0)/good.size()<<"\n";
    cout<<"Badput: "<<accumulate(bad.begin(),bad.end(),0.0)/bad.size()<<"\n";
    cout<<"Drop Rate: "<<accumulate(drop.begin(),drop.end(),0.0)/drop.size()<<"\n";
    cout<<"Utilization: "<<accumulate(util.begin(),util.end(),0.0)/util.size()<<"\n";
}

// void Simulation::printLog(Time time) {
//     // this is essentially going to be stats for the client node
//     // no server stats here
//     // one thing I don't get is how are we planning to do request wise stats? Do we even want to do that?

//     log_file << simTime() - time << "\n";
// }