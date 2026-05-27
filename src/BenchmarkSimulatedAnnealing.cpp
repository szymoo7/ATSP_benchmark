#include "../include/BenchmarkSimulatedAnnealing.h"

#include "../include/TSPLIBParser.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>

namespace {
const std::map<std::string, long long> kDatasetCatalog = {
    {"br17.atsp", 39},
    {"ftv33.atsp", 1286},
    {"ftv35.atsp", 1473},
    {"ftv38.atsp", 1530},
    {"p43.atsp", 5620},
    {"ftv44.atsp", 1613},
    {"ftv47.atsp", 1776},
    {"ry48p.atsp", 14422},
    {"ft53.atsp", 6905},
    {"ftv55.atsp", 1608},
    {"ftv64.atsp", 1839},
    {"ft70.atsp", 38673},
    {"ftv70.atsp", 1950},
    {"kro124p.atsp", 36230},
    {"ftv170.atsp", 2755},
};

std::uint32_t mixSeed(std::uint32_t seed, std::uint32_t value) {
    seed ^= value + 0x9e3779b9u + (seed << 6U) + (seed >> 2U);
    return seed;
}

std::uint32_t seedFromText(const std::string& text) {
    std::uint32_t seed = 2166136261u;
    for (const unsigned char ch : text) {
        seed ^= ch;
        seed *= 16777619u;
    }
    return seed;
}
} // namespace

void BenchmarkSimulatedAnnealing::run() {
    std::cout << "\n[SW-Benchmark] Loading TSPLIB datasets from data directory\n";

    const std::vector<DatasetRecord> datasets = loadDatasets();
    if (datasets.empty()) {
        std::cout << "[SW-Benchmark] No valid .atsp datasets found.\n";
        return;
    }

    std::cout << "[SW-Benchmark] Loaded " << datasets.size() << " dataset files.\n";

    std::vector<BenchmarkRow> rows;
    runExperiment1(datasets, rows);
    runExperiment2(datasets, rows);
    runExperiment3(datasets, rows);

    writeCsv(rows);
    std::cout << "[SW-Benchmark] Done. Saved: sa_benchmark_results.csv\n";
}

std::vector<BenchmarkSimulatedAnnealing::DatasetRecord> BenchmarkSimulatedAnnealing::loadDatasets() const {
    std::vector<DatasetRecord> datasets;
    const auto dataDirectory = resolveDataDirectory();
    if (!dataDirectory.has_value()) {
        return datasets;
    }

    namespace fs = std::filesystem;
    for (const auto& entry : fs::directory_iterator(*dataDirectory)) {
        if (!entry.is_regular_file()) {
            continue;
        }

        const fs::path filePath = entry.path();
        if (filePath.extension() != ".atsp") {
            continue;
        }

        try {
            DatasetRecord record;
            record.filePath = filePath.string();
            record.fileName = filePath.filename().string();
            record.data = TSPLIBParser::parse(record.filePath);
            record.size = static_cast<int>(record.data.size);
            const auto itOptimum = kDatasetCatalog.find(record.fileName);
            record.optimum = (itOptimum != kDatasetCatalog.end()) ? itOptimum->second : -1;
            datasets.push_back(std::move(record));
        } catch (const std::exception& ex) {
            std::cout << "[SW-Benchmark] Skipping " << filePath.filename().string() << ": " << ex.what() << "\n";
        }
    }

    std::sort(datasets.begin(), datasets.end(), [](const DatasetRecord& lhs, const DatasetRecord& rhs) {
        if (lhs.size != rhs.size) {
            return lhs.size < rhs.size;
        }
        return lhs.fileName < rhs.fileName;
    });

    return datasets;
}

std::optional<std::filesystem::path> BenchmarkSimulatedAnnealing::resolveDataDirectory() const {
    namespace fs = std::filesystem;

    const std::vector<fs::path> candidates = {
            fs::current_path() / "data",
            fs::current_path() / ".." / "data",
            fs::current_path() / ".." / ".." / "data",
    };

    for (const auto& candidate : candidates) {
        if (fs::exists(candidate)) {
            return fs::weakly_canonical(candidate);
        }
    }

    return std::nullopt;
}

