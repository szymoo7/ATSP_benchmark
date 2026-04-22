#include "../../include/bnb/BnBContainers.h"

#include <new>

namespace bnb {

// Destroy only internal pointer storage; nodes are owned by the solver.
CustomStack::~CustomStack() {
    delete[] data_;
}

bool CustomStack::push(Node* node) {
    if (!ensureCapacity(size_ + 1)) {
        return false;
    }
    data_[size_] = node;
    ++size_;
    return true;
}

Node* CustomStack::pop() {
    if (size_ == 0) {
        return nullptr;
    }
    --size_;
    return data_[size_];
}

bool CustomStack::isEmpty() const {
    return size_ == 0;
}

int CustomStack::size() const {
    return size_;
}

// Grow stack backing array exponentially.
bool CustomStack::ensureCapacity(int needed) {
    if (capacity_ >= needed) {
        return true;
    }
    int newCapacity = (capacity_ == 0) ? 32 : capacity_ * 2;
    while (newCapacity < needed) {
        newCapacity *= 2;
    }

    Node** newData = new (std::nothrow) Node*[newCapacity];
    if (newData == nullptr) {
        return false;
    }

    for (int i = 0; i < size_; ++i) {
        newData[i] = data_[i];
    }

    delete[] data_;
    data_ = newData;
    capacity_ = newCapacity;
    return true;
}

// Destroy only internal pointer storage; nodes are owned by the solver.
CustomQueue::~CustomQueue() {
    delete[] data_;
}

bool CustomQueue::push(Node* node) {
    if (!ensureCapacity(size_ + 1)) {
        return false;
    }

    data_[(head_ + size_) % capacity_] = node;
    ++size_;
    return true;
}

Node* CustomQueue::pop() {
    if (size_ == 0) {
        return nullptr;
    }
    Node* node = data_[head_];
    head_ = (head_ + 1) % capacity_;
    --size_;
    return node;
}

bool CustomQueue::isEmpty() const {
    return size_ == 0;
}

int CustomQueue::size() const {
    return size_;
}

// Grow queue buffer and re-pack circular order from head to tail.
bool CustomQueue::ensureCapacity(int needed) {
    if (capacity_ >= needed) {
        return true;
    }

    int newCapacity = (capacity_ == 0) ? 32 : capacity_ * 2;
    while (newCapacity < needed) {
        newCapacity *= 2;
    }

    Node** newData = new (std::nothrow) Node*[newCapacity];
    if (newData == nullptr) {
        return false;
    }

    for (int i = 0; i < size_; ++i) {
        newData[i] = data_[(head_ + i) % capacity_];
    }

    delete[] data_;
    data_ = newData;
    capacity_ = newCapacity;
    head_ = 0;
    return true;
}

// Destroy only internal pointer storage; nodes are owned by the solver.
CustomPriorityQueue::~CustomPriorityQueue() {
    delete[] heap_;
}

bool CustomPriorityQueue::push(Node* node) {
    if (!ensureCapacity(size_ + 1)) {
        return false;
    }

    int index = size_;
    heap_[index] = node;
    ++size_;

    // Bubble-up to keep min-heap by lowerBound.
    while (index > 0) {
        const int parent = (index - 1) / 2;
        if (heap_[parent]->lowerBound <= heap_[index]->lowerBound) {
            break;
        }
        Node* tmp = heap_[parent];
        heap_[parent] = heap_[index];
        heap_[index] = tmp;
        index = parent;
    }

    return true;
}

Node* CustomPriorityQueue::pop() {
    if (size_ == 0) {
        return nullptr;
    }

    Node* top = heap_[0];
    --size_;
    if (size_ == 0) {
        return top;
    }

    heap_[0] = heap_[size_];

    int index = 0;
    // Bubble-down to restore min-heap order.
    while (true) {
        const int left = 2 * index + 1;
        const int right = 2 * index + 2;
        int smallest = index;

        if (left < size_ && heap_[left]->lowerBound < heap_[smallest]->lowerBound) {
            smallest = left;
        }
        if (right < size_ && heap_[right]->lowerBound < heap_[smallest]->lowerBound) {
            smallest = right;
        }
        if (smallest == index) {
            break;
        }

        Node* tmp = heap_[index];
        heap_[index] = heap_[smallest];
        heap_[smallest] = tmp;
        index = smallest;
    }

    return top;
}

bool CustomPriorityQueue::isEmpty() const {
    return size_ == 0;
}

int CustomPriorityQueue::size() const {
    return size_;
}

// Grow heap backing array exponentially.
bool CustomPriorityQueue::ensureCapacity(int needed) {
    if (capacity_ >= needed) {
        return true;
    }

    int newCapacity = (capacity_ == 0) ? 32 : capacity_ * 2;
    while (newCapacity < needed) {
        newCapacity *= 2;
    }

    Node** newHeap = new (std::nothrow) Node*[newCapacity];
    if (newHeap == nullptr) {
        return false;
    }

    for (int i = 0; i < size_; ++i) {
        newHeap[i] = heap_[i];
    }

    delete[] heap_;
    heap_ = newHeap;
    capacity_ = newCapacity;
    return true;
}

} // namespace bnb

