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

struct Config {
    int users = 50;
    Time warmup = 5.0;
    Time simTime = 60.0;
    int runs = 10;
    vector<ServerConfig> scfgs;



    int cores = 4;
    int maxThreads = 16;
    Time contextSwitch = 0.0005;
    enum Dist { CONST, UNIFORM, EXP } serviceDist = EXP;
    Time serviceMin = 0.005;
    Time serviceVar = 0.01;
    Time timeoutMin = 0.05;
    Time timeoutVar = 0.05;
 
    
};