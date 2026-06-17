#include "../include/GeneticAlgorithm.h"

#include "../include/AlgorithmUtils.h"
#include "../include/Timer.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <numeric>
#include <random>
#include <vector>

namespace {

constexpr int kTournamentSize = 3;

// clampRates: keep crossover and mutation rates inside [0, 1]
void clampRates(double& crossoverRate, double& mutationRate) {
    crossoverRate = std::clamp(crossoverRate, 0.0, 1.0);
    mutationRate = std::clamp(mutationRate, 0.0, 1.0);
}

// copyPath: allocate and return a copy of source path (caller must delete[])
int* copyPath(const int* source, std::size_t length) {
    if (source == nullptr || length == 0) {
        return nullptr;
    }

    int* copy = new int[length];
    std::copy_n(source, length, copy);
    return copy;
}

// copyInto: copy source path into an existing buffer
void copyInto(int* destination, const int* source, std::size_t length) {
    if (destination == nullptr || source == nullptr || length == 0) {
        return;
    }
    std::copy_n(source, length, destination);
}

// allocatePopulation: allocate populationSize tours of length tourLength
int** allocatePopulation(int populationSize, std::size_t tourLength) {
    int** population = new int*[static_cast<std::size_t>(populationSize)];
    for (int i = 0; i < populationSize; ++i) {
        population[static_cast<std::size_t>(i)] = new int[tourLength];
    }
    return population;
}

// freePopulation: release memory allocated by allocatePopulation
void freePopulation(int** population, int populationSize) {
    if (population == nullptr) {
        return;
    }

    for (int i = 0; i < populationSize; ++i) {
        delete[] population[static_cast<std::size_t>(i)];
    }
    delete[] population;
}

// buildRandomPath: create random tour with fixed start vertex 0
void buildRandomPath(int* path, std::size_t length, std::mt19937& rng) {
    if (path == nullptr || length == 0) {
        return;
    }

    path[0] = 0;
    if (length > 1) {
        std::iota(path + 1, path + static_cast<std::ptrdiff_t>(length), 1);
        std::shuffle(path + 1, path + static_cast<std::ptrdiff_t>(length), rng);
    }
}

// mutateSwap: swap two random cities; positions are chosen in [1, length-1]
void mutateSwap(int* path, std::size_t length, std::mt19937& rng) {
    if (path == nullptr || length <= 2) {
        return;
    }

    std::uniform_int_distribution<std::size_t> positionDistribution(1, length - 1);
    const std::size_t first = positionDistribution(rng);
    std::size_t second = positionDistribution(rng);
    while (second == first) {
        second = positionDistribution(rng);
    }
    std::swap(path[first], path[second]);
}

// mutateInversion: reverse a random segment; positions are chosen in [1, length-1]
void mutateInversion(int* path, std::size_t length, std::mt19937& rng) {
    if (path == nullptr || length <= 2) {
        return;
    }

    std::uniform_int_distribution<std::size_t> positionDistribution(1, length - 1);
    std::size_t left = positionDistribution(rng);
    std::size_t right = positionDistribution(rng);
    if (left > right) {
        std::swap(left, right);
    }
    if (left == right) {
        return;
    }

    std::reverse(path + static_cast<std::ptrdiff_t>(left),
                 path + static_cast<std::ptrdiff_t>(right) + 1);
}

// applyMutation: apply selected mutation operator to path
void applyMutation(int* path,
                   std::size_t length,
                   GeneticAlgorithm::MutationType mutationType,
                   std::mt19937& rng) {
    switch (mutationType) {
        case GeneticAlgorithm::MutationType::Swap:
            mutateSwap(path, length, rng);
            break;
        case GeneticAlgorithm::MutationType::Inversion:
            mutateInversion(path, length, rng);
            break;
        case GeneticAlgorithm::MutationType::Random: {
            std::uniform_int_distribution<int> mutationChoice(0, 1);
            if (mutationChoice(rng) == 0) {
                mutateSwap(path, length, rng);
            } else {
                mutateInversion(path, length, rng);
            }
            break;
        }
    }
}

// orderCrossover: OX crossover with fixed start vertex 0
void orderCrossover(int* child,
                    const int* parent1,
                    const int* parent2,
                    std::size_t length,
                    bool* used,
                    std::mt19937& rng) {
    if (child == nullptr || parent1 == nullptr || parent2 == nullptr || used == nullptr || length == 0) {
        return;
    }

    for (std::size_t i = 0; i < length; ++i) {
        child[i] = -1;
        used[i] = false;
    }
    child[0] = 0;
    used[0] = true;

    if (length <= 2) {
        if (length == 2) {
            child[1] = parent1[1];
        }
        return;
    }

    std::uniform_int_distribution<std::size_t> cutDistribution(1, length - 1);
    std::size_t cutA = cutDistribution(rng);
    std::size_t cutB = cutDistribution(rng);
    if (cutA > cutB) {
        std::swap(cutA, cutB);
    }

    for (std::size_t position = cutA; position <= cutB; ++position) {
        child[position] = parent1[position];
        used[static_cast<std::size_t>(child[position])] = true;
    }

    std::size_t fillPosition = (cutB + 1) % length;
    if (fillPosition == 0) {
        fillPosition = 1;
    }

    for (std::size_t scanPosition = (cutB + 1) % length; scanPosition != cutA;
         scanPosition = (scanPosition + 1) % length) {
        if (scanPosition == 0) {
            continue;
        }

        const int city = parent2[scanPosition];
        if (!used[static_cast<std::size_t>(city)]) {
            while (child[fillPosition] != -1) {
                fillPosition = (fillPosition + 1) % length;
                if (fillPosition == 0) {
                    fillPosition = 1;
                }
            }
            child[fillPosition] = city;
            used[static_cast<std::size_t>(city)] = true;
        }
    }

    for (std::size_t position = 1; position < length; ++position) {
        if (child[position] == -1) {
            for (int city = 1; city < static_cast<int>(length); ++city) {
                if (!used[static_cast<std::size_t>(city)]) {
                    child[position] = city;
                    used[static_cast<std::size_t>(city)] = true;
                    break;
                }
            }
        }
    }
}

// selectParentIndex: tournament selection of one parent index
int selectParentIndex(const int* costs, int populationSize, std::mt19937& rng) {
    std::uniform_int_distribution<int> indexDistribution(0, populationSize - 1);

    int bestIndex = indexDistribution(rng);
    int bestCost = costs[static_cast<std::size_t>(bestIndex)];

    for (int tournamentRound = 1; tournamentRound < kTournamentSize; ++tournamentRound) {
        const int candidateIndex = indexDistribution(rng);
        const int candidateCost = costs[static_cast<std::size_t>(candidateIndex)];
        if (candidateCost < bestCost) {
            bestIndex = candidateIndex;
            bestCost = candidateCost;
        }
    }

    return bestIndex;
}

// findBestIndex: return index of the lowest-cost individual
int findBestIndex(const int* costs, int populationSize) {
    int bestIndex = 0;
    int bestCost = costs[0];

    for (int i = 1; i < populationSize; ++i) {
        if (costs[static_cast<std::size_t>(i)] < bestCost) {
            bestCost = costs[static_cast<std::size_t>(i)];
            bestIndex = i;
        }
    }

    return bestIndex;
}

} // namespace