// If targetSize is small (<=14), try to load the large `ftv170.atsp` instance and
// build a truncated TSP instance of the requested size. Returns a DatasetRecord
// with `rec.data` set to the truncated TSP and `rec.optimum` filled from the
// embedded BruteForce reference map when available. Returns std::nullopt on any
// failure so caller can fall back to the normal dataset selection.
std::optional<BenchmarkSimulatedAnnealing::DatasetRecord> BenchmarkSimulatedAnnealing::selectTruncatedFt170IfApplicable(
        int targetSize) const {
    if (targetSize > 14) {
        return std::nullopt;
    }

    const auto dataDir = resolveDataDirectory();
    if (!dataDir.has_value()) {
        return std::nullopt;
    }

    namespace fs = std::filesystem;
    const fs::path basePath = (*dataDir) / "ftv170.atsp";
    if (!fs::exists(basePath)) {
        return std::nullopt;
    }

    try {
        TSPData base = TSPLIBParser::parse(basePath.string());
        TSPData truncated = base.getTruncatedData(targetSize);
        DatasetRecord rec;
        rec.fileName = "TRUNC_" + std::to_string(targetSize) + "_ftv170.atsp";
        rec.filePath = basePath.string();
        rec.size = targetSize;

        static const std::map<int, long long> kBfReference = {
            {8, 188}, {9, 188}, {10, 188}, {11, 361}, {12, 383}, {13, 389}, {14, 411}
        };
        const auto itRef = kBfReference.find(targetSize);
        rec.optimum = (itRef != kBfReference.end()) ? itRef->second : -1;
        rec.data = std::move(truncated);
        return rec;
    } catch (const std::exception& ex) {
        std::cout << "[SW-Benchmark] Warning: failed to load/trim ftv170.atsp: " << ex.what() << "\n";
        return std::nullopt;
    }
}

// Picks closest dataset instance for given target
const BenchmarkSimulatedAnnealing::DatasetRecord* BenchmarkSimulatedAnnealing::pickNearestDataset(
        int targetSize,
        const std::vector<DatasetRecord>& datasets) const {
    if (datasets.empty()) {
        return nullptr;
    }

    const DatasetRecord* bestRecord = &datasets.front();
    int bestDistance = std::abs(targetSize - bestRecord->size);

    for (const DatasetRecord& record : datasets) {
        const int distance = std::abs(targetSize - record.size);
        if (distance < bestDistance) {
            bestDistance = distance;
            bestRecord = &record;
            continue;
        }

        if (distance == bestDistance && record.size < bestRecord->size) {
            bestRecord = &record;
        }
    }

    return bestRecord;
}

// Runs trials tests
BenchmarkSimulatedAnnealing::AggregateResult BenchmarkSimulatedAnnealing::runTrials(
        const DatasetRecord& dataset,
        SimulatedAnnealingAlgorithm::InitialSolutionType initialSolution,
        SimulatedAnnealingAlgorithm::NeighborhoodType neighborhoodType,
        SimulatedAnnealingAlgorithm::CoolingSchedule schedule,
        double initialTemperature,
        double coolingRate,
        int epochLength,
        int maxIterations) const {
    long long totalTime = 0;
    double totalCost = 0.0;
    long long bestCost = std::numeric_limits<long long>::max();

    const std::string seedTag = dataset.fileName + "|" + SimulatedAnnealingAlgorithm::toString(initialSolution) +
                                "|" + SimulatedAnnealingAlgorithm::toString(neighborhoodType) + "|" +
                                SimulatedAnnealingAlgorithm::toString(schedule) + "|" +
                                std::to_string(epochLength) + "|" + std::to_string(maxIterations);

    for (int trial = 0; trial < kTrialCount; ++trial) {
        const std::uint32_t baseSeed = seedFromText(seedTag);
        const std::uint32_t trialSeed = mixSeed(baseSeed, static_cast<std::uint32_t>(trial + 1));

        SimulatedAnnealingAlgorithm algorithm(initialSolution,
                                              neighborhoodType,
                                              schedule,
                                              initialTemperature,
                                              coolingRate,
                                              epochLength,
                                              maxIterations,
                                              trialSeed);
        const Result result = algorithm.solve(dataset.data);

        totalTime += result.executionTimeMicroseconds;
        totalCost += static_cast<double>(result.minCost);
        bestCost = std::min(bestCost, static_cast<long long>(result.minCost));
    }

    AggregateResult aggregate;
    aggregate.averageTimeMicroseconds = totalTime / kTrialCount;
    aggregate.averageCost = totalCost / static_cast<double>(kTrialCount);
    aggregate.bestCost = (bestCost == std::numeric_limits<long long>::max()) ? -1 : bestCost;
    return aggregate;
}

