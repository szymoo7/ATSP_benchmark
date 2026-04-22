#include "../include/BranchAndBoundAlgorithm.h"

#include "../include/bnb/BnBContainers.h"
#include "../include/bnb/BnBNode.h"

#include <chrono>
#include <limits>
#include <new>

// solve: core Branch and Bound algorithm for ATSP.
// - Constructs search tree with selected traversal strategy (BFS, DFS, or Lowest-Cost).
// - Uses matrix reduction to compute lower bounds at each node.
// - Prunes subtrees where lower bound exceeds best known solution (upper bound).
// - Returns optimal or best-found tour with cost, path, and performance metrics.
Result BranchAndBoundAlgorithm::solve(const TSPData& data) {
    Result result;
    const auto startTime = std::chrono::steady_clock::now();

    // Validate input data
    if (!data.isLoaded() || data.size == 0) {
        result.minCost = 0;
        return result;
    }

    const int n = static_cast<int>(data.size);
    // Handle trivial single-node case
    if (n == 1) {
        const int singlePath[1] = {0};
        result.minCost = data.distanceMatrix[0][0];
        result.setRawPath(singlePath, 1);
        result.executionTimeMicroseconds =
                std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - startTime)
                        .count();
        return result;
    }

    // Create root node of search tree
    bnb::Node* root = bnb::createNode(n);
    if (root == nullptr) {
        result.minCost = std::numeric_limits<int>::max();
        result.executionTimeMicroseconds =
                std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - startTime)
                        .count();
        return result;
    }

    // Initialize root: copy distance matrix (diagonal = INF), mark city 0 as start
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            root->reducedMatrix[i][j] = (i == j) ? bnb::kInfinity : data.distanceMatrix[i][j];
        }
    }
    root->visited[0] = true;
    root->path[0] = 0;
    root->level = 0;
    root->currentCity = 0;
    root->costSoFar = 0;
    // Compute initial lower bound via matrix reduction
    root->lowerBound = bnb::reduceMatrix(root->reducedMatrix, n);

    // Prepare containers for frontier (choice depends on strategy)
    bnb::CustomStack stack;
    bnb::CustomQueue queue;
    bnb::CustomPriorityQueue priorityQueue;

    // Lambda: push node to frontier using selected strategy
    auto pushNode = [&](bnb::Node* node) -> bool {
        switch (strategy()) {
            case Strategy::DFS:
                return stack.push(node);
            case Strategy::BFS:
                return queue.push(node);
            case Strategy::LowestCost:
                return priorityQueue.push(node);
        }
        return false;
    };

    // Lambda: pop next node from frontier using selected strategy
    auto popNode = [&]() -> bnb::Node* {
        switch (strategy()) {
            case Strategy::DFS:
                return stack.pop();
            case Strategy::BFS:
                return queue.pop();
            case Strategy::LowestCost:
                return priorityQueue.pop();
        }
        return nullptr;
    };

    // Lambda: get current frontier size
    auto frontierSize = [&]() -> int {
        switch (strategy()) {
            case Strategy::DFS:
                return stack.size();
            case Strategy::BFS:
                return queue.size();
            case Strategy::LowestCost:
                return priorityQueue.size();
        }
        return 0;
    };

    // Lambda: check if frontier is empty
    auto frontierEmpty = [&]() -> bool {
        switch (strategy()) {
            case Strategy::DFS:
                return stack.isEmpty();
            case Strategy::BFS:
                return queue.isEmpty();
            case Strategy::LowestCost:
                return priorityQueue.isEmpty();
        }
        return true;
    };

    // Push root node to frontier
    if (!pushNode(root)) {
        bnb::deleteNode(root);
        result.minCost = std::numeric_limits<int>::max();
        result.executionTimeMicroseconds =
                std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - startTime)
                        .count();
        return result;
    }

    // Initialize metrics and upper bound
    result.maxNodesInMemory = 1;
    long long bestCost = std::numeric_limits<long long>::max();
    int* bestPathBuffer = new (std::nothrow) int[n];
    bool hasBestPath = false;

    // Main B&B loop: process frontier until empty or solution found
    while (!frontierEmpty()) {
        // Track maximum frontier size
        if (frontierSize() > result.maxNodesInMemory) {
            result.maxNodesInMemory = frontierSize();
        }

        // Pop next node from frontier
        bnb::Node* node = popNode();
        if (node == nullptr) {
            break;
        }
        ++result.visitedNodes;

        // Prune: skip if node's lower bound exceeds current best solution
        if (node->lowerBound >= bestCost) {
            bnb::deleteNode(node);
            continue;
        }

        // Check if complete tour found (all cities visited)
        if (node->level == n - 1) {
            const int returnEdge = data.distanceMatrix[node->currentCity][0];
            if (returnEdge < bnb::kInfinity) {
                const long long cycleCost = node->costSoFar + static_cast<long long>(returnEdge);
                // Update best cost and path if improvement found
                if (cycleCost < bestCost) {
                    bestCost = cycleCost;
                    hasBestPath = true;
                    if (bestPathBuffer != nullptr) {
                        for (int i = 0; i < n; ++i) {
                            bestPathBuffer[i] = node->path[i];
                        }
                    }
                }
            }
            bnb::deleteNode(node);
            continue;
        }

        // Generate child nodes: try all unvisited cities as next destination
        for (int nextCity = n - 1; nextCity >= 0; --nextCity) {
            // Skip already visited city
            if (node->visited[nextCity]) {
                continue;
            }
            // Skip infeasible edge
            if (node->reducedMatrix[node->currentCity][nextCity] >= bnb::kInfinity) {
                continue;
            }

            // Create child node with edge currentCity -> nextCity
            bnb::Node* child = bnb::cloneChildFromParent(node, nextCity, data);
            if (child == nullptr) {
                continue;
            }

            // Prune child: skip if its lower bound exceeds best solution
            if (child->lowerBound >= bestCost) {
                bnb::deleteNode(child);
                continue;
            }

            // Add child to frontier; clean up if push fails
            if (!pushNode(child)) {
                bnb::deleteNode(child);
            }
        }

        // Release current node (no longer needed)
        bnb::deleteNode(node);
    }

    // Assemble result from best found path
    if (hasBestPath && bestPathBuffer != nullptr && bestCost <= static_cast<long long>(std::numeric_limits<int>::max())) {
        result.minCost = static_cast<int>(bestCost);
        result.setRawPath(bestPathBuffer, static_cast<std::size_t>(n));
    } else if (!hasBestPath) {
        // No complete tour found
        result.minCost = std::numeric_limits<int>::max();
    }

    // Release path buffer
    delete[] bestPathBuffer;

    // Record execution time
    result.executionTimeMicroseconds =
            std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - startTime).count();

    return result;
}

