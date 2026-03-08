#pragma once

#include <iostream>
#include <vector>
#include <queue>
#include <random>
#include <cmath>
#include <numeric>
#include <algorithm>
#include <fstream>
#include <ostream>
#include <iomanip>

#include "servers.h" 

using namespace std;

using Time = double;
using return_status = int;

enum return_status {
    GOOD = 0,
    C_TIMEOUT = 1,
    S_TIMEOUT = 2,
    DROP = 3
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
    int ret;

    vector<Time> serviceStartTimes;
    // vector<Time> coreStartTimes;
    vector<Time> serviceTimes;
    vector<Time> serviceResponseTimes;
    vector<Time> serviceTimes;
    int activeServer;
    Time remService;

    Request(int id, int user, int serverCount) : id(id), user(user)
    {
        serviceStartTimes.resize(serverCount+1, -1);
        serviceResponseTimes.resize(serverCount+1, 0);
        // coreStartTimes.resize(serverCount+1, -1);
        serviceTimes.resize(serverCount+1, 0);

        activeServer = -1;
        remService = 0;
    }

    void setReturnStatus(return_status ret) {
        this->ret = ret;
    }

    ~Request() {}
};

struct Config {
    int users;
    int numServers;
    int numDelays;
    int run_id;
    vector<vector<double>> adjMatrix;
    vector<ServerConfig> scfgs;
    Time ease_in;
    Time timeout;
    Time thinkMin;
    Time thinkVar;

    // up for discussion for different simulation
    bool sys_other = false;

    int runs = 10;
    Config() = default;
    Config(string filename);
};

class ClientNode {
    int users;
    Time thinkMin;
    Time thinkVar;
    RNG rng;
    vector<pair<int,double>> outProbs;
    public:
        int visitCount = 0;
        ClientNode(int users, Time thinkMin, Time thinkVar, unsigned seed, vector<double> outProbs);
        Time thinkTime();
        int route();
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

enum EventTypes {   
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
    bool operator>(Event const& o) const { return time > o.time; }
    Event() = default;
    Event(Time time, EventTypes type, int serverId, int coreId, int reqId) : time(time), type(type), serverId(serverId), coreId(coreId), reqId(reqId) {}
};

class Simulation {
    Config cfg;
    RNG rng;
    Time now = 0.0;
    ClientNode client;
    SimMetrics metrics;

    priority_queue<Event, vector<Event>, greater<Event>> fel;

    vector<Request> requests;

    vector<Server> servers;

    int nextReqId = 0;

    int completed = 0;
    int good = 0;
    int bad = 0;
    int dropped = 0;

    double respSum = 0.0;
    int respCount = 0;
    ofstream log_file;

    private:
        void printLog(Time reqTime);

        Time thinkTime();
        void scheduleThink(int user, Time base);

    public:
        Simulation(Config c);
        ~Simulation() {
            log_file.close();
        }

        void run();
        Request& getRequest(int reqId); // not even used
        void clientTimeout(Request& req);
        Time simTime();
        SimMetrics results();

        void schedule(Time t, EventTypes type, int sId, int reqId, int coreId = -1);
};


struct SimMetrics {
    vector<int> nodeVisitCounts;
    vector<double> serverDemands;
    vector<double> serverResponseTimes;
    vector<double> throughputs;
    double util;
    double throughput;
    double goodput;
    double badput;
    double dropRate;
    double sysResponseTime;
};