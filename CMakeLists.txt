cmake_minimum_required(VERSION 3.31)
project(PEA_project1)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static -static-libgcc -static-libstdc++")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -static -static-libgcc")

add_executable(PEA_project1
		main.cpp
		src/AlgorithmUtils.cpp
		src/BenchmarkBnB.cpp
		src/Benchmark.cpp
		src/BranchAndBoundAlgorithm.cpp
		src/BranchAndBoundBFSAlgorithm.cpp
		src/BranchAndBoundDFSAlgorithm.cpp
		src/BranchAndBoundLowestCostAlgorithm.cpp
		src/bnb/BnBContainers.cpp
		src/bnb/BnBNode.cpp
		src/BruteForceAlgorithm.cpp
		src/Menu.cpp
		src/NearestNeighborAlgorithm.cpp
		src/RandomSearchAlgorithm.cpp
		src/RepetitiveNNAlgorithm.cpp
		src/Timer.cpp
		src/TSPLIBParser.cpp
)

target_include_directories(PEA_project1 PRIVATE include)
