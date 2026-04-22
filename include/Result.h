#pragma once

#include <cstddef>
#include <limits>
#include <utility>
#include <vector>

// Result: container returned by algorithms (best path, cost, execution time).
struct Result {
    std::vector<int> bestPath;

    // Raw path used by non-STL BnB path representation.
    int* rawBestPath = nullptr;
    std::size_t rawBestPathLength = 0;

    int minCost = std::numeric_limits<int>::max();
    long long executionTimeMicroseconds = 0;
    long long visitedNodes = 0;
    long long maxNodesInMemory = 0;

    Result() = default;

    // Deep-copy raw path to keep Result self-contained.
    Result(const Result& other)
        : bestPath(other.bestPath),
          rawBestPath(nullptr),
          rawBestPathLength(0),
          minCost(other.minCost),
          executionTimeMicroseconds(other.executionTimeMicroseconds),
          visitedNodes(other.visitedNodes),
          maxNodesInMemory(other.maxNodesInMemory) {
        setRawPath(other.rawBestPath, other.rawBestPathLength);
    }

    // Move ownership of raw path buffer.
    Result(Result&& other) noexcept
        : bestPath(std::move(other.bestPath)),
          rawBestPath(other.rawBestPath),
          rawBestPathLength(other.rawBestPathLength),
          minCost(other.minCost),
          executionTimeMicroseconds(other.executionTimeMicroseconds),
          visitedNodes(other.visitedNodes),
          maxNodesInMemory(other.maxNodesInMemory) {
        other.rawBestPath = nullptr;
        other.rawBestPathLength = 0;
    }

    // Copy-assign with deep copy of raw path buffer.
    Result& operator=(const Result& other) {
        if (this == &other) {
            return *this;
        }

        bestPath = other.bestPath;
        minCost = other.minCost;
        executionTimeMicroseconds = other.executionTimeMicroseconds;
        visitedNodes = other.visitedNodes;
        maxNodesInMemory = other.maxNodesInMemory;
        setRawPath(other.rawBestPath, other.rawBestPathLength);
        return *this;
    }

    // Move-assign and transfer raw path ownership.
    Result& operator=(Result&& other) noexcept {
        if (this == &other) {
            return *this;
        }

        clearRawPath();
        bestPath = std::move(other.bestPath);
        rawBestPath = other.rawBestPath;
        rawBestPathLength = other.rawBestPathLength;
        minCost = other.minCost;
        executionTimeMicroseconds = other.executionTimeMicroseconds;
        visitedNodes = other.visitedNodes;
        maxNodesInMemory = other.maxNodesInMemory;

        other.rawBestPath = nullptr;
        other.rawBestPathLength = 0;
        return *this;
    }

    ~Result() {
        clearRawPath();
    }

    // Replace raw path with a deep copy.
    void setRawPath(const int* path, std::size_t length) {
        clearRawPath();
        if (path == nullptr || length == 0) {
            return;
        }

        rawBestPath = new int[length];
        rawBestPathLength = length;
        for (std::size_t i = 0; i < length; ++i) {
            rawBestPath[i] = path[i];
        }
    }

private:
    // Free owned raw path buffer.
    void clearRawPath() {
        delete[] rawBestPath;
        rawBestPath = nullptr;
        rawBestPathLength = 0;
    }
};