GeneticAlgorithm::GeneticAlgorithm(int populationSize,
                                   double crossoverRate,
                                   double mutationRate,
                                   int generations,
                                   MutationType mutationType,
                                   std::optional<std::uint32_t> seed)
    : populationSize_(std::max(2, populationSize)),
      crossoverRate_(crossoverRate),
      mutationRate_(mutationRate),
      generations_(generations),
      mutationType_(mutationType),
      rng_(seed.has_value() ? *seed : std::random_device{}()) {
    clampRates(crossoverRate_, mutationRate_);
}

// Runs GA for given data
Result GeneticAlgorithm::solve(const TSPData& data) {
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

    const std::size_t tourLength = data.size;
    const int populationSize = populationSize_;
    // generations == 0 => scale iteration count with problem size
    const int generationCount = generations_ > 0 ? generations_ : std::max(100, static_cast<int>(tourLength) * 20);

    // Double-buffered populations avoid per-generation allocations.
    int** popA = allocatePopulation(populationSize, tourLength);
    int** popB = allocatePopulation(populationSize, tourLength);
    int* costsA = new int[static_cast<std::size_t>(populationSize)];
    int* costsB = new int[static_cast<std::size_t>(populationSize)];
    int* child = new int[tourLength];
    bool* used = new bool[tourLength];

    int** population = popA;
    int** nextPopulation = popB;
    int* costs = costsA;
    int* nextCosts = costsB;

    for (int i = 0; i < populationSize; ++i) {
        buildRandomPath(population[static_cast<std::size_t>(i)], tourLength, rng_);
        costs[static_cast<std::size_t>(i)] =
                calculateCycleCost(data, population[static_cast<std::size_t>(i)], tourLength);
    }

    std::uniform_real_distribution<double> probabilityDistribution(0.0, 1.0);

    for (int generation = 0; generation < generationCount; ++generation) {
        // Elitism: keep the best individual unchanged in the next generation.
        const int eliteIndex = findBestIndex(costs, populationSize);
        copyInto(nextPopulation[0], population[static_cast<std::size_t>(eliteIndex)], tourLength);
        nextCosts[0] = costs[static_cast<std::size_t>(eliteIndex)];

        for (int individual = 1; individual < populationSize; ++individual) {
            const int firstParentIndex = selectParentIndex(costs, populationSize, rng_);
            int secondParentIndex = selectParentIndex(costs, populationSize, rng_);
            while (secondParentIndex == firstParentIndex) {
                secondParentIndex = selectParentIndex(costs, populationSize, rng_);
            }

            int* offspring = nextPopulation[static_cast<std::size_t>(individual)];
            if (probabilityDistribution(rng_) < crossoverRate_) {
                orderCrossover(child,
                               population[static_cast<std::size_t>(firstParentIndex)],
                               population[static_cast<std::size_t>(secondParentIndex)],
                               tourLength,
                               used,
                               rng_);
                copyInto(offspring, child, tourLength);
            } else {
                copyInto(offspring, population[static_cast<std::size_t>(firstParentIndex)], tourLength);
            }

            if (probabilityDistribution(rng_) < mutationRate_) {
                applyMutation(offspring, tourLength, mutationType_, rng_);
            }

            nextCosts[static_cast<std::size_t>(individual)] = calculateCycleCost(data, offspring, tourLength);
        }

        // Ping-pong swap between current and next generation buffers.
        std::swap(population, nextPopulation);
        std::swap(costs, nextCosts);
    }

    const int bestIndex = findBestIndex(costs, populationSize);
    int* bestRaw = copyPath(population[static_cast<std::size_t>(bestIndex)], tourLength);
    const int bestCost = costs[static_cast<std::size_t>(bestIndex)];

    result.executionTimeMicroseconds = timer.elapsedMicroseconds();
    if (bestRaw != nullptr) {
        result.bestPath.assign(bestRaw, bestRaw + tourLength);
    }
    result.minCost = bestCost;

    delete[] bestRaw;
    freePopulation(popA, populationSize);
    freePopulation(popB, populationSize);
    delete[] costsA;
    delete[] costsB;
    delete[] child;
    delete[] used;

    return result;
}

std::string GeneticAlgorithm::toString(MutationType type) {
    switch (type) {
        case MutationType::Swap:
            return "Swap";
        case MutationType::Inversion:
            return "Inversion";
        case MutationType::Random:
            return "Random";
    }
    return "Unknown";
}
