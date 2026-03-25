#pragma once

#include "Result.h"
#include "TSPData.h"

#include <map>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

// Benchmark: orchestrates calibration and measurement across algorithms/datasets
class Benchmark {
public:
    void runSmartBenchmark();

private:
    enum class AlgorithmKind {
        BruteForce,
        Random,
        NearestNeighbor,
        RepetitiveNN
    };

    // BenchmarkRow: single CSV row describing a measurement
    struct BenchmarkRow {
        std::string source;
        std::string algorithm;
        int size = 0;
        long long averageTimeMicroseconds = 0;
        double averageCost = 0.0;
        long long bestCost = -1;
        int trialsCount = 0;
        long long referenceCost = -1;
        double relativeErrorPercent = 0.0;
        std::string parameters;
    };

    // DatasetDescriptor: dataset metadata (file + known optimum if available)
    struct DatasetDescriptor {
        std::string fileName;
        long long knownOptimum = -1;
    };

    // MeasurementPoint: requested vs mapped size and source information
    struct MeasurementPoint {
        int requestedSize = 0;
        int mappedSize = 0;
        std::string sourceLabel;
        std::string fileName;
        bool useTruncatedBase = false;
        long long referenceCost = -1;
    };

    // AggregateResult: average time/cost across trials
    struct AggregateResult {
        long long averageTimeMicroseconds = 0;
        double averageCost = 0.0;
        long long bestCost = -1;
    };

    // Configuration constants used by benchmark
    static constexpr long long kTargetCalibrationTimeMicroseconds = 10LL * 60LL * 1000LL * 1000LL;
    static constexpr int kBruteForceHardMaxSize = 14;
    static constexpr int kTrialsPerMeasurement = 3;
    static constexpr int kRandConvergenceTrials = 10;
    static constexpr long long kRandTimeLimitMs = 1000;
    static constexpr const char* kCalibrationFileName = "ftv170.atsp";

    [[nodiscard]] static std::string algorithmName(AlgorithmKind kind);

    [[nodiscard]] TSPData loadFromFileName(const std::string& fileName) const;
    [[nodiscard]] std::optional<std::string> resolveDataPath(const std::string& fileName) const;
    [[nodiscard]] double computeRelativeErrorPercent(long long measured, long long reference) const;

    [[nodiscard]] int calibrateMaxSize(AlgorithmKind kind, const TSPData& baseData) const;
    [[nodiscard]] AggregateResult runMultipleTrials(AlgorithmKind kind, const TSPData& data, int trials) const;
    [[nodiscard]] Result runSingle(AlgorithmKind kind, const TSPData& data) const;

    [[nodiscard]] std::vector<int> buildSharedSmallPoints(int bfMaxSize) const;
    [[nodiscard]] std::vector<MeasurementPoint> buildPointsForAlgorithm(AlgorithmKind kind,
                                                                        int algorithmMaxSize,
                                                                        const std::vector<int>& sharedSmallPoints,
                                                                        const std::map<int, DatasetDescriptor>& datasetBySize,
                                                                        const std::map<int, long long>& smallPointBfCosts,
                                                                        int baseSize) const;

    [[nodiscard]] std::vector<int> buildExtendedIdealPoints(int fromSize, int toSize) const;
    [[nodiscard]] std::pair<int, DatasetDescriptor> mapToNearestDataset(
            int targetSize,
            const std::map<int, DatasetDescriptor>& datasetBySize) const;
    void runRandConvergenceTest(std::vector<BenchmarkRow>& rows,
                                const std::map<int, DatasetDescriptor>& datasetBySize) const;


    void writeCsv(const std::vector<BenchmarkRow>& rows) const;
};
