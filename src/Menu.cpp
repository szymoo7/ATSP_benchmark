// Menu: console UI for manual BnB runs and BnB benchmark automation.

#include "../include/Menu.h"

#include "../include/Benchmark.h"
#include "../include/BenchmarkBnB.h"
#include "../include/BranchAndBoundBFSAlgorithm.h"
#include "../include/BranchAndBoundDFSAlgorithm.h"
#include "../include/BranchAndBoundLowestCostAlgorithm.h"
#include "../include/BruteForceAlgorithm.h"
#include "../include/NearestNeighborAlgorithm.h"
#include "../include/RandomSearchAlgorithm.h"
#include "../include/RepetitiveNNAlgorithm.h"
#include "../include/TSPLIBParser.h"

#include <iostream>
#include <string>

void Menu::run() {
    while (true) {
        printMenu();
        const int option = readInt("Choose option: ");

        switch (option) {
            case 1:
                loadData();
                break;
            case 2:
                printMatrix();
                break;
            case 3: {
                runBruteForce();
                break;
            }
            case 4:
                runRandomSearch();
                break;
            case 5: {
                runNearestNeighbor();
                break;
            }
            case 6: {
                runRepetitiveNearestNeighbor();
                break;
            }
            case 7:
                runClassicBenchmark();
                break;
            case 8: {
                runBranchAndBoundBfs();
                break;
            }
            case 9: {
                runBranchAndBoundDfs();
                break;
            }
            case 10: {
                runBranchAndBoundLowestCost();
                break;
            }
            case 11:
                runBenchmarkBnB();
                break;
            case 0:
                std::cout << "Program terminated.\n";
                return;
            default:
                std::cout << "Invalid option.\n";
                break;
        }
    }
}

void Menu::printMenu() {
    std::cout << "\n=== ATSP Menu ===\n"
              << "1. Load data from TSPLIB file\n"
              << "2. Display distance matrix\n"
              << "3. Brute Force\n"
              << "4. Random Search (RAND)\n"
              << "5. Nearest Neighbor (NN)\n"
              << "6. Repetitive Nearest Neighbor (RNN)\n"
              << "7. Run intelligent Benchmark (Save to CSV)\n"
              << "8. Branch and Bound - BFS\n"
              << "9. Branch and Bound - DFS\n"
              << "10. Branch and Bound - Lowest-Cost\n"
              << "11. Run Branch and Bound benchmark (Save to CSV)\n"
              << "0. Exit\n";
}

// loadData: prompt for a file and load TSPLIB data into memory
void Menu::loadData() {
    std::cout << "Enter file path: ";
    std::string path;
    std::getline(std::cin >> std::ws, path);

    try {
        data_ = TSPLIBParser::parse(path);
        std::cout << "Data loaded. N = " << data_.size << "\n";
    } catch (const std::exception& ex) {
        std::cout << "Loading error: " << ex.what() << "\n";
    }
}

// printMatrix: print the currently loaded distance matrix
void Menu::printMatrix() const {
    if (!data_.isLoaded()) {
        std::cout << "No data loaded.\n";
        return;
    }

    std::cout << "Distance matrix (" << data_.size << "x" << data_.size << "):\n";
    for (std::size_t i = 0; i < data_.size; ++i) {
        for (std::size_t j = 0; j < data_.size; ++j) {
            std::cout << data_.distanceMatrix[i][j] << ' ';
        }
        std::cout << '\n';
    }
}

void Menu::runBranchAndBoundBfs() {
    if (!data_.isLoaded()) {
        std::cout << "Please load data first.\n";
        return;
    }

    const TSPData runData = selectDataForAlgorithmRun();
    BranchAndBoundBFSAlgorithm algorithm;
    runWithStrategy(algorithm, runData);
}

void Menu::runBranchAndBoundDfs() {
    if (!data_.isLoaded()) {
        std::cout << "Please load data first.\n";
        return;
    }

    const TSPData runData = selectDataForAlgorithmRun();
    BranchAndBoundDFSAlgorithm algorithm;
    runWithStrategy(algorithm, runData);
}

void Menu::runBranchAndBoundLowestCost() {
    if (!data_.isLoaded()) {
        std::cout << "Please load data first.\n";
        return;
    }

    const TSPData runData = selectDataForAlgorithmRun();
    BranchAndBoundLowestCostAlgorithm algorithm;
    runWithStrategy(algorithm, runData);
}

void Menu::runBruteForce() {
    if (!data_.isLoaded()) {
        std::cout << "Please load data first.\n";
        return;
    }

    const TSPData runData = selectDataForAlgorithmRun();
    BruteForceAlgorithm algorithm;
    const Result result = algorithm.solve(runData);
    printResult(result);
}

