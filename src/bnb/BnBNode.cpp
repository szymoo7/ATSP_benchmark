#include "../../include/bnb/BnBNode.h"

#include <new>

namespace bnb {

// allocateMatrix: allocate a dense n x n matrix.
int** allocateMatrix(int n) {
    int** matrix = new (std::nothrow) int*[n];
    if (matrix == nullptr) {
        return nullptr;
    }

    for (int i = 0; i < n; ++i) {
        matrix[i] = new (std::nothrow) int[n];
        if (matrix[i] == nullptr) {
            for (int j = 0; j < i; ++j) {
                delete[] matrix[j];
            }
            delete[] matrix;
            return nullptr;
        }
    }
    return matrix;
}

// freeMatrix: release all matrix rows and the row-pointer array.
void freeMatrix(int** matrix, int n) {
    if (matrix == nullptr) {
        return;
    }
    for (int i = 0; i < n; ++i) {
        delete[] matrix[i];
    }
    delete[] matrix;
}

// createNode: allocate node with matrix, visited flags, and path buffers.
Node* createNode(int n) {
    Node* node = new (std::nothrow) Node();
    if (node == nullptr) {
        return nullptr;
    }

    node->n = n;
    node->reducedMatrix = allocateMatrix(n);
    node->visited = new (std::nothrow) bool[n];
    node->path = new (std::nothrow) int[n];

    if (node->reducedMatrix == nullptr || node->visited == nullptr || node->path == nullptr) {
        freeMatrix(node->reducedMatrix, n);
        delete[] node->visited;
        delete[] node->path;
        delete node;
        return nullptr;
    }

    for (int i = 0; i < n; ++i) {
        node->visited[i] = false;
        node->path[i] = -1;
    }

    return node;
}

// deleteNode: free all buffers owned by node.
void deleteNode(Node* node) {
    if (node == nullptr) {
        return;
    }
    freeMatrix(node->reducedMatrix, node->n);
    delete[] node->visited;
    delete[] node->path;
    delete node;
}

// copyMatrix: duplicate matrix content.
void copyMatrix(int** destination, int** source, int n) {
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            destination[i][j] = source[i][j];
        }
    }
}

// reduceMatrix: perform row and column reduction and return reduction sum.
long long reduceMatrix(int** matrix, int n) {
    long long reductionCost = 0;

    for (int i = 0; i < n; ++i) {
        int rowMin = kInfinity;
        for (int j = 0; j < n; ++j) {
            if (matrix[i][j] < rowMin) {
                rowMin = matrix[i][j];
            }
        }

        if (rowMin > 0 && rowMin < kInfinity) {
            for (int j = 0; j < n; ++j) {
                if (matrix[i][j] < kInfinity) {
                    matrix[i][j] -= rowMin;
                }
            }
            reductionCost += static_cast<long long>(rowMin);
        }
    }

    for (int j = 0; j < n; ++j) {
        int columnMin = kInfinity;
        for (int i = 0; i < n; ++i) {
            if (matrix[i][j] < columnMin) {
                columnMin = matrix[i][j];
            }
        }

        if (columnMin > 0 && columnMin < kInfinity) {
            for (int i = 0; i < n; ++i) {
                if (matrix[i][j] < kInfinity) {
                    matrix[i][j] -= columnMin;
                }
            }
            reductionCost += static_cast<long long>(columnMin);
        }
    }

    return reductionCost;
}

// cloneChildFromParent: apply one edge choice and compute child lower bound.
Node* cloneChildFromParent(const Node* parent, int nextCity, const TSPData& data) {
    Node* child = createNode(parent->n);
    if (child == nullptr) {
        return nullptr;
    }

    copyMatrix(child->reducedMatrix, parent->reducedMatrix, parent->n);
    for (int i = 0; i < parent->n; ++i) {
        child->visited[i] = parent->visited[i];
        child->path[i] = parent->path[i];
    }

    child->level = parent->level + 1;
    child->currentCity = nextCity;
    child->path[child->level] = nextCity;
    child->visited[nextCity] = true;

    child->costSoFar = parent->costSoFar + static_cast<long long>(data.distanceMatrix[parent->currentCity][nextCity]);

    // Disable leaving current city again and entering next city again.
    for (int c = 0; c < child->n; ++c) {
        child->reducedMatrix[parent->currentCity][c] = kInfinity;
    }
    for (int r = 0; r < child->n; ++r) {
        child->reducedMatrix[r][nextCity] = kInfinity;
    }

    // Block immediate return to start before all cities are visited.
    child->reducedMatrix[nextCity][0] = kInfinity;

    const int reducedEdge = parent->reducedMatrix[parent->currentCity][nextCity];
    const long long extraReduction = reduceMatrix(child->reducedMatrix, child->n);
    child->lowerBound = parent->lowerBound + static_cast<long long>(reducedEdge) + extraReduction;

    return child;
}

} // namespace bnb

