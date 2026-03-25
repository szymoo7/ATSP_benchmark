#pragma once

#include "Result.h"
#include "TSPData.h"

// IAlgorithm: abstract interface for TSP algorithms
class IAlgorithm {
public:
    virtual ~IAlgorithm() = default;
    // solve: run the algorithm and return Result (path, cost, time)
    virtual Result solve(const TSPData& data) = 0;
};

