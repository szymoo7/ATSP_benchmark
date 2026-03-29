// BruteForceAlgorithm::solve: exhaustive search for optimal tour
// - Fixes vertex 0 as start and permutes remaining vertices to evaluate all tours
// - Time complexity: O((N-1)!), memory: O(N) for current path
// - Only suitable for very small N; used primarily for calibration/reference

#include "../include/BruteForceAlgorithm.h"

#include "../include/AlgorithmUtils.h"
#include "../include/Timer.h"

#include <algorithm>
#include <limits>
#include <utility>
#include <vector>

// Runs algorithm for given data
Result BruteForceAlgorithm::solve(const TSPData& data) {
    Result result;
    Timer timer;
    timer.start();

    if (!data.isLoaded() || data.size == 0) {
        result.minCost = 0;
        result.executionTimeMicroseconds = timer.elapsedMicroseconds();
        return result;
    }

    if (data.size == 1) {
        result.bestPath = {0};
        result.minCost = data.distanceMatrix[0][0];
        result.executionTimeMicroseconds = timer.elapsedMicroseconds();
        return result;
    }

    // Prepare permutation of vertices 1..N-1 (fix 0 as start)
    std::vector<int> perm;
    perm.reserve(data.size - 1);
    for (std::size_t v = 1; v < data.size; ++v) {
        perm.push_back(static_cast<int>(v));
    }

    int bestCost = std::numeric_limits<int>::max();
    std::vector<int> bestPath;
    std::vector<int> path;
    path.reserve(data.size);

    // Iterate all permutations
    do {
        path.clear();
        path.push_back(0);
        path.insert(path.end(), perm.begin(), perm.end());

        // compute full cycle cost including return edge
        const int cost = calculateCycleCost(data, path);
        if (cost < bestCost) {
            bestCost = cost;
            bestPath = path;
        }
    } while (std::next_permutation(perm.begin(), perm.end()));

    result.bestPath = std::move(bestPath);
    result.minCost = bestCost;
    result.executionTimeMicroseconds = timer.elapsedMicroseconds();
    return result;
}
