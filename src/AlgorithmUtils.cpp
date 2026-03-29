#include "../include/AlgorithmUtils.h"

// calculateCycleCost: compute cost of a cycle given by path
int calculateCycleCost(const TSPData& data, const std::vector<int>& path) {
    if (path.empty()) {
        return 0;
    }

    long long cost = 0;
    // Sum edges along the path
    for (std::size_t i = 0; i + 1 < path.size(); ++i) {
        cost += data.distanceMatrix[path[i]][path[i + 1]];
    }
    // Add closing edge back to the first vertex
    cost += data.distanceMatrix[path.back()][path.front()];
    return static_cast<int>(cost);
}
