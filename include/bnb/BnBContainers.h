#pragma once

#include "bnb/BnBNode.h"

namespace bnb {

// CustomStack: LIFO container for DFS traversal.
class CustomStack {
public:
    CustomStack() = default;
    ~CustomStack();

    // Push node pointer to stack top.
    bool push(Node* node);
    // Pop node pointer from stack top.
    Node* pop();
    [[nodiscard]] bool isEmpty() const;
    [[nodiscard]] int size() const;

private:
    // Ensure internal dynamic array can store at least needed elements.
    bool ensureCapacity(int needed);

    Node** data_ = nullptr;
    int size_ = 0;
    int capacity_ = 0;
};

// CustomQueue: FIFO container for BFS traversal.
class CustomQueue {
public:
    CustomQueue() = default;
    ~CustomQueue();

    // Push node pointer to queue tail.
    bool push(Node* node);
    // Pop node pointer from queue head.
    Node* pop();
    [[nodiscard]] bool isEmpty() const;
    [[nodiscard]] int size() const;

private:
    // Ensure internal circular buffer capacity.
    bool ensureCapacity(int needed);

    Node** data_ = nullptr;
    int head_ = 0;
    int size_ = 0;
    int capacity_ = 0;
};

// CustomPriorityQueue: min-heap by lowerBound for Lowest-Cost traversal.
class CustomPriorityQueue {
public:
    CustomPriorityQueue() = default;
    ~CustomPriorityQueue();

    // Insert node pointer and restore heap order.
    bool push(Node* node);
    // Remove node pointer with minimal lower bound.
    Node* pop();
    [[nodiscard]] bool isEmpty() const;
    [[nodiscard]] int size() const;

private:
    // Ensure heap array capacity.
    bool ensureCapacity(int needed);

    Node** heap_ = nullptr;
    int size_ = 0;
    int capacity_ = 0;
};

} // namespace bnb

