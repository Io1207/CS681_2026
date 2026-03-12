#pragma once

#include <iostream>
#include <vector>
#include <queue>
#include <random>
#include <cmath>
#include <numeric>
#include <algorithm>
#include <assert.h>
#include <string>
#include <map>
#include <filesystem>

using namespace std;
namespace fs = std::filesystem;

using Time = double;

struct Event;
class Simulation;
class Request;

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
    unsigned int randInt(int min, int max) {
        uniform_int_distribution<> d(min, max);
        return d(gen);
    } 
};


inline string operator*(string s, int n) {
	string res;
	for (int i = 0; i < n; i++) res += s;
	return res;
}