#pragma once

#include "IAlgorithm.h"

// NearestNeighborAlgorithm: greedy heuristic starting from a vertex
class NearestNeighborAlgorithm final : public IAlgorithm {
public:
    explicit NearestNeighborAlgorithm(int startVertex = 0);

    Result solve(const TSPData& data) override;

private:
    [[nodiscard]] int normalizeStartVertex(std::size_t n) const;

    int startVertex_;
};
