#include <benchmark/benchmark.h>
#include "SegmentSet-generator.hpp"
#include <omp.h>
#include <iostream>

PSET generate_test_set(int seed, int size){
	rc::Random r(seed);
	auto generator = rc::gen::arbitrary<PSET>();
	auto ret_val = generator(r,size).value();
	std::cout << "set size = " << ret_val.segments.size() << std::endl;
	return ret_val;
}
static PSET rhs = generate_test_set(232,100);
static void BM_par_NEGATED(benchmark::State& state){
	omp_set_num_threads(state.range(0));
	for(auto _ : state){
		auto result = rhs;
		result.NEGATE_par();
		benchmark::DoNotOptimize(result);
	}
	benchmark::DoNotOptimize(rhs);
}
static void BM_seq_NEGATED(benchmark::State& state){
	for(auto _ : state){
		auto result = rhs;
		result.NEGATE_seq();
		benchmark::DoNotOptimize(result);
	}
	benchmark::DoNotOptimize(rhs);
}
BENCHMARK(BM_seq_NEGATED);
BENCHMARK(BM_par_NEGATED)->DenseRange(1,8);
BENCHMARK_MAIN();
