#pragma once

#include "Result.h"
#include "SimulatedAnnealingAlgorithm.h"
#include "TSPData.h"

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

// BenchmarkSimulatedAnnealing: automated SW benchmark suite with CSV export.
class BenchmarkSimulatedAnnealing {
public:
    // Run all configured SW experiments and save the results into CSV.
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
        std::string algorithm;
        int size = 0;
        long long averageTimeMicroseconds = 0;
        double averageCost = 0.0;
        long long bestCost = -1;
        int trialCount = 0;
        double relativeErrorPercent = 0.0;
        std::string initialSolution;
        std::string neighborhoodType;
        std::string parameters;
    };

    struct AggregateResult {
        long long averageTimeMicroseconds = 0;
        double averageCost = 0.0;
        long long bestCost = -1;
    };

    static constexpr int kTrialCount = 3;

    [[nodiscard]] std::vector<DatasetRecord> loadDatasets() const;
    [[nodiscard]] std::optional<std::filesystem::path> resolveDataDirectory() const;
    [[nodiscard]] const DatasetRecord* pickNearestDataset(int targetSize, const std::vector<DatasetRecord>& datasets) const;
    [[nodiscard]] AggregateResult runTrials(const DatasetRecord& dataset,
                                            SimulatedAnnealingAlgorithm::InitialSolutionType initialSolution,
                                            SimulatedAnnealingAlgorithm::NeighborhoodType neighborhoodType,
                                            SimulatedAnnealingAlgorithm::CoolingSchedule schedule,
                                            double initialTemperature,
                                            double coolingRate,
                                            int epochLength,
                                            int maxIterations) const;
    // If targetSize <= 14, attempt to build a DatasetRecord based on a truncated ftv170.atsp
    // Returns std::nullopt if truncated base is not available or construction fails.
    [[nodiscard]] std::optional<DatasetRecord> selectTruncatedFt170IfApplicable(int targetSize) const;
    [[nodiscard]] double computeRelativeErrorPercent(double measured, long long optimum) const;
    [[nodiscard]] static std::string makeParametersString(double initialTemperature,
                                                          double coolingRate,
                                                          int epochLength,
                                                          int maxIterations,
                                                          SimulatedAnnealingAlgorithm::CoolingSchedule schedule);
    void writeCsv(const std::vector<BenchmarkRow>& rows) const;

    void runExperiment1(const std::vector<DatasetRecord>& datasets, std::vector<BenchmarkRow>& rows) const;
    void runExperiment2(const std::vector<DatasetRecord>& datasets, std::vector<BenchmarkRow>& rows) const;
    void runExperiment3(const std::vector<DatasetRecord>& datasets, std::vector<BenchmarkRow>& rows) const;
};