#include "sim.h"
/*
    g++ -std=c++20 sim.cpp -O2 -o sim
*/
// using Time = double;

// struct Config {
//     int cores = 4;
//     int maxThreads = 16;
//     int users = 50;
//     Time contextSwitch = 0.0005;
//     Time warmup = 5.0;
//     Time simTime = 60.0;
//     int runs = 10;
//     enum Dist { CONST, UNIFORM, EXP } serviceDist = EXP;
//     Time serviceMin = 0.005;
//     Time serviceVar = 0.01;
//     Time timeoutMin = 0.05;
//     Time timeoutVar = 0.05;
// };


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