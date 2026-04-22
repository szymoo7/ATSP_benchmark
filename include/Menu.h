// Menu: simple command-line UI for loading data and running algorithms
#pragma once

#include "BranchAndBoundAlgorithm.h"
#include "Result.h"
#include "TSPData.h"

#include <iostream>
#include <string>

// Class for menu
class Menu {
public:
    void run();

private:
    static void printMenu();

    void loadData();
    void printMatrix() const;
    void runBruteForce();
    void runRandomSearch();
    void runNearestNeighbor();
    void runRepetitiveNearestNeighbor();
    void runClassicBenchmark();
    void runBranchAndBoundBfs();
    void runBranchAndBoundDfs();
    void runBranchAndBoundLowestCost();
    void runBenchmarkBnB();
    [[nodiscard]] TSPData selectDataForAlgorithmRun() const;

    static void runWithStrategy(BranchAndBoundAlgorithm& algorithm, const TSPData& data);

    // Helper functions for parsing
    static void printResult(const Result& result);
    [[nodiscard]] static int readInt(const std::string& prompt);
    [[nodiscard]] static long long readLongLong(const std::string& prompt);

    TSPData data_;
};

