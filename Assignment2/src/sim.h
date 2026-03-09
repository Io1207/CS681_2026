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

enum return_status {
    GOOD,
    C_TIMEOUT,
    S_TIMEOUT,
    DROP,
    NOT_SET
};

struct Request
{
    static int ctr;
    int id;
    int user;
    Time start = -1;
    Time completion = -1;
    Time c_timeout;
    
    bool timedOut = false;
    bool completed = false;
    bool serverTimedOut = false;
    return_status ret = NOT_SET;
    Time serviceTime;

    vector<Time> serviceStartTimes;
    // vector<Time> coreStartTimes;
    vector<Time> serviceTimes;
    vector<Time> serviceResponseTimes;
    int activeServer;
    Time remService;

    Request(int id, int user, int serverCount, Time ttl) : user(user), c_timeout(ttl) {
        ctr++;
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
    Time timeoutMin;
    Time timeoutVar;
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
        Time timeoutOverhead = 0.001;
        int visitCount = 0;
        ClientNode(int users, Time thinkMin, Time thinkVar, unsigned seed, vector<double> outProbs);
        Time thinkTime();
        int route();
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
    Event(Time time, EventTypes type, int serverId, int reqId, int core = -1) : time(time), type(type), serverId(serverId), coreId(coreId), reqId(reqId) {
        coreId = core;
    }
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

class Simulation {
    Config cfg;
    RNG rng;
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
    ofstream log_file;

    private:
        void printLog(Time reqTime);

        Time thinkTime();
        Time sampleClientTimeout();
        void scheduleThink(int user, Time base);

    public:
        Time now = 0.0;
        Simulation(Config c);
        ~Simulation() {
            log_file.close();
        }

        void run();
        Request& getRequest(int reqId);
        void clientTimeout(Request& req);
        Time simTime();
        SimMetrics results();

        void schedule(Time t, EventTypes type, int sId, int reqId, int coreId = -1);
};


