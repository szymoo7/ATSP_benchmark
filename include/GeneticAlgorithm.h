#pragma once

#include "IAlgorithm.h"

#include <cstdint>
#include <optional>
#include <random>
#include <string>

// GeneticAlgorithm: evolutionary heuristic for ATSP using order crossover and mutation operators.
class GeneticAlgorithm final : public IAlgorithm {
public:
    // MutationType: operators used when a mutation event is applied
    enum class MutationType {
        Swap,
        Inversion,
        Random
    };

    GeneticAlgorithm(int populationSize = 100,
                     double crossoverRate = 0.8,
                     double mutationRate = 0.1,
                     int generations = 0,
                     MutationType mutationType = MutationType::Random,
                     std::optional<std::uint32_t> seed = std::nullopt);

    Result solve(const TSPData& data) override;

    [[nodiscard]] static std::string toString(MutationType type);

private:
    int populationSize_;
    double crossoverRate_;
    double mutationRate_;
    int generations_;
    MutationType mutationType_;
    std::mt19937 rng_;
};
