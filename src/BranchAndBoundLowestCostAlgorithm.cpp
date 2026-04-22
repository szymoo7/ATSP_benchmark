#include "../include/BranchAndBoundLowestCostAlgorithm.h"

// Select best-first traversal in the shared BnB solver.
BranchAndBoundAlgorithm::Strategy BranchAndBoundLowestCostAlgorithm::strategy() const {
    return Strategy::LowestCost;
}
