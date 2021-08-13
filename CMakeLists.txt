project(fw-analyzer)
cmake_minimum_required(VERSION 3.20)

include(ExternalProject)
include(FetchContent)
FetchContent_Declare(rapidcheck
	PREFIX rapidcheck
	GIT_REPOSITORY https://github.com/emil-e/rapidcheck.git
	GIT_PROGRESS true
	GIT_SHALLOW true
	CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/rapidcheck
)

include(ExternalProject)
FetchContent_Declare(rapidyaml
	PREFIX rapidyaml
	GIT_REPOSITORY https://github.com/biojppm/rapidyaml.git
	# GIT_TAG v0.1.0
	GIT_PROGRESS true
	GIT_SHALLOW true
	CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/rapidyaml -DYAML_BUILD_SHARED_LIBS=ON
)

FetchContent_MakeAvailable(rapidyaml rapidcheck)

# file(DOWNLOAD https://raw.githubusercontent.com/p-ranav/argparse/v2.1/include/argparse.hpp external/argparse/argparse.hpp)
file(DOWNLOAD https://raw.githubusercontent.com/p-ranav/argparse/master/include/argparse/argparse.hpp external/argparse/argparse.hpp)
file(DOWNLOAD https://raw.githubusercontent.com/p-ranav/tabulate/v1.4/single_include/tabulate/tabulate.hpp external/tabulate/tabulate.hpp)

include_directories(${rapidcheck_SOURCE_DIR}/include)
include_directories(${rapidcheck_SOURCE_DIR}/extras/gtest/include)
include_directories(${rapidyaml_SOURCE_DIR}/src)
include_directories(${rapidyaml_SOURCE_DIR}/ext/c4core/src)
include_directories("${CMAKE_CURRENT_BINARY_DIR}/external")

add_compile_options(
	"-Wall" "-Wpedantic" "-Wextra" "-fexceptions" "-fopenmp" "-fconcepts-diagnostics-depth=20"
	"$<$<CONFIG:RELEASE>:-O3>"
	"$<$<CONFIG:DEBUG>:-O0;-g3;-ggdb;-ftrapv;-fbounds-check;-fsanitize=undefined;-fsanitize-undefined-trap-on-error>"
	)
# set(CMAKE_CXX_CLANG_TIDY "clang-tidy;-checks=*,-llvmlibc-*,-fuchsia-*,-abseil-*,-cppcoreguidelines-avoid-non-const-global-variables")
find_package(fmt REQUIRED)
include_directories(fmt)

set(CMAKE_CXX_STANDARD 20)


add_executable(analyzer
	src/main.cpp
	src/parser/common.cpp
	src/parser/IpSet.cpp
	src/config.hpp
	src/SegmentSet.cpp
	src/RulesetParser.cpp
	src/config.cpp
	src/util.cpp
	src/log.cpp
	src/IpAnalyzer.cpp
	src/Ruleset.cpp
	src/args.cpp)
add_dependencies(analyzer ryml)
target_link_libraries(analyzer fmt gmp omp ${rapidyaml_BINARY_DIR}/libryml.a)


find_package(GTest)
if(GTest_FOUND)
	include_directories(${GTEST_INCLUDE_DIRS})
	add_executable(test
		src/SegmentSet.cpp
		src/log.cpp
		src/util.cpp
		src/util.test.cpp
		src/IpAnalyzer.test.cpp
		src/RulesetParser.cpp
		src/parser/common.cpp
		src/parser/IpSet.cpp
		src/IpAnalyzer.cpp
		src/Ruleset.cpp
		src/vector.test.cpp
		src/Segment.test.cpp
		src/SegmentSet.test.cpp)
	target_link_libraries(test ${GTEST_BOTH_LIBRARIES} pthread fmt gmp omp)
	add_dependencies(test rapidcheck)
	target_link_libraries(test ${rapidcheck_BINARY_DIR}/librapidcheck.a)
endif()

find_package(benchmark)
if(benchmark_FOUND)
	add_executable(intersection_bench
		src/SegmentSet.cpp
		src/intersection_bench.cpp)
	target_link_libraries(intersection_bench ${GTEST_BOTH_LIBRARIES} pthread fmt gmp omp benchmark::benchmark ${rapidcheck_BINARY_DIR}/librapidcheck.a)
	add_dependencies(intersection_bench rapidcheck)
	add_executable(negated_bench
		src/SegmentSet.cpp
		src/negated_bench.cpp)
	target_link_libraries(negated_bench ${GTEST_BOTH_LIBRARIES} pthread fmt gmp omp benchmark::benchmark ${rapidcheck_BINARY_DIR}/librapidcheck.a)
	add_dependencies(negated_bench rapidcheck)
	add_executable(union_bench
		src/SegmentSet.cpp
		src/union_bench.cpp)
	target_link_libraries(union_bench ${GTEST_BOTH_LIBRARIES} pthread fmt gmp omp benchmark::benchmark ${rapidcheck_BINARY_DIR}/librapidcheck.a)
	add_dependencies(union_bench rapidcheck)

	add_executable(intersection_negated_bench
		src/SegmentSet.cpp
		src/intersection_negated_bench.cpp)
	add_dependencies(intersection_negated_bench rapidcheck)
	target_link_libraries(intersection_negated_bench ${GTEST_BOTH_LIBRARIES} pthread fmt gmp omp benchmark::benchmark)
	target_link_libraries(intersection_negated_bench ${rapidcheck_BINARY_DIR}/librapidcheck.a)
endif()
