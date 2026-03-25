cmake_minimum_required(VERSION 3.31)
project(PEA_project1)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(PEA_project1
		main.cpp
		src/AlgorithmUtils.cpp
		src/Benchmark.cpp
		src/BruteForceAlgorithm.cpp
		src/Menu.cpp
		src/NearestNeighborAlgorithm.cpp
		src/RandomSearchAlgorithm.cpp
		src/RepetitiveNNAlgorithm.cpp
		src/Timer.cpp
		src/TSPLIBParser.cpp
)

target_include_directories(PEA_project1 PRIVATE include)
