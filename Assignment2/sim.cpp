#include "sim.h"
/*
    g++ -std=c++20 sim.cpp -O2 -o sim
*/
using Time = double;

struct Config {
    int cores = 4;
    int maxThreads = 16;
    int users = 50;
    Time contextSwitch = 0.0005;
    Time warmup = 5.0;
    Time simTime = 60.0;
    int runs = 10;
    enum Dist { CONST, UNIFORM, EXP } serviceDist = EXP;
    Time serviceMin = 0.005;
    Time serviceVar = 0.01;
    Time timeoutMin = 0.05;
    Time timeoutVar = 0.05;
};

struct RNG {
    mt19937 gen;
    RNG(unsigned s) : gen(s) {}
    double uniform(double a, double b) {
        uniform_real_distribution<> d(a,b);
        return d(gen);
    }
    double exponential(double lambda) {
        exponential_distribution<> d(lambda);
        return d(gen);
    }
    double lognormal(double m, double s) {
        lognormal_distribution<> d(m,s);
        return d(gen);
    }
};

struct Request {
    int id;
    int user;
    Time arrival;
    Time start = -1;
    Time completion = -1;
    bool timedOut = false;
    bool completed = false;
};

struct Event {
    Time time;
    int type;
    int reqId;
    bool operator>(Event const& o) const { return time > o.time; }
};

class Simulation {
    Config cfg;
    RNG rng;
    Time now = 0.0;

    priority_queue<Event, vector<Event>, greater<Event>> fel;

    vector<Request> requests;
    queue<int> threadQueue;
    int activeThreads = 0;
    int nextCore = 0;

    vector<Time> coreBusy;
    vector<Time> coreLastStart;

    int nextReqId = 0;

    int completed = 0;
    int good = 0;
    int bad = 0;
    int dropped = 0;

    double respSum = 0.0;
    int respCount = 0;

public:
    Simulation(Config c, unsigned seed) : cfg(c), rng(seed) {
        coreBusy.assign(cfg.cores, 0.0);
        coreLastStart.assign(cfg.cores, 0.0);
        for(int i=0;i<cfg.users;i++) scheduleThink(i,0.0);
    }

    double serviceTime() {
        if(cfg.serviceDist == Config::CONST)
            return cfg.serviceMin;
        if(cfg.serviceDist == Config::UNIFORM)
            return rng.uniform(cfg.serviceMin, cfg.serviceMin+cfg.serviceVar);
        return cfg.serviceMin + rng.exponential(1.0/cfg.serviceVar);
    }

    double timeoutTime() {
        return cfg.timeoutMin + rng.uniform(0.0, cfg.timeoutVar);
    }

    double thinkTime() {
        return rng.lognormal(0.0,0.5);
    }

    void schedule(Time t, int type, int reqId) {
        fel.push({t,type,reqId});
    }

    void scheduleThink(int user, Time base) {
        Time t = base + thinkTime();
        schedule(t,0,user);
    }

    void startRequest(int user) {
        Request r;
        r.id = nextReqId++;
        r.user = user;
        r.arrival = now;
        requests.push_back(r);
        int id = r.id;
        schedule(now + timeoutTime(),3,id);
        if(activeThreads < cfg.maxThreads) {
            activeThreads++;
            schedule(now,1,id);
        } else {
            threadQueue.push(id);
        }
    }

    void beginService(int id) {
        Request &r = requests[id];
        if(r.timedOut) return;
        r.start = now;
        int core = nextCore;
        nextCore = (nextCore+1)%cfg.cores;
        coreLastStart[core] = now;
        Time s = serviceTime() + cfg.contextSwitch;
        schedule(now+s,2,id);
    }

    void finishService(int id) {
        Request &r = requests[id];
        r.completion = now;
        r.completed = true;
        activeThreads--;
        completed++;
        if(r.timedOut) bad++;
        else good++;
        if(now > cfg.warmup && !r.timedOut) {
            respSum += r.completion - r.arrival;
            respCount++;
        }
        scheduleThink(r.user,now);
        if(!threadQueue.empty()) {
            int nxt = threadQueue.front();
            threadQueue.pop();
            activeThreads++;
            schedule(now,1,nxt);
        }
    }

    void timeoutRequest(int id) {
        Request &r = requests[id];
        if(r.completed) return;
        r.timedOut = true;
        dropped++;
    }

    void run() {
        while(!fel.empty()) {
            Event e = fel.top(); fel.pop();
            if(e.time > cfg.simTime) break;
            now = e.time;
            switch(e.type) {
                case 0: startRequest(e.reqId); break;
                case 1: beginService(e.reqId); break;
                case 2: finishService(e.reqId); break;
                case 3: timeoutRequest(e.reqId); break;
            }
        }
        for(int i=0;i<cfg.cores;i++)
            coreBusy[i] += 0.0;
    }

    struct Metrics {
        double avgResp;
        double throughput;
        double goodput;
        double badput;
        double dropRate;
        double util;
    };

    Metrics results() {
        double effTime = cfg.simTime - cfg.warmup;
        Metrics m{};
        m.avgResp = respCount ? respSum/respCount : 0.0;
        m.throughput = completed/effTime;
        m.goodput = good/effTime;
        m.badput = bad/effTime;
        m.dropRate = (double)dropped/(completed+dropped+1e-9);
        m.util = (double)activeThreads/(cfg.maxThreads+1e-9);
        return m;
    }
};

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
    Config cfg;
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