// Benchmark: automated benchmarking utility for ATSP algorithms
// Calibrates runtime limits, maps TSPLIB instances to target sizes and produces CSV results

#include "../include/Benchmark.h"

#include "../include/BruteForceAlgorithm.h"
#include "../include/NearestNeighborAlgorithm.h"
#include "../include/RandomSearchAlgorithm.h"
#include "../include/RepetitiveNNAlgorithm.h"
#include "../include/TSPLIBParser.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
#include <optional>
#include <random>
#include <stdexcept>
#include <vector>

// Mapped data set in format {nodes, {file_name, best_known_solution}}
namespace {
const std::map<int, std::pair<std::string, long long>> kDatasetCatalog = {
    {17, {"br17.atsp", 39}},
    {34, {"ftv33.atsp", 1286}},
    {36, {"ftv35.atsp", 1473}},
    {39, {"ftv38.atsp", 1530}},
    {43, {"p43.atsp", 5620}},
    {45, {"ftv44.atsp", 1613}},
    {48, {"ftv47.atsp", 1776}},
    {53, {"ft53.atsp", 6905}},
    {56, {"ftv55.atsp", 1608}},
    {65, {"ftv64.atsp", 1839}},
    {70, {"ft70.atsp", 38673}},
    {71, {"ftv70.atsp", 1950}},
    {100, {"kro124p.atsp", 36230}},
    {171, {"ftv170.atsp", 2755}},
    {48, {"ry48p.atsp", 14422}},
};
}

