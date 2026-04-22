#pragma once

#include "IAlgorithm.h"

// BranchAndBoundAlgorithm: shared BnB solver with matrix-reduction lower bound.
class BranchAndBoundAlgorithm : public IAlgorithm {
public:
    Result solve(const TSPData& data) override;
    virtual ~BranchAndBoundAlgorithm() = default;

protected:
    enum class Strategy {
        BFS,
        DFS,
        LowestCost
    };

    // strategy: concrete traversal order selected by derived class.
    [[nodiscard]] virtual Strategy strategy() const = 0;
};


