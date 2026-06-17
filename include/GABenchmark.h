#pragma once

#include "GeneticAlgorithm.h"
#include "Result.h"
#include "TSPData.h"

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

// GABenchmark: automated GA benchmark suite with CSV export.
class GABenchmark {
public:
    void run();

private:
    struct DatasetRecord {
        std::string fileName;
        std::string filePath;
        int size = 0;
        long long optimum = -1;
        TSPData data;
    };

    struct BenchmarkRow {
        std::string source;
        int size = 0;
        std::string algorithm;
        int populationSize = 0;
        double crossoverRate = 0.0;
        double mutationRate = 0.0;
        long long averageTimeMicroseconds = 0;
        long long bestTimeMicroseconds = 0;
        double averageCost = 0.0;
        long long bestCost = -1;
        long long worstCost = -1;
        double stdDevCost = 0.0;
        long long referenceCost = -1;
        double relativeError = 0.0;
    };

    struct AggregateResult {
        long long averageTimeMicroseconds = 0;
        long long bestTimeMicroseconds = 0;
        double averageCost = 0.0;
        long long bestCost = -1;
        long long worstCost = -1;
        double stdDevCost = 0.0;
    };

    static constexpr int kTrialCount = 100;

    [[nodiscard]] std::vector<DatasetRecord> loadDatasets() const;
    [[nodiscard]] std::optional<std::filesystem::path> resolveDataDirectory() const;
    [[nodiscard]] const DatasetRecord* pickNearestDataset(int targetSize,
                                                          const std::vector<DatasetRecord>& datasets) const;
    [[nodiscard]] std::optional<DatasetRecord> selectTruncatedFt170IfApplicable(int targetSize) const;
    [[nodiscard]] std::optional<DatasetRecord> resolveDatasetForSize(int targetSize,
                                                                     const std::vector<DatasetRecord>& datasets) const;
    [[nodiscard]] AggregateResult runTrials(const DatasetRecord& dataset,
                                            int populationSize,
                                            double crossoverRate,
                                            double mutationRate) const;
    [[nodiscard]] double computeRelativeErrorPercent(double measured, long long optimum) const;
    void writeCsv(const std::vector<BenchmarkRow>& rows) const;

    void runScenario1(const std::vector<DatasetRecord>& datasets, std::vector<BenchmarkRow>& rows) const;
    void runScenario2(const std::vector<DatasetRecord>& datasets, std::vector<BenchmarkRow>& rows) const;
    void runScenario3(const std::vector<DatasetRecord>& datasets, std::vector<BenchmarkRow>& rows) const;
    void runScenario4(const std::vector<DatasetRecord>& datasets, std::vector<BenchmarkRow>& rows) const;
};
