#include "sim.h"
#include "servers.h"
struct Config;

/*
    g++ -std=c++20 sim.cpp -O2 -o sim
*/

Config::Config(string filename) {
    ifstream f(filename);
    f >> users;
    f >> ease_in >> timeout >> thinkMin >> thinkVar;
    f >> numServers;
    f >> numDelays;

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

        f >> s.numCores >> s.maxThreads >> s.maxQueue;
        
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

Simulation::Simulation(Config c, unsigned seed) : cfg(c), rng(seed), client(cfg.users, cfg.thinkMin, cfg.thinkVar, seed, cfg.adjMatrix[0]) {
    metrics.nodeVisitCounts.resize(cfg.numServers);
    metrics.serverDemands.resize(cfg.numServers);
    metrics.serverResponseTimes.resize(cfg.numServers);
    for(int i=0;i<cfg.users;i++) {
        schedule(cfg.ease_in*i/cfg.users, LEAVE_CLIENT, client.route(), i);
    }
}

void Simulation::schedule(Time t, EventTypes type, int sId, int reqId, int coreId = -1) {
    fel.push({t, type, sId, coreId, reqId});
}

void Simulation::scheduleThink(int user, Time base) {
    schedule(base+thinkTime(), LEAVE_CLIENT, client.route(), user);
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
    while (!fel.empty()) {
        Event e = fel.top();
        fel.pop();
        int lastReqId = requests.size();
        switch (e.type) {
            case LEAVE_CLIENT: servers[e.serverId].processEvent(Event(e.time, ENTER_SERVER, e.serverId, -1, lastReqId+1));
                               client.visitCount++;
                               requests.emplace_back(lastReqId+1, e.serverId, cfg.numServers); break;
            case ENTER_CLIENT: scheduleThink(e.reqId, e.time); break;
            case CLIENT_TIMEOUT: scheduleThink(e.reqId, e.time); break;
        }
    }
}

SimMetrics Simulation::results() {
    double effTime = simTime() - cfg.warmup;
    SimMetrics m{};
    for (int i = 0; i<cfg.numServers; i++) {
        m.nodeVisitCounts[i] = servers[i].getVisits();
        m.serverDemands[i] = servers[i].getAvgServiceTime();
        m.serverDemands[i] = servers[i].getAvgResponseTime();
    }
}

pair<double,double> meanCI(vector<double> &v) {
    int n=v.size();
    double m=accumulate(v.begin(),v.end(),0.0)/n;
    double s=0;
    for(double x:v) s+=(x-m)*(x-m);
    s=sqrt(s/(n-1));
    double z=1.96;
    double h=z*s/sqrt(n);
    return {m,h};
}

int main() {
    string filename = "config.txt";
    Config cfg = Config(filename);
    vector<double> resp;
    vector<double> thr;
    vector<double> good;
    vector<double> bad;
    vector<double> drop;
    vector<double> util;

    for(int r=0;r<cfg.runs;r++) {
        Simulation sim(cfg, r+1);
        sim.run();
        auto m = sim.results();
        resp.push_back(m.avgResp);
        thr.push_back(m.throughput);
        good.push_back(m.goodput);
        bad.push_back(m.badput);
        drop.push_back(m.dropRate);
        util.push_back(m.util);
    }

    auto [meanR,ciR] = meanCI(resp);
    cout<<"Avg Response Time: "<<meanR<<" +/- "<<ciR<<"\n";
    cout<<"Throughput: "<<accumulate(thr.begin(),thr.end(),0.0)/thr.size()<<"\n";
    cout<<"Goodput: "<<accumulate(good.begin(),good.end(),0.0)/good.size()<<"\n";
    cout<<"Badput: "<<accumulate(bad.begin(),bad.end(),0.0)/bad.size()<<"\n";
    cout<<"Drop Rate: "<<accumulate(drop.begin(),drop.end(),0.0)/drop.size()<<"\n";
    cout<<"Utilization: "<<accumulate(util.begin(),util.end(),0.0)/util.size()<<"\n";
}

void printLog(int runId, string filename){
    ifstream f(filename);
    
}