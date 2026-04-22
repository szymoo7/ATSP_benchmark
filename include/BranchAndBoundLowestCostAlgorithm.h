#pragma once

#include "BranchAndBoundAlgorithm.h"

// BranchAndBoundLowestCostAlgorithm: best-first traversal by minimal lower bound.
class BranchAndBoundLowestCostAlgorithm final : public BranchAndBoundAlgorithm {
protected:
    // Use min-heap frontier by lower bound.
    [[nodiscard]] BranchAndBoundAlgorithm::Strategy strategy() const override;
};
