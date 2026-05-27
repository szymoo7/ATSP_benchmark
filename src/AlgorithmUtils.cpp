#include "../include/AlgorithmUtils.h"

#include <vector>

// calculateCycleCost: compute cost of a cycle given by a raw path buffer
int calculateCycleCost(const TSPData& data, const int* path, std::size_t length) {
    if (path == nullptr || length == 0) {
        return 0;
    }

    long long cost = 0;
    for (std::size_t i = 0; i + 1 < length; ++i) {
        cost += data.distanceMatrix[path[i]][path[i + 1]];
    }
    cost += data.distanceMatrix[path[length - 1]][path[0]];
    return static_cast<int>(cost);
}

// calculateCycleCost: compute cost of a cycle given by path
int calculateCycleCost(const TSPData& data, const std::vector<int>& path) {
    return calculateCycleCost(data, path.data(), path.size());
}

