#include "../include/SimulatedAnnealingAlgorithm.h"

#include "../include/AlgorithmUtils.h"
#include "../include/NearestNeighborAlgorithm.h"
#include "../include/Timer.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <random>
#include <vector>

namespace {

// Allocate and return a copy of `source` as a raw dynamic array (caller owns and must delete[]).
// Using a raw array here reduces vector overhead inside the hot SA loop.
int* copyPath(const int* source, std::size_t length) {
    if (source == nullptr || length == 0) {
        return nullptr;
    }

    int* copy = new int[length];
    std::copy_n(source, length, copy);
    return copy;
}

// Apply a single neighborhood move to `path` and return a newly allocated candidate array.
// The returned array is owned by the caller and must be deleted[] when no longer needed.
// Note: position 0 is kept fixed (distribution starts at 1) to keep the start vertex stable.
int* applyNeighborhoodRaw(const int* path,
                          std::size_t length,
                          SimulatedAnnealingAlgorithm::NeighborhoodType neighborhoodType,
                          std::mt19937& rng) {
    if (path == nullptr || length == 0) {
        return nullptr;
    }

    int* candidate = copyPath(path, length);
    if (length <= 2) {
        return candidate;
    }

    // Choose positions in [1, length-1] so vertex 0 remains fixed in the tour.
    std::uniform_int_distribution<std::size_t> positionDistribution(1, length - 1);

    switch (neighborhoodType) {
        case SimulatedAnnealingAlgorithm::NeighborhoodType::Swap: {
            const std::size_t first = positionDistribution(rng);
            std::size_t second = positionDistribution(rng);
            while (second == first) {
                second = positionDistribution(rng);
            }
            std::swap(candidate[first], candidate[second]);
            break;
        }
        case SimulatedAnnealingAlgorithm::NeighborhoodType::Insert: {
            const std::size_t from = positionDistribution(rng);
            const std::size_t to = positionDistribution(rng);
            if (from == to) {
                return candidate;
            }

            const int value = candidate[from];
            if (from < to) {
                for (std::size_t i = from; i + 1 < to; ++i) {
                    candidate[i] = candidate[i + 1];
                }
                candidate[to - 1] = value;
            } else {
                for (std::size_t i = from; i > to; --i) {
                    candidate[i] = candidate[i - 1];
                }
                candidate[to] = value;
            }
            break;
        }
        case SimulatedAnnealingAlgorithm::NeighborhoodType::Invert: {
            std::size_t left = positionDistribution(rng);
            std::size_t right = positionDistribution(rng);
            if (left > right) {
                std::swap(left, right);
            }
            if (left == right) {
                return candidate;
            }

            std::reverse(candidate + static_cast<std::ptrdiff_t>(left),
                         candidate + static_cast<std::ptrdiff_t>(right) + 1);
            break;
        }
    }

    return candidate;
}

}

SimulatedAnnealingAlgorithm::SimulatedAnnealingAlgorithm(InitialSolutionType initialSolutionType,
                                                         NeighborhoodType neighborhoodType,
                                                         CoolingSchedule coolingSchedule,
                                                         double initialTemperature,
                                                         double coolingRate,
                                                         int epochLength,
                                                         int maxIterations,
                                                         std::optional<std::uint32_t> seed)
    : initialSolutionType_(initialSolutionType),
      neighborhoodType_(neighborhoodType),
      coolingSchedule_(coolingSchedule),
      initialTemperature_(initialTemperature),
      coolingRate_(std::clamp(coolingRate, 0.000001, 0.999999)),
      epochLength_(epochLength),
      maxIterations_(maxIterations),
      rng_(seed.has_value() ? *seed : std::random_device{}()) {
}

