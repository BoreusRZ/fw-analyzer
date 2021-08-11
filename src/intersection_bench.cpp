#include <benchmark/benchmark.h>
#include "SegmentSet-generator.hpp"
#include <omp.h>
#include <iostream>

PSET generate_test_set(){
	rc::Random r(0);
	auto generator = rc::gen::arbitrary<PSET>();
	auto ret_val = generator(r,10000).value();
	std::cout << "set size = " << ret_val.segments.size() << std::endl;
	return ret_val;
}
static PSET test_set = generate_test_set();
static void BM_par_INTERSECTION(benchmark::State& state){
	omp_set_num_threads(state.range(0));
	for(auto _ : state){
		auto result = test_set;
		result.INTERSECTION_par(test_set);
		benchmark::DoNotOptimize(result);
	}
	benchmark::DoNotOptimize(test_set);
}
static void BM_seq_INTERSECTION(benchmark::State& state){
	for(auto _ : state){
		auto result = test_set;
		result.INTERSECTION_seq(test_set);
		benchmark::DoNotOptimize(result);
	}
	benchmark::DoNotOptimize(test_set);
}
BENCHMARK(BM_seq_INTERSECTION);
BENCHMARK(BM_par_INTERSECTION)->DenseRange(1,8);
BENCHMARK_MAIN();
