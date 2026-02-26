#pragma once

#include <iostream>
#include <vector>
#include <queue>
#include <random>
#include <cmath>
#include <numeric>
#include <algorithm>

#include "servers.h"

using namespace std;

using Time = double;

struct Config
{
    int users = 50;
    Time warmup = 5.0;
    Time simTime = 60.0;
    int runs = 10;
    vector<ServerConfig> scfgs;

    int cores = 4;
    int maxThreads = 16;
    Time contextSwitch = 0.0005;
    enum Dist
    {
        CONST,
        UNIFORM,
        EXP
    } serviceDist = EXP;
    Time serviceMin = 0.005;
    Time serviceVar = 0.01;
    Time timeoutMin = 0.05;
    Time timeoutVar = 0.05;
};

struct RNG
{
    mt19937 gen;
    RNG(unsigned s) : gen(s) {}
    double uniform(double a, double b)
    {
        uniform_real_distribution<> d(a, b);
        return d(gen);
    }
    double exponential(double lambda)
    {
        exponential_distribution<> d(lambda);
        return d(gen);
    }
    double lognormal(double m, double s)
    {
        lognormal_distribution<> d(m, s);
        return d(gen);
    }
};

struct Request
{
    int id;
    int user;
    Time start = -1;
    Time completion = -1;
    bool timedOut = false;
    bool completed = false;
    bool serverTimedOut = false;

    vector<Time> serviceStartTimes;
    vector<Time> coreStartTimes;
    vector<Time> serviceResponseTimes;
    vector<Time> serviceTimes;
    int activeServer;
    Time remService;

    Request(int id, int user, int serverCount) : id(id), user(user)
    {
        serviceStartTimes.resize(serverCount+1, -1);
        serviceResponseTimes.resize(serverCount+1, 0);
        coreStartTimes.resize(serverCount+1, -1);
        serviceTimes.resize(serverCount+1, 0);

        activeServer = -1;
        remService = 0;
    }   

};

enum EventTypes
{

    LEAVE_CLIENT,
    ENTER_CLIENT,
    ENTER_SERVER,
    CTX_SWITCH_IN,
    CTX_SWITCH_OUT,
    FINISH_SERVICE,
    CLIENT_TIMEOUT,
    SERVER_TIMEOUT,

};

struct Event
{
    Time time;
    EventTypes type;
    int serverId;
    int coreId = -1;
    int reqId;
    bool operator>(Event const &o) const { return time > o.time; }
};

class Simulation
{
    Config cfg;
    RNG rng;
    Time now = 0.0;

    priority_queue<Event, vector<Event>, greater<Event>> fel;

    vector<Request> requests;
    // queue<int> threadQueue;
    // int activeThreads = 0;
    // int nextCore = 0;

    // vector<Time> coreBusy;
    // vector<Time> coreLastStart;

    vector<Server> servers;

    int nextReqId = 0;

    int completed = 0;
    int good = 0;
    int bad = 0;
    int dropped = 0;

    double respSum = 0.0;
    int respCount = 0;

public:
    Simulation(Config c, unsigned seed) : cfg(c), rng(seed)
    {
        coreBusy.assign(cfg.cores, 0.0);
        coreLastStart.assign(cfg.cores, 0.0);
        for (int i = 0; i < cfg.users; i++)
            scheduleThink(i, 0.0);
    }

    double simTime()
    {
        return now;
    }

    double serviceTime()
    {
        if (cfg.serviceDist == Config::CONST)
            return cfg.serviceMin;
        if (cfg.serviceDist == Config::UNIFORM)
            return rng.uniform(cfg.serviceMin, cfg.serviceMin + cfg.serviceVar);
        return cfg.serviceMin + rng.exponential(1.0 / cfg.serviceVar);
    }

    double timeoutTime()
    {
        return cfg.timeoutMin + rng.uniform(0.0, cfg.timeoutVar);
    }

    double thinkTime()
    {
        return rng.lognormal(0.0, 0.5);
    }

    Request &getRequest(int reqId)
    {
        return requests[reqId];
    }

    void schedule(Time t, EventTypes type, int sId, int reqId, int coreId = -1)
    {
        fel.push({t, type, sId, coreId, reqId});
    }

    void scheduleThink(int user, Time base)
    {
        Time t = base + thinkTime();
        schedule(t, LEAVE_CLIENT, user);
    }

    void startRequest(int user)
    {
        Request r;
        r.id = nextReqId++;
        r.user = user;
        r.arrival = now;
        requests.push_back(r);
        int id = r.id;
        schedule(now + timeoutTime(), CLIENT_TIMEOUT, id);
        if (activeThreads < cfg.maxThreads)
        {
            activeThreads++;
            schedule(now, ENTER_SERVER, id);
        }
        else
        {
            threadQueue.push(id);
        }
    }

    void beginService(int id)
    {
        Request &r = requests[id];
        if (r.timedOut)
            return;
        r.start = now;
        int core = nextCore;
        nextCore = (nextCore + 1) % cfg.cores;
        coreLastStart[core] = now;
        Time s = serviceTime() + cfg.contextSwitch;
        schedule(now + s, FINISH_SERVICE, id);
    }

    void finishService(int id)
    {
        Request &r = requests[id];
        r.completion = now;
        r.completed = true;
        activeThreads--;
        completed++;
        if (r.timedOut)
            bad++;
        else
            good++;
        if (now > cfg.warmup && !r.timedOut)
        {
            respSum += r.completion - r.start;
            respCount++;
        }
        scheduleThink(r.user, now);
        if (!threadQueue.empty())
        {
            int nxt = threadQueue.front();
            threadQueue.pop();
            activeThreads++;
            schedule(now, ENTER_SERVER, nxt);
        }
    }

    void timeoutRequest(int id)
    {
        Request &r = requests[id];
        if (r.completed)
            return;
        r.timedOut = true;
        dropped++;
    }

    void run()
    {
        while (!fel.empty())
        {
            Event e = fel.top();
            fel.pop();
            if (e.time > cfg.simTime)
                break;
            now = e.time;
            switch (e.type)
            {
            case LEAVE_CLIENT:
                startRequest(e.reqId);
                break;
            case ENTER_SERVER:
                beginService(e.reqId);
                break;
            case FINISH_SERVICE:
                finishService(e.reqId);
                break;
            case CLIENT_TIMEOUT:
                timeoutRequest(e.reqId);
                break;
            }
        }
        for (int i = 0; i < cfg.cores; i++)
            coreBusy[i] += 0.0;
    }

    struct Metrics
    {
        double avgResp;
        double throughput;
        double goodput;
        double badput;
        double dropRate;
        double util;
    };

    Metrics results()
    {
        double effTime = cfg.simTime - cfg.warmup;
        Metrics m{};
        m.avgResp = respCount ? respSum / respCount : 0.0;
        m.throughput = completed / effTime;
        m.goodput = good / effTime;
        m.badput = bad / effTime;
        m.dropRate = (double)dropped / (completed + dropped + 1e-9);
        m.util = (double)activeThreads / (cfg.maxThreads + 1e-9);
        return m;
    }
};
