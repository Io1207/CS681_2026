#include <iostream>
#include <vector>
#include <queue>
#include <random>
#include <cmath>
#include <numeric>
#include <algorithm>
#include <assert.h>
#include <string>

using namespace std;

using Time = double;

struct Event;
class Simulation;
class Request;

string operator*(string s, int n) {
	string res;
	for (int i = 0; i < n; i++) res += s;
	return res;
}