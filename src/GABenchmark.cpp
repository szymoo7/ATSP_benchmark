#include "../include/GABenchmark.h"

#include "../include/TSPLIBParser.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <unordered_map>

namespace {

const std::unordered_map<std::string, int> kKnownOptimums = {
        {"br17.atsp", 39},
        {"ftv33.atsp", 1286},
        {"ftv35.atsp", 1473},
        {"ftv38.atsp", 1530},
        {"p43.atsp", 5620},
        {"ftv44.atsp", 1613},
        {"ftv47.atsp", 1776},
        {"ry48p.atsp", 14422},
        {"ftv55.atsp", 1608},
        {"ftv64.atsp", 1839},
        {"ftv70.atsp", 1950},
        {"kro124p.atsp", 36230},
        {"ftv170.atsp", 2755},
        {"rbg323.atsp", 1326},
        {"rbg403.atsp", 2465},
        {"rbg443.atsp", 2720},
};

const std::unordered_map<int, int> kTruncatedFt170Optimums = {
        {8, 188},
        {9, 188},
        {10, 188},
        {11, 361},
        {12, 383},
        {13, 389},
        {14, 411},
};

constexpr std::array<int, 10> kBenchmarkSizes = {10, 12, 14, 45, 71, 100, 171, 323, 403, 443};

constexpr int kBaselinePopulation = 100;
constexpr double kBaselineCrossoverRate = 0.8;
constexpr double kBaselineMutationRate = 0.1;

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

double computeStdDev(const std::vector<double>& values, double mean) {
    if (values.size() <= 1) {
        return 0.0;
    }

    double varianceSum = 0.0;
    for (const double value : values) {
        const double delta = value - mean;
        varianceSum += delta * delta;
    }
    return std::sqrt(varianceSum / static_cast<double>(values.size()));
}

} // namespace

void GABenchmark::run() {
    std::cout << "\n[GA-Benchmark] Loading TSPLIB datasets from data directory\n";
    std::cout << "[GA-Benchmark] Warning: full suite runs " << kTrialCount
              << " trials per configuration and may take a long time.\n";

    const std::vector<DatasetRecord> datasets = loadDatasets();
    if (datasets.empty()) {
        std::cout << "[GA-Benchmark] No valid .atsp datasets found.\n";
        return;
    }

    std::cout << "[GA-Benchmark] Loaded " << datasets.size() << " dataset files.\n";

    std::vector<BenchmarkRow> rows;
    runScenario1(datasets, rows);
    runScenario2(datasets, rows);
    runScenario3(datasets, rows);
    runScenario4(datasets, rows);

    writeCsv(rows);
    std::cout << "[GA-Benchmark] Done. Saved: ga_benchmark_results.csv\n";
}

