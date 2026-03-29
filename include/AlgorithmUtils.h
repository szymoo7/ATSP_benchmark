// AlgorithmUtils: utility class for calculating cycle cost
#pragma once

#include "TSPData.h"

#include <vector>

[[nodiscard]] int calculateCycleCost(const TSPData& data, const std::vector<int>& path);
