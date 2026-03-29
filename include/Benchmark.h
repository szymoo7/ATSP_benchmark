#pragma once

#include "Result.h"
#include "TSPData.h"

#include <map>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

// Benchmark: calibration and measurement for algorithms and datasets
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

    // DatasetDescriptor: dataset information (file + known optimum if available)
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

    // Configuration parameters used by benchmark
    static constexpr long long kTargetCalibrationTimeMicroseconds = 10LL * 60LL * 1000LL * 1000LL; // 10 minutes
    static constexpr int kBruteForceHardMaxSize = 15;
    static constexpr int kTrialsPerMeasurement = 5;
    static constexpr int kRandConvergenceTrials = 5;
    static constexpr long long kRandTimeLimitMs = 1000 * 60 * 10; // 10 minutes
    static constexpr const char* kCalibrationFileName = "ftv170.atsp";

    // Return the corresponding string for given algorithm
    [[nodiscard]] static std::string algorithmName(AlgorithmKind kind);

    // Load data from file with given name
    [[nodiscard]] TSPData loadFromFileName(const std::string& fileName) const;

    // Find file path with given name
    [[nodiscard]] std::optional<std::string> resolveDataPath(const std::string& fileName) const;

    // Calculate relative error
    [[nodiscard]] double computeRelativeErrorPercent(long long measured, long long reference) const;

    // Find N_max
    [[nodiscard]] int calibrateMaxSize(AlgorithmKind kind, const TSPData& baseData) const;

    // Run multiple tests for same instance
    [[nodiscard]] AggregateResult runMultipleTrials(AlgorithmKind kind, const TSPData& data, int trials) const;

    // Run single test for instance
    [[nodiscard]] Result runSingle(AlgorithmKind kind, const TSPData& data) const;

    // Find points that cover with bruteforce
    [[nodiscard]] std::vector<int> buildSharedSmallPoints(int bfMaxSize) const;

    //
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