// runSmartBenchmark: main function for the automated benchmark
// - Performs calibration, maps requested sizes to available TSPLIB files
// - executes measurements for each algorithm, and writes CSV results
// - Controls flow and error handling for the full benchmark
void Benchmark::runSmartBenchmark() {
    std::cout << "\n[Benchmark] ============================================\n";
    std::cout << "[Benchmark] Starting intelligent ATSP benchmark\n";
    std::cout << "[Benchmark] Phase 1/3: Calibration on ftv170.atsp\n";
    std::cout << "[Benchmark] Phase 2/3: Intelligent point mapping\n";
    std::cout << "[Benchmark] Phase 3/4: Baseline measurements and TSPLIB mapping\n";
    std::cout << "[Benchmark] Phase 4/4: RAND convergence test\n";
    std::cout << "[Benchmark] ============================================\n";

    // Finding dataset files
    std::map<int, DatasetDescriptor> datasetBySize;
    for (const auto& [size, descriptor] : kDatasetCatalog) {
        if (!resolveDataPath(descriptor.first).has_value()) {
            std::cout << "[Benchmark] Warning: missing data file " << descriptor.first << " (N=" << size
                      << "), skipping in mapping.\n";
            continue;
        }
        datasetBySize[size] = {descriptor.first, descriptor.second};
    }

    // Loading file for calibration (ftv170.atsp)
    TSPData baseData;
    try {
        baseData = loadFromFileName(kCalibrationFileName);
    } catch (const std::exception& ex) {
        std::cout << "[Benchmark] Calibration file loading error: " << ex.what() << "\n";
        return;
    }

    std::cout << "[Benchmark] Calibration file loaded: " << kCalibrationFileName << ", base_N=" << baseData.size
              << "\n";
    std::cout << "[Benchmark] Single run time limit in calibration: "
              << kTargetCalibrationTimeMicroseconds / 1000LL << " ms (~10 min)\n";

    // Running calibration for algorithms (finding N_max)
    const int bfMaxSize = calibrateMaxSize(AlgorithmKind::BruteForce, baseData);
    const int nnMaxSize = calibrateMaxSize(AlgorithmKind::NearestNeighbor, baseData);
    const int rnnMaxSize = calibrateMaxSize(AlgorithmKind::RepetitiveNN, baseData);
    const int randMaxSize = rnnMaxSize;

    std::cout << "[Benchmark] Calibration results Nmax: BF=" << bfMaxSize << ", NN=" << nnMaxSize
              << ", RNN=" << rnnMaxSize << " (RAND uses RNN)\n";

    // Finding shared points (3 last < bfMaxSize)
    const std::vector<int> sharedSmallPoints = buildSharedSmallPoints(bfMaxSize);
    std::cout << "[Benchmark] Shared points (small N): ";
    for (size_t i = 0; i < sharedSmallPoints.size(); ++i) {
        std::cout << sharedSmallPoints[i] << (i + 1 == sharedSmallPoints.size() ? "\n" : ", ");
    }

    // Run tests for brute force small N (N < 12)
    std::map<int, long long> smallPointBfCosts;
    for (const int n : sharedSmallPoints) {
        const TSPData truncated = baseData.getTruncatedData(n);
        const AggregateResult bfAggregate = runMultipleTrials(AlgorithmKind::BruteForce, truncated, kTrialsPerMeasurement);
        const auto bfCost = static_cast<long long>(std::llround(bfAggregate.averageCost));
        smallPointBfCosts[n] = bfCost;
        std::cout << "[Benchmark] BF reference for small N=" << n << " -> cost=" << bfCost << "\n";
    }

    std::vector<BenchmarkRow> rows;

    const auto appendRowsForAlgorithm = [&](AlgorithmKind kind, int algorithmMaxSize) {

        // Build rest 4 points for algorithm
        std::cout << "\n[Benchmark] ---- Algorithm: " << algorithmName(kind) << " ----\n";
        const std::vector<MeasurementPoint> points = buildPointsForAlgorithm(
                kind,
                algorithmMaxSize,
                sharedSmallPoints,
                datasetBySize,
                smallPointBfCosts,
                static_cast<int>(baseData.size));

        std::cout << "[Benchmark] Test points: ";
        for (size_t i = 0; i < points.size(); ++i) {
            std::cout << points[i].mappedSize << "(" << points[i].sourceLabel << ")"
                      << (i + 1 == points.size() ? "\n" : ", ");
        }
        for (const MeasurementPoint& point : points) {
            std::cout << "[Benchmark]   point: requested_N=" << point.requestedSize << " -> used_N=" << point.mappedSize
                      << " | file=" << point.fileName
                      << " | mode=" << (point.useTruncatedBase ? "truncated calibration file" : "full TSPLIB file")
                      << "\n";
        }

        for (const MeasurementPoint& point : points) {
            TSPData testData;
            try {
                testData = point.useTruncatedBase ? baseData.getTruncatedData(point.mappedSize)
                                                  : loadFromFileName(point.fileName);
            } catch (const std::exception& ex) {
                std::cout << "[Benchmark] Skipping point N=" << point.mappedSize << " for " << algorithmName(kind)
                          << " (" << ex.what() << ")\n";
                continue;
            }

            // Running tests
            const AggregateResult aggregate = runMultipleTrials(kind, testData, kTrialsPerMeasurement);
            const long long avgUs = aggregate.averageTimeMicroseconds;
            const long long measuredCost = static_cast<long long>(std::llround(aggregate.averageCost));

            double relativeError = 0.0;
            if (kind == AlgorithmKind::BruteForce) {
                relativeError = 0.0;
            } else if (point.referenceCost > 0) {
                relativeError = computeRelativeErrorPercent(measuredCost, point.referenceCost);
            }

            // Prepare rows for saving
            rows.push_back({point.sourceLabel,
                            algorithmName(kind),
                            point.mappedSize,
                            avgUs,
                            aggregate.averageCost,
                            aggregate.bestCost,
                            kTrialsPerMeasurement,
                            point.referenceCost,
                            relativeError,
                            "MainBenchmark"});

            std::cout << "[Benchmark] Result -> src=" << point.sourceLabel << ", alg=" << algorithmName(kind)
                      << ", N=" << point.mappedSize << ", t_avg_us=" << avgUs << ", cost=" << measuredCost
                      << ", error_%=" << std::fixed << std::setprecision(3) << relativeError << std::defaultfloat
                      << "\n";
        }

        std::cout << "[Benchmark] ---- End of algorithm: " << algorithmName(kind) << " ----\n";
    };

    // Add rows and run tests
    appendRowsForAlgorithm(AlgorithmKind::BruteForce, bfMaxSize);
    appendRowsForAlgorithm(AlgorithmKind::NearestNeighbor, nnMaxSize);
    appendRowsForAlgorithm(AlgorithmKind::RepetitiveNN, rnnMaxSize);
    appendRowsForAlgorithm(AlgorithmKind::Random, randMaxSize);

    // Run random test for convergence
    try {
        runRandConvergenceTest(rows, datasetBySize);
    } catch (const std::exception& ex) {
        std::cout << "[Benchmark] RAND convergence test skipped: " << ex.what() << "\n";
    }

    // Save data into csv
    writeCsv(rows);
    std::cout << "[Benchmark] Benchmark finished. Results saved to benchmark_results.csv\n";
}

