#pragma once

#include "TSPData.h"

namespace bnb {

constexpr int kInfinity = 1000000000;

// Node: state stored in the BnB search tree.
struct Node {
    int** reducedMatrix = nullptr;
    bool* visited = nullptr;
    int* path = nullptr;
    int n = 0;
    int level = 0;
    int currentCity = 0;
    long long costSoFar = 0;
    long long lowerBound = 0;
};

// Allocate n x n matrix, return nullptr on allocation failure.
int** allocateMatrix(int n);
// Release matrix allocated by allocateMatrix.
void freeMatrix(int** matrix, int n);
// Allocate and initialize a full BnB node.
Node* createNode(int n);
// Free all resources owned by node.
void deleteNode(Node* node);
// Copy full matrix content from source to destination.
void copyMatrix(int** destination, int** source, int n);
// Reduce rows and columns and return added reduction cost.
long long reduceMatrix(int** matrix, int n);
// Build child node by applying one move and recomputing bound.
Node* cloneChildFromParent(const Node* parent, int nextCity, const TSPData& data);

} // namespace bnb