void Menu::runRandomSearch() {
    if (!data_.isLoaded()) {
        std::cout << "Please load data first.\n";
        return;
    }

    const TSPData runData = selectDataForAlgorithmRun();
    const long long timeMs = readLongLong("Enter RAND time limit [ms]: ");
    RandomSearchAlgorithm algorithm(timeMs);
    const Result result = algorithm.solve(runData);
    printResult(result);
}

void Menu::runNearestNeighbor() {
    if (!data_.isLoaded()) {
        std::cout << "Please load data first.\n";
        return;
    }

    const TSPData runData = selectDataForAlgorithmRun();
    NearestNeighborAlgorithm algorithm;
    const Result result = algorithm.solve(runData);
    printResult(result);
}

void Menu::runRepetitiveNearestNeighbor() {
    if (!data_.isLoaded()) {
        std::cout << "Please load data first.\n";
        return;
    }

    const TSPData runData = selectDataForAlgorithmRun();
    RepetitiveNNAlgorithm algorithm;
    const Result result = algorithm.solve(runData);
    printResult(result);
}

void Menu::runClassicBenchmark() {
    std::cout << "\n[Menu] Running option: Intelligent Benchmark (Save to CSV)\n";
    std::cout << "[Menu] The process will be performed automatically without further questions.\n";
    Benchmark benchmark;
    benchmark.runSmartBenchmark();
    std::cout << "[Menu] Benchmark finished. Returned to main menu.\n";
}

void Menu::runBenchmarkBnB() {
    BenchmarkBnB benchmark;
    benchmark.run();
}

// selectDataForAlgorithmRun: prompt user to choose a truncated size or full instance
TSPData Menu::selectDataForAlgorithmRun() const {
    while (true) {
        std::cout << "Current problem size: N = " << data_.size << "\n";
        const int selectedSize = readInt(
                "Enter N size to test (or type 0 to use the full matrix): ");

        if (selectedSize == 0 || selectedSize == static_cast<int>(data_.size)) {
            return data_;
        }

        try {
            return data_.getTruncatedData(selectedSize);
        } catch (const std::invalid_argument& ex) {
            std::cout << "Invalid N size: " << ex.what() << "\n";
        }
    }
}

void Menu::printResult(const Result& result) {
    if (result.rawBestPathLength == 0 && result.bestPath.empty()) {
        std::cout << "No path found.\n";
        std::cout << "Cost: " << result.minCost << "\n";
        std::cout << "Time [us]: " << result.executionTimeMicroseconds << "\n";
        std::cout << "Visited_Nodes: " << result.visitedNodes << "\n";
        std::cout << "max_nodes_in_mem: " << result.maxNodesInMemory << "\n";
        return;
    }

    std::cout << "Best path: ";
    if (result.rawBestPathLength > 0 && result.rawBestPath != nullptr) {
        for (std::size_t i = 0; i < result.rawBestPathLength; ++i) {
            std::cout << result.rawBestPath[i] << " -> ";
        }
        std::cout << result.rawBestPath[0] << '\n';
    } else {
        for (const int node : result.bestPath) {
            std::cout << node << " -> ";
        }
        std::cout << result.bestPath.front() << '\n';
    }

    std::cout << "Cost: " << result.minCost << '\n';
    std::cout << "Time [us]: " << result.executionTimeMicroseconds << '\n';
    std::cout << "Visited_Nodes: " << result.visitedNodes << '\n';
    std::cout << "max_nodes_in_mem: " << result.maxNodesInMemory << '\n';
}

void Menu::runWithStrategy(BranchAndBoundAlgorithm& algorithm, const TSPData& data) {
    const Result result = algorithm.solve(data);
    printResult(result);
}

// readInt / readLongLong: small utility input helpers
int Menu::readInt(const std::string& prompt) {
    while (true) {
        std::cout << prompt;
        std::string line;
        if (!std::getline(std::cin >> std::ws, line)) {
            return 0;
        }

        try {
            std::size_t pos = 0;
            const int value = std::stoi(line, &pos);
            if (pos == line.size()) {
                return value;
            }
        } catch (...) {
        }
        std::cout << "Please enter a valid integer.\n";
    }
}

long long Menu::readLongLong(const std::string& prompt) {
    while (true) {
        std::cout << prompt;
        std::string line;
        if (!std::getline(std::cin >> std::ws, line)) {
            return 1;
        }

        try {
            std::size_t pos = 0;
            const long long value = std::stoll(line, &pos);
            if (pos == line.size() && value > 0) {
                return value;
            }
        } catch (...) {
        }
        std::cout << "Please enter a positive integer.\n";
    }
}