// algorithmName: return short label for a given AlgorithmKind
// - Pure helper: maps enum to short string used in logs/CSV
std::string Benchmark::algorithmName(AlgorithmKind kind) {
    switch (kind) {
        case AlgorithmKind::BruteForce:
            return "BruteForce";
        case AlgorithmKind::Random:
            return "RAND";
        case AlgorithmKind::NearestNeighbor:
            return "NN";
        case AlgorithmKind::RepetitiveNN:
            return "RNN";
        default:
            return "Unknown";
    }
}

// loadFromFileName: resolve a dataset path and parse it into TSPData
// - Throws runtime_error if file cannot be resolved
// - Delegates heavy lifting to TSPLIBParser::parse
TSPData Benchmark::loadFromFileName(const std::string& fileName) const {
    const auto resolved = resolveDataPath(fileName);
    if (!resolved.has_value()) {
        throw std::runtime_error("Data file not found: " + fileName);
    }
    return TSPLIBParser::parse(resolved.value());
}

// resolveDataPath: search common relative locations for a data file and return absolute path
// - Looks under ./data, ../data and ../../data and returns the first existing path
// - Returns std::nullopt if not found
std::optional<std::string> Benchmark::resolveDataPath(const std::string& fileName) const {
    namespace fs = std::filesystem;

    const std::vector<fs::path> candidates = {
            fs::path("data") / fileName,
            fs::path("..") / "data" / fileName,
            fs::path("..") / ".." / "data" / fileName,
    };

    // Check several relative candidate locations for the requested file
    for (const auto& relativeCandidate : candidates) {
        const fs::path absolute = fs::weakly_canonical(fs::current_path() / relativeCandidate);
        if (fs::exists(absolute)) {
            return absolute.string();
        }
    }
    return std::nullopt;
}

// Calculating relative error
double Benchmark::computeRelativeErrorPercent(long long measured, long long reference) const {
    // compute relative error in percent; return 0 if reference invalid
    if (reference <= 0) {
        return 0.0;
    }
    return (static_cast<double>(measured - reference) / static_cast<double>(reference)) * 100.0;
}

// calibrateMaxSize: determines the largest problem under 10 minutes
// - Runs increasing sizes until the per-run time exceeds
//   kTargetCalibrationTimeMicroseconds or the base size limit
// - Returns the largest N considered acceptable for later benchmarking
int Benchmark::calibrateMaxSize(AlgorithmKind kind, const TSPData& baseData) const {
    const long long limitUs = kTargetCalibrationTimeMicroseconds;
    int startN = 5;
    int step = 1;

    if (kind == AlgorithmKind::NearestNeighbor || kind == AlgorithmKind::RepetitiveNN) {
        startN = 10;
        step = 10;
    }

    const int baseMaxN = static_cast<int>(baseData.size);
    const int maxN = (kind == AlgorithmKind::BruteForce) ? std::min(baseMaxN, kBruteForceHardMaxSize) : baseMaxN;
    int bestSize = std::max(2, startN - step);

    std::cout << "[Benchmark] [Calibration] Start for " << algorithmName(kind) << " | startN=" << startN
              << ", step=" << step << "\n";
    if (kind == AlgorithmKind::BruteForce) {
        std::cout << "[Benchmark] [Calibration] BruteForce has hard limit N <= " << kBruteForceHardMaxSize
                  << "\n";
    }

    for (int n = startN; n <= maxN; n += step) {
        const TSPData candidate = baseData.getTruncatedData(n);
        const auto t0 = std::chrono::steady_clock::now();
        const Result result = runSingle(kind, candidate);
        const auto t1 = std::chrono::steady_clock::now();
        const long long elapsedUs = std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count();

        std::cout << "[Benchmark] [Calibration] " << algorithmName(kind) << " | N=" << n << " | time_us=" << elapsedUs
                  << "\n";

        if (elapsedUs > limitUs) {
            std::cout << "[Benchmark] [Calibration] Exceeded 10 min at N=" << n << " -> stop\n";
            break;
        }

        (void)result;
        bestSize = n;

        if (bestSize >= maxN) {
            break;
        }

        if ((kind == AlgorithmKind::NearestNeighbor || kind == AlgorithmKind::RepetitiveNN) && step > 1 &&
            n + step > maxN) {
            step = 1;
        }
    }

    std::cout << "[Benchmark] [Calibration] Nmax(" << algorithmName(kind) << ") = " << bestSize << "\n";
    return bestSize;
}

