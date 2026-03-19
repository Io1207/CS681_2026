#include "sim.h"
#include "servers.h"
#include <filesystem>

struct Config;
namespace fs = std::filesystem;

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
    ifstream configFile(filename);
    // cout << fs::current_path().string() << endl;
    // cout << filename << endl;
    if (!configFile.is_open()) {
        throw runtime_error("Could not open config file");
    }
    configFile >> numDelays;
    configFile >> users;
    configFile >> ease_in >> timeoutMin >> timeoutVar >> thinkMin >> thinkVar;
    configFile >> numServers;

    adjMatrix.resize(numServers+1);
    for(int i=0;i<=numServers;i++) {
        adjMatrix[i].resize(numServers+1);
        for(int j=0;j<=numServers;j++) {
            configFile >> adjMatrix[i][j];
        }
    }

    scfgs.resize(numServers);
    for(int i=0; i<numServers; i++) {
        ServerConfig &s = scfgs[i];

        configFile >> s.numCores >> s.maxThreads >> s.maxQueue;
        
        int dist;
        configFile >> dist >> s.serviceMin >> s.serviceVar;
        if (dist == 0) s.serviceDist = ServerConfig::CONST;
        else if (dist == 1) s.serviceDist = ServerConfig::UNIFORM;
        else s.serviceDist = ServerConfig::EXP;

        configFile >> s.contextSwitchTime >> s.roundRobinTimeout;
        configFile >> s.timeout;

        s.outProbs = adjMatrix[i+1];
    }

    int changePresent;
    configFile >> changePresent;
    if (!changePresent) return;
    else {
        configFile >> numChanges;
        configFile >> nextChangeTime >> nextChangeType;
    }
    this->linesRead = 7 + (numServers+1)*numServers + numServers*9 + 2;
}

