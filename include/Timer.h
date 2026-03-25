#pragma once

#include <chrono>

// Timer: simple high-resolution timer for measuring execution time
class Timer {
public:
    void start();

    [[nodiscard]] long long elapsedMicroseconds() const;

private:
    using Clock = std::chrono::high_resolution_clock;

    bool running_ = false;
    Clock::time_point start_{};
};

