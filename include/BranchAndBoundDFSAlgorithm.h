#pragma once

#include "BranchAndBoundAlgorithm.h"

// BranchAndBoundDFSAlgorithm: depth-first tree traversal for BnB.
class BranchAndBoundDFSAlgorithm final : public BranchAndBoundAlgorithm {
protected:
    // Use LIFO frontier expansion.
    [[nodiscard]] BranchAndBoundAlgorithm::Strategy strategy() const override;
};