// runMultipleTrials: execute trials independent runs of kind on data
// - Collects average time, average cost and best observed cost
// - Ensures at least one trial is executed
Benchmark::AggregateResult Benchmark::runMultipleTrials(AlgorithmKind kind, const TSPData& data, int trials) const {
    long long totalTime = 0;
    long long totalCost = 0;
    long long bestCost = std::numeric_limits<long long>::max();

    const int safeTrials = std::max(1, trials);
    for (int i = 0; i < safeTrials; ++i) {
        const Result result = runSingle(kind, data);
        std::cout << "[Benchmark] [Trial] " << algorithmName(kind) << " | trial=" << (i + 1) << "/" << safeTrials
                  << " | time_us=" << result.executionTimeMicroseconds << " | cost=" << result.minCost << "\n";
        totalTime += result.executionTimeMicroseconds;
        totalCost += result.minCost;
        bestCost = std::min(bestCost, static_cast<long long>(result.minCost));
    }

    AggregateResult aggregate;
    aggregate.averageTimeMicroseconds = totalTime / safeTrials;
    aggregate.averageCost = static_cast<double>(totalCost) / static_cast<double>(safeTrials);
    aggregate.bestCost = (bestCost == std::numeric_limits<long long>::max()) ? -1 : bestCost;
    return aggregate;
}

// runSingle: run a single execution of the requested algorithm kind on data
// - Dispatches to the concrete algorithm implementations
// - Returns Result containing path, cost and execution time
Result Benchmark::runSingle(AlgorithmKind kind, const TSPData& data) const {
    switch (kind) {
        case AlgorithmKind::BruteForce: {
            BruteForceAlgorithm algorithm;
            return algorithm.solve(data);
        }
        case AlgorithmKind::Random: {
            RandomSearchAlgorithm algorithm(kRandTimeLimitMs);
            return algorithm.solve(data);
        }
        case AlgorithmKind::NearestNeighbor: {
            NearestNeighborAlgorithm algorithm;
            return algorithm.solve(data);
        }
        case AlgorithmKind::RepetitiveNN: {
            RepetitiveNNAlgorithm algorithm;
            return algorithm.solve(data);
        }
        default:
            throw std::runtime_error("Unknown benchmark algorithm.");
    }
}

// buildSharedSmallPoints: produce a small set of N values shared across algorithms
// - Chooses a few small N near the brute-force max for consistent references
std::vector<int> Benchmark::buildSharedSmallPoints(int bfMaxSize) const {
    std::vector<int> points;
    const int maxN = std::max(2, bfMaxSize);
    for (int offset = 2; offset >= 0; --offset) {
        const int candidate = std::max(2, maxN - offset);
        if (points.empty() || points.back() != candidate) {
            points.push_back(candidate);
        }
    }
    return points;
}

// buildExtendedIdealPoints: find a few intermediate points between
// fromSize and toSize (exclusive) using ratios
// - Produces up to 4 points to sample larger problem sizes evenly
std::vector<int> Benchmark::buildExtendedIdealPoints(int fromSize, int toSize) const {
    std::vector<int> points;
    if (toSize <= fromSize) {
        return points;
    }

    for (int i = 1; i <= 4; ++i) {
        const double ratio = static_cast<double>(i) / 5.0;
        const double value = static_cast<double>(fromSize) + (static_cast<double>(toSize - fromSize) * ratio);
        points.push_back(static_cast<int>(std::lround(value)));
    }

    return points;
}

