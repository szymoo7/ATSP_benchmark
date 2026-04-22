#include "../include/BranchAndBoundDFSAlgorithm.h"

// Select DFS traversal in the shared BnB solver.
BranchAndBoundAlgorithm::Strategy BranchAndBoundDFSAlgorithm::strategy() const {
    return Strategy::DFS;
}
