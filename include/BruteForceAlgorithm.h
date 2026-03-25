#pragma once

#include "IAlgorithm.h"

// BruteForceAlgorithm: exhaustive search for optimal TSP tour (only for small N)
class BruteForceAlgorithm final : public IAlgorithm {
public:
    Result solve(const TSPData& data) override;
};
