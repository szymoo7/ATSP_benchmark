#include "../include/TSPLIBParser.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <fstream>
#include <stdexcept>

// parse: read weights from TSPLIB file and build a TSPData instance
TSPData TSPLIBParser::parse(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file) {
        throw std::runtime_error("Cannot open the file: " + filePath);
    }

    std::size_t dimension = 0;
    bool inEdgeWeightSection = false;
    bool sawEdgeWeightSection = false;
    std::vector<int> weights;

    std::string line;
    // Read file line by line, collect integers from weight sections or plain matrices
    while (std::getline(file, line)) {
        const std::string trimmed = trim(line);
        if (trimmed.empty()) {
            continue;
        }

        const std::string upper = toUpper(trimmed);
        // stop at EOF marker if present
        if (upper.rfind("EOF", 0) == 0) {
            break;
        }

        // parse DIMENSION header if present
        if (upper.find("DIMENSION") != std::string::npos) {
            const auto ints = extractIntegers(trimmed);
            if (!ints.empty() && ints.back() > 0) {
                dimension = static_cast<std::size_t>(ints.back());
            }
            continue;
        }

        // detect explicit EDGE_WEIGHT_SECTION start
        if (upper.find("EDGE_WEIGHT_SECTION") != std::string::npos) {
            inEdgeWeightSection = true;
            sawEdgeWeightSection = true;
            continue;
        }

        // collect integers from either edge weight section or from plain numeric lines
        if (inEdgeWeightSection || (!sawEdgeWeightSection && !containsAlpha(upper))) {
            auto values = extractIntegers(trimmed);
            weights.insert(weights.end(), values.begin(), values.end());
        }
    }

    // If DIMENSION not provided, infer from number of weights (must be square)
    if (dimension == 0) {
        const auto candidate = static_cast<std::size_t>(std::sqrt(static_cast<long double>(weights.size())));
        if (candidate * candidate != weights.size()) {
            throw std::runtime_error("Missing valid DIMENSION and cannot infer size from data.");
        }
        dimension = candidate;
    }

    const std::size_t requiredValues = dimension * dimension;
    if (weights.size() < requiredValues) {
        throw std::runtime_error("Insufficient weights in TSPLIB file: expected " + std::to_string(requiredValues) +
                                 ", read " + std::to_string(weights.size()) + ".");
    }

    // Allocate dynamic matrix and copy values row-major
    TSPData data;
    data.size = dimension;
    data.distanceMatrix = new int*[dimension];
    std::size_t idx = 0;
    for (std::size_t i = 0; i < dimension; ++i) {
        data.distanceMatrix[i] = new int[dimension];
        for (std::size_t j = 0; j < dimension; ++j) {
            data.distanceMatrix[i][j] = weights[idx++];
        }
    }
    return data;
}

// trim: remove leading and trailing whitespace characters
std::string TSPLIBParser::trim(const std::string& text) {
    std::size_t begin = 0;
    while (begin < text.size() && std::isspace(static_cast<unsigned char>(text[begin])) != 0) {
        ++begin;
    }

    std::size_t end = text.size();
    while (end > begin && std::isspace(static_cast<unsigned char>(text[end - 1])) != 0) {
        --end;
    }
    return text.substr(begin, end - begin);
}

// toUpper: convert string to uppercase for simple keyword matching
std::string TSPLIBParser::toUpper(const std::string& text) {
    std::string result = text;
    for (char& ch : result) {
        ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
    }
    return result;
}

// containsAlpha: detect alphabetic characters to distinguish headers from data
bool TSPLIBParser::containsAlpha(const std::string& text) {
    return std::any_of(text.begin(), text.end(), [](char ch) {
        return std::isalpha(static_cast<unsigned char>(ch)) != 0;
    });
}

// extractIntegers: parse and return all integer values found in the input string
std::vector<int> TSPLIBParser::extractIntegers(const std::string& text) {
    std::vector<int> values;
    std::size_t i = 0;

    while (i < text.size()) {
        if (text[i] == '+' || text[i] == '-' || std::isdigit(static_cast<unsigned char>(text[i])) != 0) {
            std::size_t j = i;
            if (text[j] == '+' || text[j] == '-') {
                ++j;
            }

            const std::size_t digitBegin = j;
            while (j < text.size() && std::isdigit(static_cast<unsigned char>(text[j])) != 0) {
                ++j;
            }

            if (j > digitBegin) {
                values.push_back(std::stoi(text.substr(i, j - i)));
                i = j;
                continue;
            }
        }
        ++i;
    }

    return values;
}

