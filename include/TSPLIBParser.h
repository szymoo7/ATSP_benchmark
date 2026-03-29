#pragma once

#include "TSPData.h"

#include <string>
#include <vector>

// TSPLIB parser: reads ATSP/TSP files and returns TSPData
// Helper functions perform trimming, uppercasing and integer extraction
class TSPLIBParser {
public:
    [[nodiscard]] static TSPData parse(const std::string& filePath);

private:
    // trim: remove leading/trailing whitespace
    [[nodiscard]] static std::string trim(const std::string& text);

    // toUpper: convert text to uppercase for keyword matching
    [[nodiscard]] static std::string toUpper(const std::string& text);

    // containsAlpha: check if string has alphabetic characters
    [[nodiscard]] static bool containsAlpha(const std::string& text);

    // extractIntegers: parse all integers from a text line
    [[nodiscard]] static std::vector<int> extractIntegers(const std::string& text);
};
