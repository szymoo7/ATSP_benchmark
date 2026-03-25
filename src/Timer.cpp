#include "../include/Timer.h"

// Timer implementation: start and elapsed microseconds
void Timer::start() {
    running_ = true;
    start_ = Clock::now();
}

long long Timer::elapsedMicroseconds() const {
    if (!running_) {
        return 0;
    }
    const auto now = Clock::now();
    return std::chrono::duration_cast<std::chrono::microseconds>(now - start_).count();
}

