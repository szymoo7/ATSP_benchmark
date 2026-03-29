#pragma once

#include "IAlgorithm.h"

#include <cstdint>
#include <optional>
#include <random>

// RandomSearchAlgorithm: stochastic search that samples random permutations within a given time
class RandomSearchAlgorithm final : public IAlgorithm {
public:
    explicit RandomSearchAlgorithm(long long timeLimitMs, std::optional<std::uint32_t> seed = std::nullopt);

    Result solve(const TSPData& data) override;

private:
    long long timeLimitMicroseconds_;
    std::mt19937 rng_;
};