Result SimulatedAnnealingAlgorithm::solve(const TSPData& data) {
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
        result.minCost = calculateCycleCost(data, result.bestPath);
        result.executionTimeMicroseconds = timer.elapsedMicroseconds();
        return result;
    }

    // Build an initial solution: prefer Nearest Neighbor when requested,
    // otherwise (or if it fails) fall back to a random permutation.
    std::vector<int> initialPath;
    if (initialSolutionType_ == InitialSolutionType::NearestNeighbor) {
        initialPath = buildNearestNeighborPath(data);
    }
    if (initialPath.empty() || initialPath.size() != data.size) {
        initialPath = buildRandomPath(data, rng_);
    }

    // Convert initial vector path to raw dynamic arrays for performance in the SA inner loop.
    // Ownership: these arrays must be freed with delete[] before returning.
    int* currentPath = copyPath(initialPath.data(), initialPath.size());
    int* bestPath = copyPath(currentPath, data.size);

    // Evaluate the initial tour cost and set it as the current and best known cost.
    int currentCost = calculateCycleCost(data, currentPath, data.size);
    int bestCost = currentCost;

    // Determine epoch length and overall max iterations (use defaults when parameters are non-positive).
    const int effectiveEpochLength = (epochLength_ > 0) ? epochLength_ : std::max(10, static_cast<int>(data.size) * 5);
    const int effectiveMaxIterations =
            (maxIterations_ > 0) ? maxIterations_ : std::max(1000, static_cast<int>(data.size) * static_cast<int>(data.size) * 20);

    // Initialize temperature: use provided value or estimate from instance statistics.
    double temperature = (initialTemperature_ > 0.0) ? initialTemperature_ : estimateInitialTemperature(data);
    const double minTemperature = 1e-6;
    // RNG for acceptance probability comparisons in (0,1).
    std::uniform_real_distribution<double> probability(0.0, 1.0);

    // Main SA loop: run until temperature is low or iteration budget exhausted.
    int iterations = 0;
    while (temperature > minTemperature && iterations < effectiveMaxIterations) {
        // Each epoch performs several neighborhood evaluations.
        for (int epochStep = 0; epochStep < effectiveEpochLength && iterations < effectiveMaxIterations; ++epochStep) {
            // Create a neighbor candidate (newly allocated raw array owned by caller).
            int* candidatePath = applyNeighborhoodRaw(currentPath, data.size, neighborhoodType_, rng_);
            // Compute candidate tour cost and difference to current.
            const int candidateCost = calculateCycleCost(data, candidatePath, data.size);
            const int delta = candidateCost - currentCost;

            // Acceptance rule: always accept improving moves; otherwise accept with Metropolis probability.
            bool acceptMove = delta <= 0;
            if (!acceptMove && temperature > 0.0) {
                const double acceptanceProbability = std::exp(-static_cast<double>(delta) / temperature);
                acceptMove = probability(rng_) < acceptanceProbability;
            }

            if (acceptMove) {
                // Accept: replace current solution (transfer ownership of arrays).
                delete[] currentPath;
                currentPath = candidatePath;
                currentCost = candidateCost;
                // Update best-so-far if improvement observed.
                if (currentCost < bestCost) {
                    int* newBestPath = copyPath(currentPath, data.size);
                    delete[] bestPath;
                    bestPath = newBestPath;
                    bestCost = currentCost;
                }
            } else {
                // Reject: free candidate and keep current.
                delete[] candidatePath;
            }

            ++iterations;
        }

        // Cooling schedule update between epochs.
        switch (coolingSchedule_) {
            case CoolingSchedule::Geometric:
                // Exponential decay.
                temperature *= coolingRate_;
                break;
            case CoolingSchedule::Linear:
                // Linear decrease targeting zero over the remaining iterations.
                temperature -= initialTemperature_ /
                               (static_cast<double>(effectiveMaxIterations) / static_cast<double>(effectiveEpochLength) + 1.0);
                break;
        }
    }

    if (bestPath != nullptr) {
        result.bestPath.assign(bestPath, bestPath + data.size);
    }
    result.minCost = bestCost;
    result.executionTimeMicroseconds = timer.elapsedMicroseconds();

    delete[] currentPath;
    delete[] bestPath;

    return result;
}

std::string SimulatedAnnealingAlgorithm::toString(InitialSolutionType type) {
    switch (type) {
        case InitialSolutionType::Random:
            return "Random";
        case InitialSolutionType::NearestNeighbor:
            return "NearestNeighbor";
    }
    return "Unknown";
}

std::string SimulatedAnnealingAlgorithm::toString(NeighborhoodType type) {
    switch (type) {
        case NeighborhoodType::Swap:
            return "Swap";
        case NeighborhoodType::Insert:
            return "Insert";
        case NeighborhoodType::Invert:
            return "Invert";
    }
    return "Unknown";
}

std::string SimulatedAnnealingAlgorithm::toString(CoolingSchedule type) {
    switch (type) {
        case CoolingSchedule::Geometric:
            return "Geometric";
        case CoolingSchedule::Linear:
            return "Linear";
    }
    return "Unknown";
}

std::vector<int> SimulatedAnnealingAlgorithm::buildRandomPath(const TSPData& data, std::mt19937& rng) {
    std::vector<int> path;
    path.reserve(data.size);
    path.push_back(0);

    std::vector<int> vertices;
    vertices.reserve(data.size > 0 ? data.size - 1 : 0);
    for (std::size_t vertex = 1; vertex < data.size; ++vertex) {
        vertices.push_back(static_cast<int>(vertex));
    }

    std::shuffle(vertices.begin(), vertices.end(), rng);
    path.insert(path.end(), vertices.begin(), vertices.end());
    return path;
}

std::vector<int> SimulatedAnnealingAlgorithm::buildNearestNeighborPath(const TSPData& data) {
    NearestNeighborAlgorithm algorithm;
    const Result result = algorithm.solve(data);
    return result.bestPath;
}


double SimulatedAnnealingAlgorithm::estimateInitialTemperature(const TSPData& data) {
    long long totalWeight = 0;
    long long edgeCount = 0;

    for (std::size_t i = 0; i < data.size; ++i) {
        for (std::size_t j = 0; j < data.size; ++j) {
            if (i == j) {
                continue;
            }
            totalWeight += data.distanceMatrix[i][j];
            ++edgeCount;
        }
    }

    if (edgeCount == 0) {
        return 1000.0;
    }

    const double averageWeight = static_cast<double>(totalWeight) / static_cast<double>(edgeCount);
    return std::max(1.0, averageWeight * static_cast<double>(data.size));
}