#pragma once

#include "IAlgorithm.h"

#include <vector>

// RepetitiveNNAlgorithm: runs nearest-neighbor heuristic from each start vertex and picks best
class RepetitiveNNAlgorithm final : public IAlgorithm {
public:
    Result solve(const TSPData& data) override;

private:
    [[nodiscard]] static std::vector<int> buildPathFromStart(const TSPData& data, int start);
};