double BenchmarkSimulatedAnnealing::computeRelativeErrorPercent(double measured, long long optimum) const {
    if (optimum <= 0) {
        return 0.0;
    }
    return ((measured - static_cast<double>(optimum)) / static_cast<double>(optimum)) * 100.0;
}

std::string BenchmarkSimulatedAnnealing::makeParametersString(
        double initialTemperature,
        double coolingRate,
        int epochLength,
        int maxIterations,
        SimulatedAnnealingAlgorithm::CoolingSchedule schedule) {
    return "T0=" + std::to_string(initialTemperature) + ",coolingRate=" + std::to_string(coolingRate) +
           ",epoch=" + std::to_string(epochLength) + ",maxIter=" + std::to_string(maxIterations) +
           ",schedule=" + SimulatedAnnealingAlgorithm::toString(schedule);
}

void BenchmarkSimulatedAnnealing::runExperiment1(const std::vector<DatasetRecord>& datasets,
                                                 std::vector<BenchmarkRow>& rows) const {
    std::cout << "[SW-Benchmark] Experiment 1: initial solution vs instance size\n";

    const std::vector<int> targetSizes = {8, 9, 12, 45, 71, 100, 171};
    for (const int targetSize : targetSizes) {
        const DatasetRecord* dataset = nullptr;
        // For very small target sizes try to use a truncated version of ftv170.atsp
        // (provides a small instance with known BF reference cost). Fall back to
        // nearest dataset if not available.
        std::optional<DatasetRecord> localTruncated = selectTruncatedFt170IfApplicable(targetSize);
        if (localTruncated.has_value()) {
            dataset = &(*localTruncated);
        } else {
            dataset = pickNearestDataset(targetSize, datasets);
        }

        if (dataset == nullptr) {
            continue;
        }

        const std::vector<std::pair<SimulatedAnnealingAlgorithm::InitialSolutionType, std::string>> solutions = {
                {SimulatedAnnealingAlgorithm::InitialSolutionType::Random, "Random"},
                {SimulatedAnnealingAlgorithm::InitialSolutionType::NearestNeighbor, "NearestNeighbor"},
        };

        for (const auto& [initialSolution, label] : solutions) {
            const int epochLength = std::max(10, dataset->size * 5);
            const int maxIterations = std::max(1000, dataset->size * dataset->size * 20);

            // ZMIANA: Przekazujemy 0.0, aby wywołać automatyczną estymację temperatury startowej
            const AggregateResult aggregate = runTrials(*dataset,
                                                        initialSolution,
                                                        SimulatedAnnealingAlgorithm::NeighborhoodType::Swap,
                                                        SimulatedAnnealingAlgorithm::CoolingSchedule::Geometric,
                                                        0.0,
                                                        0.95,
                                                        epochLength,
                                                        maxIterations);

            rows.push_back({dataset->fileName,
                            "SW",
                            dataset->size,
                            aggregate.averageTimeMicroseconds,
                            aggregate.averageCost,
                            aggregate.bestCost,
                            kTrialCount,
                            computeRelativeErrorPercent(aggregate.averageCost, dataset->optimum),
                            label,
                            "Swap",
                            makeParametersString(0.0, 0.95, epochLength, maxIterations,
                                                 SimulatedAnnealingAlgorithm::CoolingSchedule::Geometric)});
        }
    }
}