// Mapping evenly spread points to actual files
std::pair<int, Benchmark::DatasetDescriptor> Benchmark::mapToNearestDataset(
        int targetSize,
        const std::map<int, DatasetDescriptor>& datasetBySize) const {
    if (datasetBySize.empty()) {
        throw std::runtime_error("TSPLIB file database is empty - no points to map.");
    }

    // Find the nearest available dataset size to the targetSize
    auto it = datasetBySize.lower_bound(targetSize);
    if (it == datasetBySize.begin()) {
        return *it;
    }
    if (it == datasetBySize.end()) {
        const auto prev = std::prev(it);
        return *prev;
    }

    const auto right = it;
    const auto left = std::prev(it);
    const int leftDist = std::abs(targetSize - left->first);
    const int rightDist = std::abs(right->first - targetSize);
    return (leftDist <= rightDist) ? *left : *right;
}

// Function that builds MeasurementPoints for algorithms
std::vector<Benchmark::MeasurementPoint> Benchmark::buildPointsForAlgorithm(
        AlgorithmKind kind,
        int algorithmMaxSize,
        const std::vector<int>& sharedSmallPoints,
        const std::map<int, DatasetDescriptor>& datasetBySize,
        const std::map<int, long long>& smallPointBfCosts,
        int baseSize) const {

    // Build the list of measurement points (requested size, mapped size, source)
    std::vector<MeasurementPoint> points;

    for (const int n : sharedSmallPoints) {
        const auto itReference = smallPointBfCosts.find(n);
        const long long reference = (itReference != smallPointBfCosts.end()) ? itReference->second : -1;
        points.push_back({n, n, std::string("TRUNC(") + kCalibrationFileName + ")", kCalibrationFileName, true, reference});
    }

    const int cappedMax = std::min(algorithmMaxSize, baseSize);
    const std::vector<int> idealLargePoints = buildExtendedIdealPoints(sharedSmallPoints.back(), cappedMax);

    // For brute-force prefer truncated calibration base points
    if (kind == AlgorithmKind::BruteForce) {
        for (const int idealN : idealLargePoints) {
            if (std::find_if(points.begin(), points.end(), [idealN](const MeasurementPoint& p){
                    return p.mappedSize == idealN && p.useTruncatedBase;
                }) != points.end()) {
                continue;
            }
            points.push_back({idealN,
                              idealN,
                              std::string("TRUNC(") + kCalibrationFileName + ")",
                              kCalibrationFileName,
                              true,
                              -1});
        }

        int fillN = std::max(2, sharedSmallPoints.front() - 1);
        while (points.size() < 7 && fillN >= 2) {
            if (std::find_if(points.begin(), points.end(), [fillN](const MeasurementPoint& p) {
                    return p.mappedSize == fillN && p.useTruncatedBase;
                }) == points.end()) {
                points.push_back({fillN,
                                  fillN,
                                  std::string("TRUNC(") + kCalibrationFileName + ")",
                                  kCalibrationFileName,
                                  true,
                                  -1});
            }
            --fillN;
        }
    } else {
        // For heuristic algorithms map ideal sizes to nearest TSPLIB instance files
        for (const int idealN : idealLargePoints) {
            const auto [mappedSize, descriptor] = mapToNearestDataset(idealN, datasetBySize);
            points.push_back({idealN, mappedSize, descriptor.fileName, descriptor.fileName, false, descriptor.knownOptimum});
        }
    }

    std::sort(points.begin(), points.end(), [](const MeasurementPoint& lhs, const MeasurementPoint& rhs) {
        return lhs.mappedSize < rhs.mappedSize;
    });

    if (points.size() > 7) {
        points.resize(7);
    }

    return points;
}

