#pragma once

#include "TSPData.h"

#include <string>
#include <vector>

// BenchmarkBnB: fixed benchmark suite for BnB BFS/DFS/Lowest-Cost strategies.
class BenchmarkBnB {
public:
    // Run all configured suites and save CSV output.
    void run();

private:
    // CsvRow: one aggregated result line saved into CSV.
    struct CsvRow {
        std::string source;
        std::string algorithm;
        int n = 0;
        long long averageTimeMicroseconds = 0;
        int optimumCost = -1;
        int trialCount = 0;
        long long visitedNodes = 0;
        long long maxNodesInMemory = 0;
    };

    // Load the base ftv170 instance from known data locations.
    [[nodiscard]] TSPData loadBaseData() const;
    // Run BFS suite for predefined sizes.
    void runForBfs(const TSPData& baseData, std::vector<CsvRow>& rows) const;
    // Run DFS suite for predefined sizes.
    void runForDfs(const TSPData& baseData, std::vector<CsvRow>& rows) const;
    // Run Lowest-Cost suite for predefined sizes.
    void runForLowestCost(const TSPData& baseData, std::vector<CsvRow>& rows) const;

    template <typename TAlgorithm>
    // Run repeated trials for each size and append aggregated rows.
    void runSuite(const char* algorithmName,
                  TAlgorithm algorithm,
                  const int* sizes,
                  int count,
                  const TSPData& baseData,
                  std::vector<CsvRow>& rows) const;

    // Export benchmark rows to bnb_benchmark_results.csv.
    void writeCsv(const std::vector<CsvRow>& rows) const;

    static constexpr int kTrialCount = 5;
    static constexpr const char* kBaseFileName = "ftv170.atsp";
    static constexpr const char* kSourceLabel = "TRUNC(ftv170.atsp)";
};
