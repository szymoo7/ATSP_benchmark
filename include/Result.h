#pragma once

#include <limits>
#include <vector>

// Result: container returned by algorithms (best path, cost, execution time)
struct Result {
    std::vector<int> bestPath;
    int minCost = std::numeric_limits<int>::max();
    long long executionTimeMicroseconds = 0;
};

