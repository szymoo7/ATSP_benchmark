#include "../include/BranchAndBoundBFSAlgorithm.h"

// Select BFS traversal in the shared BnB solver.
BranchAndBoundAlgorithm::Strategy BranchAndBoundBFSAlgorithm::strategy() const {
    return Strategy::BFS;
}
