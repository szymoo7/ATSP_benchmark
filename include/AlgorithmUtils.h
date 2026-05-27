// AlgorithmUtils: utility class for calculating cycle cost
#pragma once

#include "TSPData.h"

#include <cstddef>
#include <vector>

[[nodiscard]] int calculateCycleCost(const TSPData& data, const std::vector<int>& path);
[[nodiscard]] int calculateCycleCost(const TSPData& data, const int* path, std::size_t length);