std::vector<GABenchmark::DatasetRecord> GABenchmark::loadDatasets() const {
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
            const auto itOptimum = kKnownOptimums.find(record.fileName);
            record.optimum = (itOptimum != kKnownOptimums.end()) ? itOptimum->second : -1;
            datasets.push_back(std::move(record));
        } catch (const std::exception& ex) {
            std::cout << "[GA-Benchmark] Skipping " << filePath.filename().string() << ": " << ex.what() << "\n";
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

std::optional<std::filesystem::path> GABenchmark::resolveDataDirectory() const {
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

std::optional<GABenchmark::DatasetRecord> GABenchmark::selectTruncatedFt170IfApplicable(int targetSize) const {
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
        const TSPData base = TSPLIBParser::parse(basePath.string());
        TSPData truncated = base.getTruncatedData(targetSize);
        DatasetRecord record;
        record.fileName = "TRUNC_" + std::to_string(targetSize) + "_ftv170.atsp";
        record.filePath = basePath.string();
        record.size = targetSize;

        const auto itRef = kTruncatedFt170Optimums.find(targetSize);
        record.optimum = (itRef != kTruncatedFt170Optimums.end()) ? itRef->second : -1;
        record.data = std::move(truncated);
        return record;
    } catch (const std::exception& ex) {
        std::cout << "[GA-Benchmark] Warning: failed to load/trim ftv170.atsp: " << ex.what() << "\n";
        return std::nullopt;
    }
}

const GABenchmark::DatasetRecord* GABenchmark::pickNearestDataset(int targetSize,
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

std::optional<GABenchmark::DatasetRecord> GABenchmark::resolveDatasetForSize(
        int targetSize,
        const std::vector<DatasetRecord>& datasets) const {
    const std::optional<DatasetRecord> truncated = selectTruncatedFt170IfApplicable(targetSize);
    if (truncated.has_value()) {
        return truncated;
    }

    const DatasetRecord* nearest = pickNearestDataset(targetSize, datasets);
    if (nearest == nullptr) {
        return std::nullopt;
    }

    return *nearest;
}

GABenchmark::AggregateResult GABenchmark::runTrials(const DatasetRecord& dataset,
                                                    int populationSize,
                                                    double crossoverRate,
                                                    double mutationRate) const {
    long long totalTime = 0;
    double totalCost = 0.0;
    long long bestCost = std::numeric_limits<long long>::max();
    long long worstCost = std::numeric_limits<long long>::min();
    long long bestTime = std::numeric_limits<long long>::max();
    std::vector<double> trialCosts;
    trialCosts.reserve(static_cast<std::size_t>(kTrialCount));

    const std::string seedTag = dataset.fileName + "|GA|" + std::to_string(populationSize) + "|" +
                                std::to_string(crossoverRate) + "|" + std::to_string(mutationRate);

    for (int trial = 0; trial < kTrialCount; ++trial) {
        const std::uint32_t baseSeed = seedFromText(seedTag);
        const std::uint32_t trialSeed = mixSeed(baseSeed, static_cast<std::uint32_t>(trial + 1));

        GeneticAlgorithm algorithm(populationSize,
                                   crossoverRate,
                                   mutationRate,
                                   0,
                                   GeneticAlgorithm::MutationType::Random,
                                   trialSeed);
        const Result result = algorithm.solve(dataset.data);

        totalTime += result.executionTimeMicroseconds;
        totalCost += static_cast<double>(result.minCost);
        trialCosts.push_back(static_cast<double>(result.minCost));
        bestCost = std::min(bestCost, static_cast<long long>(result.minCost));
        worstCost = std::max(worstCost, static_cast<long long>(result.minCost));
        bestTime = std::min(bestTime, result.executionTimeMicroseconds);
    }

    AggregateResult aggregate;
    aggregate.averageTimeMicroseconds = totalTime / kTrialCount;
    aggregate.bestTimeMicroseconds = (bestTime == std::numeric_limits<long long>::max()) ? 0 : bestTime;
    aggregate.averageCost = totalCost / static_cast<double>(kTrialCount);
    aggregate.bestCost = (bestCost == std::numeric_limits<long long>::max()) ? -1 : bestCost;
    aggregate.worstCost = (worstCost == std::numeric_limits<long long>::min()) ? -1 : worstCost;
    aggregate.stdDevCost = computeStdDev(trialCosts, aggregate.averageCost);
    return aggregate;
}

double GABenchmark::computeRelativeErrorPercent(double measured, long long optimum) const {
    if (optimum <= 0) {
        return 0.0;
    }
    return ((measured - static_cast<double>(optimum)) / static_cast<double>(optimum)) * 100.0;
}

void GABenchmark::runScenario1(const std::vector<DatasetRecord>& datasets, std::vector<BenchmarkRow>& rows) const {
    std::cout << "[GA-Benchmark] Scenario 1: baseline configuration\n";

    for (const int targetSize : kBenchmarkSizes) {
        const std::optional<DatasetRecord> dataset = resolveDatasetForSize(targetSize, datasets);
        if (!dataset.has_value()) {
            continue;
        }

        const AggregateResult aggregate = runTrials(*dataset,
                                                    kBaselinePopulation,
                                                    kBaselineCrossoverRate,
                                                    kBaselineMutationRate);

        rows.push_back({dataset->fileName,
                        dataset->size,
                        "GA",
                        kBaselinePopulation,
                        kBaselineCrossoverRate,
                        kBaselineMutationRate,
                        aggregate.averageTimeMicroseconds,
                        aggregate.bestTimeMicroseconds,
                        aggregate.averageCost,
                        aggregate.bestCost,
                        aggregate.worstCost,
                        aggregate.stdDevCost,
                        dataset->optimum,
                        computeRelativeErrorPercent(aggregate.averageCost, dataset->optimum)});
    }
}

void GABenchmark::runScenario2(const std::vector<DatasetRecord>& datasets, std::vector<BenchmarkRow>& rows) const {
    std::cout << "[GA-Benchmark] Scenario 2: population size impact\n";

    const std::vector<int> populationSizes = {50, 100, 200};
    for (const int targetSize : kBenchmarkSizes) {
        const std::optional<DatasetRecord> dataset = resolveDatasetForSize(targetSize, datasets);
        if (!dataset.has_value()) {
            continue;
        }

        for (const int populationSize : populationSizes) {
            const AggregateResult aggregate = runTrials(*dataset,
                                                        populationSize,
                                                        kBaselineCrossoverRate,
                                                        kBaselineMutationRate);

            rows.push_back({dataset->fileName,
                            dataset->size,
                            "GA",
                            populationSize,
                            kBaselineCrossoverRate,
                            kBaselineMutationRate,
                            aggregate.averageTimeMicroseconds,
                            aggregate.bestTimeMicroseconds,
                            aggregate.averageCost,
                            aggregate.bestCost,
                            aggregate.worstCost,
                            aggregate.stdDevCost,
                            dataset->optimum,
                            computeRelativeErrorPercent(aggregate.averageCost, dataset->optimum)});
        }
    }
}

void GABenchmark::runScenario3(const std::vector<DatasetRecord>& datasets, std::vector<BenchmarkRow>& rows) const {
    std::cout << "[GA-Benchmark] Scenario 3: crossover rate impact\n";

    const std::vector<double> crossoverRates = {0.5, 0.8, 0.95};
    for (const int targetSize : kBenchmarkSizes) {
        const std::optional<DatasetRecord> dataset = resolveDatasetForSize(targetSize, datasets);
        if (!dataset.has_value()) {
            continue;
        }

        for (const double crossoverRate : crossoverRates) {
            const AggregateResult aggregate = runTrials(*dataset,
                                                        kBaselinePopulation,
                                                        crossoverRate,
                                                        kBaselineMutationRate);

            rows.push_back({dataset->fileName,
                            dataset->size,
                            "GA",
                            kBaselinePopulation,
                            crossoverRate,
                            kBaselineMutationRate,
                            aggregate.averageTimeMicroseconds,
                            aggregate.bestTimeMicroseconds,
                            aggregate.averageCost,
                            aggregate.bestCost,
                            aggregate.worstCost,
                            aggregate.stdDevCost,
                            dataset->optimum,
                            computeRelativeErrorPercent(aggregate.averageCost, dataset->optimum)});
        }
    }
}

void GABenchmark::runScenario4(const std::vector<DatasetRecord>& datasets, std::vector<BenchmarkRow>& rows) const {
    std::cout << "[GA-Benchmark] Scenario 4: mutation rate impact\n";

    const std::vector<double> mutationRates = {0.05, 0.15, 0.30};
    for (const int targetSize : kBenchmarkSizes) {
        const std::optional<DatasetRecord> dataset = resolveDatasetForSize(targetSize, datasets);
        if (!dataset.has_value()) {
            continue;
        }

        for (const double mutationRate : mutationRates) {
            const AggregateResult aggregate = runTrials(*dataset,
                                                        kBaselinePopulation,
                                                        kBaselineCrossoverRate,
                                                        mutationRate);

            rows.push_back({dataset->fileName,
                            dataset->size,
                            "GA",
                            kBaselinePopulation,
                            kBaselineCrossoverRate,
                            mutationRate,
                            aggregate.averageTimeMicroseconds,
                            aggregate.bestTimeMicroseconds,
                            aggregate.averageCost,
                            aggregate.bestCost,
                            aggregate.worstCost,
                            aggregate.stdDevCost,
                            dataset->optimum,
                            computeRelativeErrorPercent(aggregate.averageCost, dataset->optimum)});
        }
    }
}

void GABenchmark::writeCsv(const std::vector<BenchmarkRow>& rows) const {
    std::ofstream outFile("ga_benchmark_results.csv", std::ios::trunc);
    if (!outFile) {
        std::cout << "[GA-Benchmark] Failed to save ga_benchmark_results.csv\n";
        return;
    }

    outFile << "Source;N;Algorithm;Population_Size;Crossover_Rate;Mutation_Rate;Avg_Time_us;Best_Time_us;"
               "Avg_Cost;Best_Cost;Worst_Cost;Std_Dev_Cost;Reference_Cost;Relative_Error\n";
    outFile << std::fixed << std::setprecision(3);

    for (const BenchmarkRow& row : rows) {
        outFile << row.source << ';' << row.size << ';' << row.algorithm << ';' << row.populationSize << ';'
                << row.crossoverRate << ';' << row.mutationRate << ';' << row.averageTimeMicroseconds << ';'
                << row.bestTimeMicroseconds << ';' << row.averageCost << ';' << row.bestCost << ';' << row.worstCost
                << ';' << row.stdDevCost << ';' << row.referenceCost << ';' << row.relativeError << '\n';
    }
}