// runRandConvergenceTest: test RAND heuristic convergence across time limits
// - Uses a fixed test instance (targetSize) and a set of time limits
// - For each time limit runs kRandConvergenceTrials independent runs and records averages
void Benchmark::runRandConvergenceTest(std::vector<BenchmarkRow>& rows,
                                       const std::map<int, DatasetDescriptor>& datasetBySize) const {

    std::cout << "\n[Benchmark][RAND-CONV] Starting RAND convergence test\n";

    const int targetSize = 48;
    const auto itDataset = datasetBySize.find(targetSize);
    if (itDataset == datasetBySize.end()) {
        throw std::runtime_error("Required instance ftv47.atsp (N=48) not found in available files base.");
    }

    const DatasetDescriptor& descriptor = itDataset->second;
    if (!resolveDataPath(descriptor.fileName).has_value()) {
        throw std::runtime_error("File not found: " + descriptor.fileName + ".");
    }

    const TSPData randData = loadFromFileName(descriptor.fileName);
    const std::vector<int> timeLimitsMs = {1, 2, 5, 10, 50, 100, 500, 1000, 5000};

    std::cout << "[Benchmark][RAND-CONV] Test file: " << descriptor.fileName << " | N=" << randData.size
              << " | optimum=" << descriptor.knownOptimum << "\n";
    std::cout << "[Benchmark][RAND-CONV] Time limits [ms]: ";
    for (size_t i = 0; i < timeLimitsMs.size(); ++i) {
        std::cout << timeLimitsMs[i] << (i + 1 == timeLimitsMs.size() ? "\n" : ", ");
    }

    // Adding random seed
    std::random_device rd;
    std::mt19937 seedGenerator(rd());
    std::uniform_int_distribution<std::uint32_t> seedDistribution;

    for (const int limitMs : timeLimitsMs) {
        long long totalTimeUs = 0;
        long long totalCost = 0;
        long long bestCost = std::numeric_limits<long long>::max();

        // Run test and collect data
        for (int trial = 0; trial < kRandConvergenceTrials; ++trial) {
            const std::uint32_t trialSeed = seedDistribution(seedGenerator);
            RandomSearchAlgorithm algorithm(limitMs, trialSeed);
            const Result result = algorithm.solve(randData);
            totalTimeUs += result.executionTimeMicroseconds;
            totalCost += result.minCost;
            bestCost = std::min(bestCost, static_cast<long long>(result.minCost));
        }

        // Calculate averages
        const long long averageTimeUs = totalTimeUs / kRandConvergenceTrials;
        const double averageCost = static_cast<double>(totalCost) / static_cast<double>(kRandConvergenceTrials);
        const double relativeError = descriptor.knownOptimum > 0
                                             ? ((averageCost - static_cast<double>(descriptor.knownOptimum)) /
                                                static_cast<double>(descriptor.knownOptimum)) *
                                                    100.0
                                             : 0.0;
        const std::string limitLabel = "Limit_" + std::to_string(limitMs) + "ms";

        // Record aggregated row for this time limit
        rows.push_back({descriptor.fileName,
                        "RAND",
                        static_cast<int>(randData.size),
                        averageTimeUs,
                        averageCost,
                        (bestCost == std::numeric_limits<long long>::max()) ? -1 : bestCost,
                        kRandConvergenceTrials,
                        descriptor.knownOptimum,
                        relativeError,
                        limitLabel});

        std::cout << "[Benchmark][RAND-CONV] N=" << randData.size << " | file=" << descriptor.fileName
                  << " | trials=" << kRandConvergenceTrials << " | limit=" << limitMs << "ms"
                  << " | avg_time_us=" << averageTimeUs << " | avg_cost=" << std::fixed << std::setprecision(3)
                  << averageCost << " | err_%=" << relativeError << std::defaultfloat << "\n";
    }

    std::cout << "[Benchmark][RAND-CONV] End of RAND convergence test\n";
}

// Write rows into csv file
void Benchmark::writeCsv(const std::vector<BenchmarkRow>& rows) const {
    std::ofstream outFile("benchmark_results.csv");
    if (!outFile) {
        std::cout << "[Benchmark] Failed to save benchmark_results.csv\n";
        return;
    }

    std::cout << "[Benchmark] CSV save: benchmark_results.csv | row count=" << rows.size() << "\n";

    outFile << "Source;Algorithm;N;Avg_Time_us;Avg_Cost;Best_Cost;Trial_Count;Reference_Cost;Relative_Error_%25;Parameters\n";
    outFile << std::fixed << std::setprecision(3);

    for (const BenchmarkRow& row : rows) {
        outFile << row.source << ';' << row.algorithm << ';' << row.size << ';' << row.averageTimeMicroseconds << ';'
                << row.averageCost << ';' << row.bestCost << ';' << row.trialsCount << ';' << row.referenceCost << ';'
                << row.relativeErrorPercent << ';' << row.parameters << '\n';
    }

    std::cout << "[Benchmark] CSV save completed successfully.\n";
}
