#include <limits>
#include <utility>
#include <vector>

// RepetitiveNNAlgorithm: run NN from every start vertex and keep best result
#include "../include/RepetitiveNNAlgorithm.h"

#include "../include/AlgorithmUtils.h"
#include "../include/Timer.h"

#include <limits>
#include <utility>
#include <vector>

// Runs RNN algorithm for given data
Result RepetitiveNNAlgorithm::solve(const TSPData& data) {
    Result result;
    Timer timer;
    timer.start();

    if (!data.isLoaded() || data.size == 0) {
        result.minCost = 0;
        result.executionTimeMicroseconds = timer.elapsedMicroseconds();
        return result;
    }

    int globalBestCost = std::numeric_limits<int>::max();
    std::vector<int> globalBestPath;

    // Builds NN algorithm path for each vertice
    for (std::size_t start = 0; start < data.size; ++start) {
        auto path = buildPathFromStart(data, static_cast<int>(start));
        const int cost = calculateCycleCost(data, path);
        if (cost < globalBestCost) {
            globalBestCost = cost;
            globalBestPath = std::move(path);
        }
    }

    result.bestPath = std::move(globalBestPath);
    result.minCost = globalBestCost;
    result.executionTimeMicroseconds = timer.elapsedMicroseconds();
    return result;
}

// Runs NN algorith for given data with given start vertice
std::vector<int> RepetitiveNNAlgorithm::buildPathFromStart(const TSPData& data, int start) {
    std::vector<bool> visited(data.size, false);
    std::vector<int> path;
    path.reserve(data.size);
    path.push_back(start);
    visited[start] = true;

    int current = start;
    for (std::size_t step = 1; step < data.size; ++step) {
        int bestNext = -1;
        int bestWeight = std::numeric_limits<int>::max();

        for (std::size_t candidate = 0; candidate < data.size; ++candidate) {
            if (visited[candidate]) {
                continue;
            }
            const int weight = data.distanceMatrix[current][candidate];
            if (weight < bestWeight) {
                bestWeight = weight;
                bestNext = static_cast<int>(candidate);
            }
        }

        if (bestNext == -1) {
            break;
        }

        visited[bestNext] = true;
        path.push_back(bestNext);
        current = bestNext;
    }

    return path;
}