void BenchmarkSimulatedAnnealing::runExperiment2(const std::vector<DatasetRecord>& datasets,
                                                 std::vector<BenchmarkRow>& rows) const {
    std::cout << "[SW-Benchmark] Experiment 2: neighborhood type\n";

    const std::vector<int> targetSizes = {45, 100, 171};
    const std::vector<SimulatedAnnealingAlgorithm::NeighborhoodType> neighborhoods = {
            SimulatedAnnealingAlgorithm::NeighborhoodType::Swap,
            SimulatedAnnealingAlgorithm::NeighborhoodType::Insert,
            SimulatedAnnealingAlgorithm::NeighborhoodType::Invert,
    };

    for (const int targetSize : targetSizes) {
        const DatasetRecord* dataset = pickNearestDataset(targetSize, datasets);
        if (dataset == nullptr) {
            continue;
        }

        const int epochLength = std::max(10, dataset->size * 5);
        const int maxIterations = std::max(1000, dataset->size * dataset->size * 20);

        for (const auto neighborhoodType : neighborhoods) {

            // ZMIANA: Przekazujemy 0.0, aby temperatura była szacowana dynamicznie i zbieżnie z Exp 1
            const AggregateResult aggregate = runTrials(*dataset,
                                                        SimulatedAnnealingAlgorithm::InitialSolutionType::NearestNeighbor,
                                                        neighborhoodType,
                                                        SimulatedAnnealingAlgorithm::CoolingSchedule::Geometric,
                                                        0.0,
                                                        0.95,
                                                        epochLength,
                                                        maxIterations);

            rows.push_back({dataset->fileName,
                            "SW",
                            dataset->size,
                            aggregate.averageTimeMicroseconds,
                            aggregate.averageCost,
                            aggregate.bestCost,
                            kTrialCount,
                            computeRelativeErrorPercent(aggregate.averageCost, dataset->optimum),
                            "NearestNeighbor",
                            SimulatedAnnealingAlgorithm::toString(neighborhoodType),
                            makeParametersString(0.0, 0.95, epochLength, maxIterations,
                                                 SimulatedAnnealingAlgorithm::CoolingSchedule::Geometric)});
        }
    }
}

