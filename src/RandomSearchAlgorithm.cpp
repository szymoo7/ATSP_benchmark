#include <algorithm>
#include <chrono>
#include <limits>
#include <utility>
#include <vector>

// RandomSearchAlgorithm: random permutation sampling within a given time limit

#include "../include/RandomSearchAlgorithm.h"

#include "../include/AlgorithmUtils.h"

#include <algorithm>
#include <chrono>
#include <limits>
#include <utility>
#include <vector>

RandomSearchAlgorithm::RandomSearchAlgorithm(long long timeLimitMs, std::optional<std::uint32_t> seed)
    : timeLimitMicroseconds_(std::max(1LL, timeLimitMs) * 1000LL),
      rng_(seed.has_value() ? *seed : std::random_device{}()) {
}

// Runs Random algorithm for given data
Result RandomSearchAlgorithm::solve(const TSPData& data) {
    Result result;
    const auto startedAt = std::chrono::steady_clock::now();

    if (!data.isLoaded() || data.size == 0) {
        result.minCost = 0;
        result.executionTimeMicroseconds = 0;
        return result;
    }

    std::vector<int> vertices;
    vertices.reserve(data.size - 1);
    for (std::size_t i = 1; i < data.size; ++i) {
        vertices.push_back(static_cast<int>(i));
    }

    int bestCost = std::numeric_limits<int>::max();
    std::vector<int> bestPath;
    std::vector<int> candidatePath;
    candidatePath.reserve(data.size);

    bool atLeastOneIteration = false;
    while (true) {
        const auto now = std::chrono::steady_clock::now();
        const auto elapsedUs = std::chrono::duration_cast<std::chrono::microseconds>(now - startedAt).count();
        if (atLeastOneIteration && elapsedUs >= timeLimitMicroseconds_) {
            break;
        }

        atLeastOneIteration = true;
        // Shuffles vertices
        std::shuffle(vertices.begin(), vertices.end(), rng_);

        // Adding path
        candidatePath.clear();
        candidatePath.push_back(0);
        candidatePath.insert(candidatePath.end(), vertices.begin(), vertices.end());

        // Updating cost
        const int cost = calculateCycleCost(data, candidatePath);
        if (cost < bestCost) {
            bestCost = cost;
            bestPath = candidatePath;
        }
    }

    result.bestPath = std::move(bestPath);
    result.minCost = bestCost;
    result.executionTimeMicroseconds =
            std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - startedAt)
                    .count();
    return result;
}

