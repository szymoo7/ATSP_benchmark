#pragma once

#include "IAlgorithm.h"

#include <cstdint>
#include <optional>
#include <random>
#include <string>
#include <vector>

// SimulatedAnnealingAlgorithm: stochastic ATSP heuristic using a configurable cooling schedule.
class SimulatedAnnealingAlgorithm final : public IAlgorithm {
public:
    enum class InitialSolutionType {
        Random,
        NearestNeighbor
    };

    enum class NeighborhoodType {
        Swap,
        Insert,
        Invert
    };

    enum class CoolingSchedule {
        Geometric,
        Linear
    };

    SimulatedAnnealingAlgorithm(InitialSolutionType initialSolutionType = InitialSolutionType::Random,
                                NeighborhoodType neighborhoodType = NeighborhoodType::Swap,
                                CoolingSchedule coolingSchedule = CoolingSchedule::Geometric,
                                double initialTemperature = 1000.0,
                                double coolingRate = 0.95,
                                int epochLength = 0,
                                int maxIterations = 0,
                                std::optional<std::uint32_t> seed = std::nullopt);

    Result solve(const TSPData& data) override;

    [[nodiscard]] static std::string toString(InitialSolutionType type);
    [[nodiscard]] static std::string toString(NeighborhoodType type);
    [[nodiscard]] static std::string toString(CoolingSchedule type);

private:
    [[nodiscard]] static std::vector<int> buildRandomPath(const TSPData& data, std::mt19937& rng);
    [[nodiscard]] static std::vector<int> buildNearestNeighborPath(const TSPData& data);
    [[nodiscard]] static double estimateInitialTemperature(const TSPData& data);

    InitialSolutionType initialSolutionType_;
    NeighborhoodType neighborhoodType_;
    CoolingSchedule coolingSchedule_;
    double initialTemperature_;
    double coolingRate_;
    int epochLength_;
    int maxIterations_;
    std::mt19937 rng_;
};