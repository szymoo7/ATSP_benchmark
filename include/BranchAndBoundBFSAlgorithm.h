#pragma once

#include "BranchAndBoundAlgorithm.h"

// BranchAndBoundBFSAlgorithm: breadth-first tree traversal for BnB.
class BranchAndBoundBFSAlgorithm final : public BranchAndBoundAlgorithm {
protected:
    // Use FIFO frontier expansion.
    [[nodiscard]] BranchAndBoundAlgorithm::Strategy strategy() const override;
};
