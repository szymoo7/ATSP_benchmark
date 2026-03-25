// AlgorithmUtils: utility functions for algorithms (cost calculation, etc.)
#pragma once

#include "TSPData.h"

#include <vector>

[[nodiscard]] int calculateCycleCost(const TSPData& data, const std::vector<int>& path);
