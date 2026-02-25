#include <iostream>
#include <vector>
#include <queue>
#include <random>
#include <cmath>
#include <numeric>
#include <algorithm>

using namespace std;

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