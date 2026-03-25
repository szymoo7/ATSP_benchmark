// TSPData: compact representation of a TSPLIB distance matrix using dynamic memory.
// Provides safe copy / move semantics and helper to create truncated instances.
#pragma once

#include <cstddef>
#include <stdexcept>
#include <vector>

struct TSPData {
    // size: number of vertices in the instance
    std::size_t size = 0;
    // distanceMatrix: pointer to an array of pointers (rows), each row is an int[]
    int** distanceMatrix = nullptr;

    // isLoaded: quick check whether data is present
    [[nodiscard]] bool isLoaded() const {
        return size > 0 && distanceMatrix != nullptr;
    }

    // getTruncatedData: return a new TSPData containing top-left newSize x newSize
    // matrix. Allocates new memory for the truncated instance.
    [[nodiscard]] TSPData getTruncatedData(int newSize) const {
        if (newSize < 2) {
            throw std::invalid_argument("newSize must be >= 2.");
        }
        if (newSize > static_cast<int>(size)) {
            throw std::invalid_argument("newSize cannot exceed current N.");
        }
        const auto truncatedSize = static_cast<std::size_t>(newSize);
        TSPData truncated;
        truncated.size = truncatedSize;
        truncated.distanceMatrix = new int*[truncatedSize];
        for (std::size_t i = 0; i < truncatedSize; ++i) {
            truncated.distanceMatrix[i] = new int[truncatedSize];
            for (std::size_t j = 0; j < truncatedSize; ++j) {
                truncated.distanceMatrix[i][j] = distanceMatrix[i][j];
            }
        }
        return truncated;
    }

    // Default constructor
    TSPData() = default;

    // Destructor
    ~TSPData() {
        if (distanceMatrix) {
            for (std::size_t i = 0; i < size; ++i) {
                delete[] distanceMatrix[i];
            }
            delete[] distanceMatrix;
        }
    }

    // Copy constructor
    TSPData(const TSPData& other) : size(other.size) {
        if (other.distanceMatrix) {
            distanceMatrix = new int*[size];
            for (std::size_t i = 0; i < size; ++i) {
                distanceMatrix[i] = new int[size];
                for (std::size_t j = 0; j < size; ++j) {
                    distanceMatrix[i][j] = other.distanceMatrix[i][j];
                }
            }
        } else {
            distanceMatrix = nullptr;
        }
    }

    // Copy assignment operator
    TSPData& operator=(const TSPData& other) {
        if (this == &other) return *this;
        // Free existing memory
        if (distanceMatrix) {
            for (std::size_t i = 0; i < size; ++i) {
                delete[] distanceMatrix[i];
            }
            delete[] distanceMatrix;
        }
        size = other.size;
        if (other.distanceMatrix) {
            distanceMatrix = new int*[size];
            for (std::size_t i = 0; i < size; ++i) {
                distanceMatrix[i] = new int[size];
                for (std::size_t j = 0; j < size; ++j) {
                    distanceMatrix[i][j] = other.distanceMatrix[i][j];
                }
            }
        } else {
            distanceMatrix = nullptr;
        }
        return *this;
    }

    // Move constructor
    TSPData(TSPData&& other) noexcept : size(other.size), distanceMatrix(other.distanceMatrix) {
        other.size = 0;
        other.distanceMatrix = nullptr;
    }

    // Move assignment operator
    TSPData& operator=(TSPData&& other) noexcept {
        if (this == &other) return *this;
        if (distanceMatrix) {
            for (std::size_t i = 0; i < size; ++i) {
                delete[] distanceMatrix[i];
            }
            delete[] distanceMatrix;
        }
        size = other.size;
        distanceMatrix = other.distanceMatrix;
        other.size = 0;
        other.distanceMatrix = nullptr;
        return *this;
    }
};