ClientNode::ClientNode(int users, Time thinkMin, Time thinkVar, unsigned seed, vector<double> _outProbs):
                 users(users), thinkMin(thinkMin), thinkVar(thinkVar), rng(seed) 
{
    double cumProbs = 0;
    for (int i = 0; i < _outProbs.size(); i++) {
        if (_outProbs[i] > 0) {
            cumProbs += _outProbs[i];
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

Simulation::Simulation(Config c, int seed, string dirname) : cfg(c), rng(seed), 
                                   client(cfg.users, cfg.thinkMin, cfg.thinkVar, cfg.run_id, cfg.adjMatrix[0])
{
    metrics.nodeVisitCounts.resize(cfg.numServers);
    metrics.serverDemands.resize(cfg.numServers);
    metrics.serverResponseTimes.resize(cfg.numServers);

    log_file.open(dirname + "/run_" + to_string(cfg.run_id) + "/main.log");
    event_log.open(dirname + "/run_" + to_string(cfg.run_id) + "/events.log");
    config.open(dirname + "/config.txt");
    now = 0.0;
    Time lastScheduled = 0.0;
    // servers.emplace_back(0,cfg.scfgs[0],this,0,)
    // servers.emplace_back();
    for(int i=0;i<cfg.numServers;i++)
    {
        // Server serv;
        servers.emplace_back(i+1,cfg.scfgs[i],this,rng.randInt(0,1000000000), dirname + "/run_" + to_string(cfg.run_id));
    }

    for(int i=0;i<cfg.users;i++) {
        lastScheduled += rng.exponential(double(cfg.users)/cfg.ease_in);
        // requests.emplace_back(i, cfg.numServers, sampleClientTimeout());
        schedule(lastScheduled, LEAVE_CLIENT, client.route(), i, i);
    }
    if (cfg.numChanges > 0) {
        schedule(cfg.nextChangeTime, SYS_SET_CHANGE, -1, -1);
        cfg.numChanges--;
    }
}

void Simulation::handleConfigChange(Event &e) {
    // This is where we will handle any dynamic changes to the system configuration. For now, we will just schedule the next change.
    vector<vector<double>> newAdjMatrix (cfg.numServers+1, vector<double>(cfg.numServers+1));
    int id;
    switch ((int)cfg.nextChangeType) {
        case 0:
            for (int i = 0; i < cfg.numServers; i++) {
                config >> id;
                config >> cfg.scfgs[id - 1].numCores >> cfg.scfgs[id - 1].maxThreads >> cfg.scfgs[id - 1].maxQueue;
                config >> cfg.scfgs[id - 1].serviceMin >> cfg.scfgs[id - 1].serviceVar;
            }
            break;
        case 1:
            for (int i = 0; i < cfg.numServers+1; i++) {
                for (int j = 0; j < cfg.numServers+1; j++) {
                    config >> newAdjMatrix[i][j];
                }
            }
            for (int i = 0; i < cfg.numServers; i++) {
                cfg.scfgs[i].outProbs = newAdjMatrix[i+1];
            }
            break;
        case 2:
            for (int i = 0; i < cfg.numServers+1; i++) {
                for (int j = 0; j < cfg.numServers+1; j++) {
                    config >> newAdjMatrix[i][j];
                }
            }
            for (int i = 0; i < cfg.numServers; i++) {
                cfg.scfgs[i].outProbs = newAdjMatrix[i+1];
            }
            for (int i = 0; i < cfg.numServers; i++) {
                config >> id;
                config >> cfg.scfgs[id - 1].numCores >> cfg.scfgs[id - 1].maxThreads >> cfg.scfgs[id - 1].maxQueue;
                config >> cfg.scfgs[id - 1].serviceMin >> cfg.scfgs[id - 1].serviceVar;
            }
            break;
        default:
            throw runtime_error("Invalid change type");
    }
    if (cfg.numChanges > 0) {
        config >> cfg.nextChangeTime >> cfg.nextChangeType;
        schedule(cfg.nextChangeTime, SYS_SET_CHANGE, -1, -1);
        cfg.numChanges--;
    }
}

void Simulation::schedule(Time t, EventTypes type, int sId, int reqId, int coreId) {
    fel.push(Event(t, type, sId, reqId, coreId));
}

void Simulation::scheduleThink(int user, Time base) {
    // requests.emplace_back(user, cfg.numServers, sampleClientTimeout());
    schedule(base+thinkTime(), LEAVE_CLIENT, client.route(), -1, user);
    // printLog(); // ?
}

Request& Simulation::getRequest(int reqId) {
    return requests[reqId];
}

Time Simulation::simTime() {
    return now;
}

Time Simulation::thinkTime() {
    return cfg.thinkMin + rng.lognormal(0.0,cfg.thinkVar);    
}

// void Simulation::handleConfigChange(Event &e) {
//     schedule(fel.end().time, CHANGE, -1, -1);
// }

void Simulation::run(vector<double>& visit, vector<double>& service_time, vector<double>& response_time) {
    Event e;
    while (!fel.empty()) {
        if (client.visitCount >= cfg.numDelays) {
            fel = {};
            for (int i=0;i<cfg.numServers;i++) {
                visit[i] +=  servers[i].getVisits();
                service_time[i] += servers[i].getAvgServiceTime();
                response_time[i] += servers[i].getAvgResponseTime();
            }
            break;
        }
        e = fel.top();
        fel.pop();
        now = e.time;
        event_log << e;
        switch (e.type) {
            case LEAVE_CLIENT: 
                requests.emplace_back(e.coreId , cfg.numServers, sampleClientTimeout()); 
                schedule(
                    simTime() + requests.back().c_timeout,
                    CLIENT_TIMEOUT,
                    0,
                    requests.back().id,
                    requests.back().user);
                schedule(e.time, ENTER_SERVER, e.serverId, requests.back().id);
                requests.back().start = simTime();
                break;
            case ENTER_CLIENT: 
                handleEnterClient(e, getRequest(e.reqId));
                break;
            case SYS_SET_CHANGE:
                handleConfigChange(e);
                break;
            case CLIENT_TIMEOUT: 
                clientTimeout(getRequest(e.reqId)); 
                break;
            default: 
                servers[e.serverId-1].processEvent(e); 
                break;
        }
    }
}

void Simulation::handleEnterClient(Event& e, Request& req)
{
    if(req.ended)
        throw "Request Re-entering server";
    req.ended = true;
    client.visitCount++;
    req.completion = simTime();

    scheduleThink(req.user, e.time); 
    switch(req.ret)
    {
        case (int)C_TIMEOUT:
            bad++;
            break;
        case (int)GOOD:
            good++;
            break;
        case (int)S_TIMEOUT:
            dropped++;
            break;
        case (int)DROP:
            dropped++;
            break;
        default:
            break;
    }
    if(req.ret == S_TIMEOUT)
        client.doomCount++;
    if(req.ret != S_TIMEOUT && req.ret != DROP)
        completed++;
    if (req.ret == GOOD)
    {
        client.seenDelays++;
        appendLog(req.completion - req.start);
    }
}

void Simulation::clientTimeout(Request& req) {
    // This is called when a request times out at the client node. We need to schedule the 
    // next think time for this user.
    if(req.completed || req.ended)
        return;
    req.timedOut = true;
    req.setReturnStatus(C_TIMEOUT);
    scheduleThink(req.user, simTime());
    bad++;
    completed++;
    // scheduleThink(req.user, simTime()+client.timeoutOverhead);
    // schedule(simTime(), ENTER_CLIENT, -1, req.id);
}

SimMetrics Simulation::results() {
    // double effTime = simTime() - cfg.warmup;
    // SimMetrics m{};
    // int totalCompleted = 0;
    // for (int i = 0; i<cfg.numServers; i++) {
    //     totalCompleted += servers[i].getCompleted();
    //     m.nodeVisitCounts[i] = servers[i].getVisits();
    //     m.serverDemands[i] = servers[i].getAvgServiceTime();
    //     m.serverDemands[i] = servers[i].getAvgResponseTime();
    // }
    // int totalVisits = accumulate(m.nodeVisitCounts.begin()+1, m.nodeVisitCounts.end(), 0);
    
    // transform(m.nodeVisitCounts.begin()+1, m.nodeVisitCounts.end(), m.nodeVisitCounts.begin(), [totalVisits] (int x) {return x/totalVisits;});

    // for (int i = 0; i<cfg.numServers; i++) {
    //     m.serverResponseTimes[i] = m.serverResponseTimes[i] / m.nodeVisitCounts[i];

    // }

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

vector<double> linEqSolver(vector<vector<double>> A, vector<double> b)
{
    double eps = 1e-9;
    if(A.size() == 0)
        return {};
    if(A.size() != b.size())
        throw "Invalid Equations";

    int n = (int) A.size();
    int m = (int) A[0].size();

    vector<int> where (m, -1);
    for (int col=0, row=0; col<m && row<n; ++col) {
        int sel = row;
        for (int i=row; i<n; ++i)
            if (abs (A[i][col]) > abs (A[sel][col]))
                sel = i;
        if (abs (A[sel][col]) < eps)
            continue;
        for (int i=col; i<=m; ++i)
        {
            swap (A[sel][i], A[row][i]);
            swap(b[sel], b[row]);
        }
        where[col] = row;

        for (int i=0; i<n; ++i)
            if (i != row) {
                double c = A[i][col] / A[row][col];
                for (int j=col; j<m; ++j)
                    A[i][j] -= A[row][j] * c;
                b[i] -= b[row]*c;
            }
        ++row;
    }

    vector<double> ans(m, 0);
    for (int i=0; i<m; ++i)
        if (where[i] != -1)
            ans[i] = b[where[i]] / A[where[i]][i];
    for (int i=0; i<n; ++i) {
        double sum = 0;
        for (int j=0; j<m; ++j)
            sum += ans[j] * A[i][j];
        if (abs (sum - A[i][m]) > eps)
            return {};
    }

    for (int i=0; i<m; ++i)
        if (where[i] == -1)
            return {};
    return ans;


}

Time meanServiceTime(const ServerConfig& cf)
{
    switch(cf.serviceDist)
    {
        case ServerConfig::CONST:
            return cf.serviceMin;
        case ServerConfig::UNIFORM:
            return cf.serviceMin + cf.serviceVar/2;
        case ServerConfig::EXP:
            return cf.serviceMin + cf.serviceVar;
        default:
            return cf.serviceMin;
    }
}

SimMetrics getTheoreticalResults(const Config& sim_config, const vector<ServerConfig>& sfgs) {
    SimMetrics t_results = SimMetrics();
    
    // getting theoretical relative visits
    vector<vector<double>> A(sim_config.numServers, vector<double>(sim_config.numServers));
    vector<double> b(sim_config.numServers);

    /**
     * VP = V; V[0] = 1; 
     * P =  | P00  P0 |
     *      | P'0  At |
     * V = | V0 V' |
     *      
     * Solve V' (At - I) = - V0 P0
     * Check against V0 P00 + V' P'0 = V0
     */
    for(int i=0; i<sim_config.numServers;i++)
    {
        for(int j=0;j<sim_config.numServers;j++)
        {
            A[i][j] = sim_config.adjMatrix[j+1][i+1];
            if(i == j)  A[i][j] -= 1;
        }
    }
    for(int i=0;i<sim_config.numServers;i++)
    {
        b[i] = - sim_config.adjMatrix[0][i+1];
    }
    vector<double> V = linEqSolver(A, b);
    
    if(V.empty())   
        throw "Empty Solution";
    else
    {
        double sum = 0;
        sum += sim_config.adjMatrix[0][0];
        for(int i=0;i<sim_config.numServers;i++)
        {
            sum += b[i] * sim_config.adjMatrix[i+1][0];
        } 
        if(abs(sum - 1.0) > 1e-9)
            throw "Bad Solution";
    }


    vector<double> visits(sim_config.numServers + 1);
    visits[0] = 1;
    copy(V.begin(), V.end(), visits.begin()+1);

    vector<double> serviceTime(sim_config.numServers + 1, 0.0);
    vector<double> serverDemand(sim_config.numServers + 1, 0.0);
    for(int i=1; i<=sim_config.numServers;i++)
    {
        serviceTime[i] = meanServiceTime(sfgs[i-1]);
        serverDemand[i] = serviceTime[i] * visits[i];
    }

    // num-in-system, response time, throughput
    // idx[0] --> system
    // idx[i] --> server i
    vector<vector<double>> N(sim_config.users+1, vector<double>(sim_config.numServers+1,0));
    vector<vector<double>> R(sim_config.users+1, vector<double>(sim_config.numServers+1,0));
    vector<vector<double>> T(sim_config.users+1, vector<double>(sim_config.numServers+1,0));
    for(int M = 1;M <= sim_config.users; M++)
    {
        for(int serv = 1;serv <= sim_config.numServers;serv++)
        {
            R[M][serv] = (1 + N[M-1][serv]) * serviceTime[serv];
            R[M][0] += R[M][serv] * visits[serv];
        }
        // N[M][0] = M;
        T[M][0] = M / R[M][0];
        for(int serv = 1; serv <= sim_config.numServers;serv++)
        {
            T[M][serv] = T[M][0] * visits[serv];
            N[M][serv] = T[M][serv] * R[M][serv];
        }
        
    }

    int totM = sim_config.users;
    t_results.serverDemands.resize(sim_config.numServers);
    t_results.serverResponseTimes.resize(sim_config.numServers);
    t_results.throughputs.resize(sim_config.numServers);
    t_results.nodeVisitCounts.resize(sim_config.numServers);
    move(serverDemand.begin()+1, serverDemand.end(), t_results.serverDemands.begin());
    move(R[totM].begin()+1,R[totM].end(),t_results.serverResponseTimes.begin());
    move(T[totM].begin()+1,T[totM].end(),t_results.throughputs.begin());
    move(N[totM].begin()+1,N[totM].end(),t_results.nodeVisitCounts.begin());

    // Why is util not a vector?
    t_results.throughput = T[totM][0];
    
    // How to get theoretical goodput, badput, dropRate?
    // I don't think we can theoretically model a limited buffer and drops

    t_results.sysResponseTime = R[totM][0];

    return t_results;

    // vector<double> curr_out(sim_config.numServers + 1);
    // for (int i = 0; i <= sim_config.numServers; i++) {
        
    // }
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

// Arg1 - simulation directory path
// Arg 2 - random seed
int main(int argc, char* argv[]) {
    
    int seed;
    string dir_name;
    if(argc <= 1)
        dir_name = "";
    else
        dir_name = argv[1];
    
    if(argc <=2)
        seed = 0;
    else 
        seed = stoi(argv[2]);
    

    Config cfg = Config(dir_name + "/config.txt");
    vector<double> resp;
    vector<double> thr;
    vector<double> good;
    vector<double> bad;
    vector<double> drop;
    vector<double> util;

    vector<double> vis_count(cfg.numServers, 0.0);
    vector<double> service_time(cfg.numServers, 0.0);
    vector<double> response_time(cfg.numServers, 0.0);

    for(int r=0;r<cfg.runs;r++) {
        cfg.run_id = r + 1;
        Simulation sim(cfg, seed+r, dir_name);
        sim.run(vis_count, service_time, response_time);
        auto m = sim.results();
        // resp.push_back(m.avgResp);
        thr.push_back(m.throughput);
        good.push_back(m.goodput);
        bad.push_back(m.badput);
        drop.push_back(m.dropRate);
        // util.push_back(m.util);
    }
    transform(vis_count.begin(), vis_count.end(), vis_count.begin(), [cfg] (double x) {return x/(cfg.numDelays*cfg.runs);});
    // getting avgServiceTime and avgResponseTime for each server
    for (int i=0;i<cfg.numServers;i++) {
        service_time[i] /= (cfg.runs*vis_count[i]) + 1e-6;
        response_time[i] /= (cfg.runs*vis_count[i]) + 1e-6;
    }
    cout << "------------------------------------------------------------------------------" << endl;
    cout << cfg.users << endl << endl;
    cout << "Visits: ";
    for(int i=0;i<cfg.numServers;i++) cout << vis_count[i] << " ";
    cout << "\nService Time: ";
    for(int i=0;i<cfg.numServers;i++) cout << service_time[i] << " ";
    cout << "\nResponse Time: ";
    for(int i=0;i<cfg.numServers;i++) cout << response_time[i] << " ";
    cout << "\n";
    cout << "------------------------------------------------------------------------------" << endl;

    // auto [meanR,ciR] = meanCI(resp);
    // cout<<"Avg Response Time: "<<meanR<<" +/- "<<ciR<<"\n";
    // cout<<"Throughput: "<<accumulate(thr.begin(),thr.end(),0.0)/thr.size()<<"\n";
    // cout<<"Goodput: "<<accumulate(good.begin(),good.end(),0.0)/good.size()<<"\n";
    // cout<<"Badput: "<<accumulate(bad.begin(),bad.end(),0.0)/bad.size()<<"\n";
    // cout<<"Drop Rate: "<<accumulate(drop.begin(),drop.end(),0.0)/drop.size()<<"\n";
    // cout<<"Utilization: "<<accumulate(util.begin(),util.end(),0.0)/util.size()<<"\n";
}

// void Simulation::printLog(Time time) {
//     // this is essentially going to be stats for the client node
//     // no server stats here
//     // one thing I don't get is how are we planning to do request wise stats? Do we even want to do that?

//     log_file << simTime() - time << "\n";
// }