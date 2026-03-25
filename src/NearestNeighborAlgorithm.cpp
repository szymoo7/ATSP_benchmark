// Detailed description: Nearest Neighbor heuristic
// - Starts from a chosen start vertex (normalized for instance size).
// - Maintains a 'visited' boolean array and builds the tour by repeatedly choosing
//   the nearest unvisited vertex to the current vertex (greedy choice).
// - Time complexity: O(N^2) in dense representation (checking all candidates each step).
// - Produces a quick approximate tour; not guaranteed optimal.
#include "../include/NearestNeighborAlgorithm.h"

#include "../include/AlgorithmUtils.h"
#include "../include/Timer.h"

#include <limits>
#include <vector>

NearestNeighborAlgorithm::NearestNeighborAlgorithm(int startVertex) : startVertex_(startVertex) {
}

Result NearestNeighborAlgorithm::solve(const TSPData& data) {
    // Prepare result and timer
    Result result;
    Timer timer;
    timer.start();

    // Handle empty / missing data
    if (!data.isLoaded() || data.size == 0) {
        result.minCost = 0;
        result.executionTimeMicroseconds = timer.elapsedMicroseconds();
        return result;
    }

    // Normalize start vertex to [0, N-1]
    const int start = normalizeStartVertex(data.size);

    // visited[i] == true means vertex i is already in the tour
    std::vector<bool> visited(data.size, false);

    // path will hold the order of visited vertices (tour without return to start)
    std::vector<int> path;
    path.reserve(data.size);
    path.push_back(start);
    visited[start] = true;

    // Greedy loop: at each step pick the nearest unvisited vertex
    int current = start;
    for (std::size_t step = 1; step < data.size; ++step) {
        int bestNext = -1;
        int bestWeight = std::numeric_limits<int>::max();

        // Scan all candidates to find the nearest unvisited vertex
        for (std::size_t candidate = 0; candidate < data.size; ++candidate) {
            if (visited[candidate]) {
                continue; // skip already visited
            }
            const int weight = data.distanceMatrix[current][candidate];
            if (weight < bestWeight) {
                bestWeight = weight;
                bestNext = static_cast<int>(candidate);
            }
        }

        // If no next vertex found (shouldn't happen in a complete matrix), stop early
        if (bestNext == -1) {
            break;
        }

        path.push_back(bestNext);
        visited[bestNext] = true;
        current = bestNext;
    }

    // Convert path to result: compute full cycle cost (including return edge)
    result.bestPath = path;
    result.minCost = calculateCycleCost(data, path);
    result.executionTimeMicroseconds = timer.elapsedMicroseconds();
    return result;
}

int NearestNeighborAlgorithm::normalizeStartVertex(std::size_t n) const {
    if (startVertex_ < 0) {
        return 0;
    }
    // Ensure start vertex is within bounds
    return startVertex_ % static_cast<int>(n);
}