void BenchmarkSimulatedAnnealing::runExperiment3(const std::vector<DatasetRecord>& datasets,
                                                 std::vector<BenchmarkRow>& rows) const {
    std::cout << "[SW-Benchmark] Experiment 3: parameter sweep\n";

    const std::vector<int> targetSizes = {17, 56, 171};
    const std::vector<double> temperatureMultipliers = {0.5, 1.0, 2.0, 4.0};
    const std::vector<double> coolingRates = {0.90, 0.95, 0.98, 0.99};
    const std::vector<SimulatedAnnealingAlgorithm::CoolingSchedule> schedules = {
            SimulatedAnnealingAlgorithm::CoolingSchedule::Geometric,
            SimulatedAnnealingAlgorithm::CoolingSchedule::Linear,
    };
    const std::vector<int> epochMultipliers = {1, 5, 10, 20};

    for (const int targetSize : targetSizes) {
        const DatasetRecord* dataset = pickNearestDataset(targetSize, datasets);
        if (dataset == nullptr) {
            continue;
        }

        const double baselineTemperature = std::max(100.0, static_cast<double>(dataset->size) * 100.0);
        const int baselineEpoch = std::max(10, dataset->size * 5);
        const int baselineMaxIterations = std::max(1000, dataset->size * dataset->size * 20);

        for (const double multiplier : temperatureMultipliers) {
            const double currentTemperature = baselineTemperature * multiplier;
            const AggregateResult aggregate = runTrials(*dataset,
                                                        SimulatedAnnealingAlgorithm::InitialSolutionType::NearestNeighbor,
                                                        SimulatedAnnealingAlgorithm::NeighborhoodType::Swap,
                                                        SimulatedAnnealingAlgorithm::CoolingSchedule::Geometric,
                                                        currentTemperature,
                                                        0.95,
                                                        baselineEpoch,
                                                        baselineMaxIterations);

            rows.push_back({dataset->fileName,
                            "SW",
                            dataset->size,
                            aggregate.averageTimeMicroseconds,
                            aggregate.averageCost,
                            aggregate.bestCost,
                            kTrialCount,
                            computeRelativeErrorPercent(aggregate.averageCost, dataset->optimum),
                            "NearestNeighbor",
                            "Swap",
                            makeParametersString(currentTemperature, 0.95, baselineEpoch, baselineMaxIterations,
                                                 SimulatedAnnealingAlgorithm::CoolingSchedule::Geometric) +
                                    ",tempMultiplier=" + std::to_string(multiplier)});
        }

        for (const double coolingRate : coolingRates) {
            const AggregateResult aggregate = runTrials(*dataset,
                                                        SimulatedAnnealingAlgorithm::InitialSolutionType::NearestNeighbor,
                                                        SimulatedAnnealingAlgorithm::NeighborhoodType::Swap,
                                                        SimulatedAnnealingAlgorithm::CoolingSchedule::Geometric,
                                                        baselineTemperature,
                                                        coolingRate,
                                                        baselineEpoch,
                                                        baselineMaxIterations);

            rows.push_back({dataset->fileName,
                            "SW",
                            dataset->size,
                            aggregate.averageTimeMicroseconds,
                            aggregate.averageCost,
                            aggregate.bestCost,
                            kTrialCount,
                            computeRelativeErrorPercent(aggregate.averageCost, dataset->optimum),
                            "NearestNeighbor",
                            "Swap",
                            makeParametersString(baselineTemperature,
                                                 coolingRate,
                                                 baselineEpoch,
                                                 baselineMaxIterations,
                                                 SimulatedAnnealingAlgorithm::CoolingSchedule::Geometric) +
                                    ",coolingSweep=true"});
        }

        for (const auto schedule : schedules) {
            const AggregateResult aggregate = runTrials(*dataset,
                                                        SimulatedAnnealingAlgorithm::InitialSolutionType::NearestNeighbor,
                                                        SimulatedAnnealingAlgorithm::NeighborhoodType::Swap,
                                                        schedule,
                                                        baselineTemperature,
                                                        0.95,
                                                        baselineEpoch,
                                                        baselineMaxIterations);

            rows.push_back({dataset->fileName,
                            "SW",
                            dataset->size,
                            aggregate.averageTimeMicroseconds,
                            aggregate.averageCost,
                            aggregate.bestCost,
                            kTrialCount,
                            computeRelativeErrorPercent(aggregate.averageCost, dataset->optimum),
                            "NearestNeighbor",
                            "Swap",
                            makeParametersString(baselineTemperature, 0.95, baselineEpoch, baselineMaxIterations, schedule)});
        }

        for (const int multiplier : epochMultipliers) {
            const int currentEpochLength = std::max(10, dataset->size * multiplier);
            const AggregateResult aggregate = runTrials(*dataset,
                                                        SimulatedAnnealingAlgorithm::InitialSolutionType::NearestNeighbor,
                                                        SimulatedAnnealingAlgorithm::NeighborhoodType::Swap,
                                                        SimulatedAnnealingAlgorithm::CoolingSchedule::Geometric,
                                                        baselineTemperature,
                                                        0.95,
                                                        currentEpochLength,
                                                        baselineMaxIterations);

            rows.push_back({dataset->fileName,
                            "SW",
                            dataset->size,
                            aggregate.averageTimeMicroseconds,
                            aggregate.averageCost,
                            aggregate.bestCost,
                            kTrialCount,
                            computeRelativeErrorPercent(aggregate.averageCost, dataset->optimum),
                            "NearestNeighbor",
                            "Swap",
                            makeParametersString(baselineTemperature, 0.95, currentEpochLength, baselineMaxIterations,
                                                 SimulatedAnnealingAlgorithm::CoolingSchedule::Geometric) +
                                    ",epochMultiplier=" + std::to_string(multiplier)});
        }
    }
}

void BenchmarkSimulatedAnnealing::writeCsv(const std::vector<BenchmarkRow>& rows) const {
    const bool fileExists = std::filesystem::exists("sa_benchmark_results.csv");
    const bool fileEmpty = !fileExists || std::filesystem::file_size("sa_benchmark_results.csv") == 0;

    std::ofstream outFile("sa_benchmark_results.csv", std::ios::app);
    if (!outFile) {
        std::cout << "[SW-Benchmark] Failed to save sa_benchmark_results.csv\n";
        return;
    }

    if (fileEmpty) {
        outFile << "Source;Algorithm;N;Avg_Times[us];Avg_Cost;Best_Cost;Trial_Count;Relative_Error[%];Initial_Solution;Neighborhood_Type;Parameters\n";
    }

    outFile << std::fixed << std::setprecision(3);
    for (const BenchmarkRow& row : rows) {
        outFile << row.source << ';' << row.algorithm << ';' << row.size << ';' << row.averageTimeMicroseconds << ';'
                << row.averageCost << ';' << row.bestCost << ';' << row.trialCount << ';' << row.relativeErrorPercent
                << ';' << row.initialSolution << ';' << row.neighborhoodType << ';' << row.parameters << '\n';
    }
}