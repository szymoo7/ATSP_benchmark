// BenchmarkBnB: fixed BnB benchmark runner for truncated ftv170 instances.

#include "../include/BenchmarkBnB.h"

#include "../include/BranchAndBoundBFSAlgorithm.h"
#include "../include/BranchAndBoundDFSAlgorithm.h"
#include "../include/BranchAndBoundLowestCostAlgorithm.h"
#include "../include/TSPLIBParser.h"

#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <stdexcept>

// loadBaseData: finds ftv170.atsp in common run directories and parses it.
TSPData BenchmarkBnB::loadBaseData() const {
    namespace fs = std::filesystem;
    const fs::path p1 = fs::current_path() / "data" / kBaseFileName;
    const fs::path p2 = fs::current_path() / ".." / "data" / kBaseFileName;
    const fs::path p3 = fs::current_path() / ".." / ".." / "data" / kBaseFileName;

    if (fs::exists(p1)) {
        return TSPLIBParser::parse(p1.string());
    }
    if (fs::exists(p2)) {
        return TSPLIBParser::parse(p2.string());
    }
    if (fs::exists(p3)) {
        return TSPLIBParser::parse(p3.string());
    }

    throw std::runtime_error("Cannot find ftv170.atsp in data directories.");
}

// run: executes all BnB suites and writes one CSV file.
void BenchmarkBnB::run() {
    std::cout << "\n[BenchmarkBnB] Loading base data: " << kBaseFileName << "\n";
    TSPData baseData;
    try {
        baseData = loadBaseData();
    } catch (const std::exception& ex) {
        std::cout << "[BenchmarkBnB] Error: " << ex.what() << "\n";
        return;
    }

    std::cout << "[BenchmarkBnB] Base size: N=" << baseData.size << "\n";

    std::vector<CsvRow> rows;
    runForBfs(baseData, rows);
    runForDfs(baseData, rows);
    runForLowestCost(baseData, rows);

    writeCsv(rows);
    std::cout << "[BenchmarkBnB] Done. Saved: bnb_benchmark_results.csv\n";
}

// runForBfs: predefined sizes for BFS strategy.
void BenchmarkBnB::runForBfs(const TSPData& baseData, std::vector<CsvRow>& rows) const {
    static constexpr int sizes[] = {6, 7, 8, 9, 10, 11, 12};
    runSuite("BFS", BranchAndBoundBFSAlgorithm{}, sizes, static_cast<int>(sizeof(sizes) / sizeof(sizes[0])), baseData, rows);
}

// runForDfs: predefined sizes for DFS strategy.
void BenchmarkBnB::runForDfs(const TSPData& baseData, std::vector<CsvRow>& rows) const {
    static constexpr int sizes[] = {10, 11, 12, 16, 20, 24, 27};
    runSuite("DFS", BranchAndBoundDFSAlgorithm{}, sizes, static_cast<int>(sizeof(sizes) / sizeof(sizes[0])), baseData, rows);
}

// runForLowestCost: predefined sizes for best-first strategy.
void BenchmarkBnB::runForLowestCost(const TSPData& baseData, std::vector<CsvRow>& rows) const {
    static constexpr int sizes[] = {10, 11, 12, 14, 16, 18, 21};
    runSuite("Lowest-Cost",
             BranchAndBoundLowestCostAlgorithm{},
             sizes,
             static_cast<int>(sizeof(sizes) / sizeof(sizes[0])),
             baseData,
             rows);
}

// runSuite: runs kTrialCount trials for each N and aggregates benchmark metrics.
template <typename TAlgorithm>
void BenchmarkBnB::runSuite(const char* algorithmName,
                            TAlgorithm algorithm,
                            const int* sizes,
                            int count,
                            const TSPData& baseData,
                            std::vector<CsvRow>& rows) const {
    for (int i = 0; i < count; ++i) {
        const int n = sizes[i];
        if (n > static_cast<int>(baseData.size)) {
            std::cout << "[BenchmarkBnB] Skipping " << algorithmName << " for N=" << n << " (base too small).\n";
            continue;
        }

        const TSPData truncated = baseData.getTruncatedData(n);

        long long totalUs = 0;
        long long totalMaxNodesInMemory = 0;
        Result lastResult;
        bool ok = true;

        for (int trial = 1; trial <= kTrialCount; ++trial) {
            std::cout << "Testing " << algorithmName << " for N=" << n << "... [" << trial << "/" << kTrialCount
                      << "]\n";

            const Result result = algorithm.solve(truncated);

            if (result.minCost < 0 || result.minCost == std::numeric_limits<int>::max()) {
                ok = false;
                std::cout << "[BenchmarkBnB] Trial failed for " << algorithmName << " N=" << n << "\n";
                break;
            }

            totalUs += result.executionTimeMicroseconds;
            totalMaxNodesInMemory += result.maxNodesInMemory;
            lastResult = result;
        }

        if (!ok) {
            continue;
        }

        CsvRow row;
        row.source = kSourceLabel;
        row.algorithm = algorithmName;
        row.n = n;
        row.averageTimeMicroseconds = totalUs / kTrialCount;
        row.optimumCost = lastResult.minCost;
        row.trialCount = kTrialCount;
        row.visitedNodes = lastResult.visitedNodes;
        row.maxNodesInMemory = totalMaxNodesInMemory / kTrialCount;
        rows.push_back(row);
    }
}

// writeCsv: exports aggregated rows using semicolon separator.
void BenchmarkBnB::writeCsv(const std::vector<CsvRow>& rows) const {
    std::ofstream out("bnb_benchmark_results.csv");
    if (!out) {
        std::cout << "[BenchmarkBnB] Cannot write CSV file.\n";
        return;
    }

    out << "Source;Algorithm;N;Avg_Time_us;Opt_Cost;Trial_Count;Visited_Nodes;max_nodes_in_mem\n";

    for (const CsvRow& row : rows) {
        out << row.source << ';' << row.algorithm << ';' << row.n << ';' << row.averageTimeMicroseconds << ';'
            << row.optimumCost << ';' << row.trialCount << ';' << row.visitedNodes << ';' << row.maxNodesInMemory
            << '\n';
    }
}

