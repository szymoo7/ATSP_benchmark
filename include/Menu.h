// Menu: simple command-line UI for loading data and running algorithms
#pragma once

#include "Result.h"
#include "TSPData.h"

#include <iostream>
#include <string>

class Menu {
public:
    void run();

private:
    static void printMenu();

    void loadData();
    void printMatrix() const;
    void runRandomSearch();
    void runIntelligentBenchmark();
    [[nodiscard]] TSPData selectDataForAlgorithmRun() const;

    template <typename TAlgorithm>
    void runAlgorithm(TAlgorithm algorithm, const TSPData& runData) {
        if (!data_.isLoaded()) {
            std::cout << "Load data first.\n";
            return;
        }

        const Result result = algorithm.solve(runData);
        printResult(result);
    }

    static void printResult(const Result& result);
    [[nodiscard]] static int readInt(const std::string& prompt);
    [[nodiscard]] static long long readLongLong(const std::string& prompt);

    TSPData data_;
};

