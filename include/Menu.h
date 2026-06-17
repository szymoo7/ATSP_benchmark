// Menu: simple command-line UI for loading data and running algorithms
#pragma once

#include "BenchmarkSimulatedAnnealing.h"
#include "BranchAndBoundAlgorithm.h"
#include "GABenchmark.h"
#include "GeneticAlgorithm.h"
#include "Result.h"
#include "SimulatedAnnealingAlgorithm.h"
#include "TSPData.h"

#include <iostream>
#include <cstdint>
#include <optional>
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
    void runSimulatedAnnealingMenu();
    void runSimulatedAnnealingSingleRun();
    void runSimulatedAnnealingBenchmark();
    void runGeneticAlgorithmMenu();
    void runGeneticAlgorithmSingleRun();
    void runGeneticAlgorithmBenchmark();
    [[nodiscard]] TSPData selectDataForAlgorithmRun() const;

    [[nodiscard]] static SimulatedAnnealingAlgorithm::InitialSolutionType readInitialSolutionType();
    [[nodiscard]] static SimulatedAnnealingAlgorithm::NeighborhoodType readNeighborhoodType();
    [[nodiscard]] static GeneticAlgorithm::MutationType readGeneticAlgorithmMutationType();
    [[nodiscard]] static std::optional<std::uint32_t> readOptionalSeed();

    static void runWithStrategy(BranchAndBoundAlgorithm& algorithm, const TSPData& data);

    // Helper functions for parsing
    static void printResult(const Result& result);
    [[nodiscard]] static int readInt(const std::string& prompt);
    [[nodiscard]] static int readIntWithDefault(const std::string& prompt, int defaultValue);
    [[nodiscard]] static long long readLongLong(const std::string& prompt);
    [[nodiscard]] static double readDouble(const std::string& prompt);
    [[nodiscard]] static double readDoubleWithDefault(const std::string& prompt, double defaultValue);

    TSPData data_;
};

