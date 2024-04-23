#include "ArenaMR/ArenaMR.hpp"
#include "BenchmarkUtility.hpp"

#include <chrono>
#include <iostream>
#include <map>

using namespace std::chrono;

static uint64_t UnsynchronizedArenaMR_BENCHMARK()
{
    arena_mr::UnsynchronizedArenaMR memory_resource(10, 10'000'000); // tune your arena
    std::pmr::map<int, int> v(&memory_resource);

    steady_clock::time_point begin = steady_clock::now();

    // This for loop will not cause arena_mr to reallocate new space because it can reuse its deallocated space
    for (int i = 0; i < 100; ++i)
    {
        for (int j = 0; j < 1'000; ++j)
        {
            v.emplace(j, j);
        }
        v.clear();
    }

    steady_clock::time_point end = steady_clock::now();
    return duration_cast<nanoseconds>(end - begin).count();
}

static uint64_t new_delete_resource_BENCHMARK()
{
    auto *memory_resource = std::pmr::new_delete_resource();
    std::pmr::map<int, int> v(memory_resource);
    steady_clock::time_point begin = steady_clock::now();

    for (int i = 0; i < 100; ++i)
    {
        for (int j = 0; j < 1'000; ++j)
        {
            v.emplace(j, j);
        }
        v.clear();
    }

    steady_clock::time_point end = steady_clock::now();
    return duration_cast<nanoseconds>(end - begin).count();
}

static uint64_t unsynchronized_pool_resource_BENCHMARK()
{
    std::pmr::unsynchronized_pool_resource memory_resource(
        // max_blocks_per_chunk should be 1, largest_required_pool_block should be 40 (I guess)
        std::pmr::pool_options{/*.max_blocks_per_chunk =*/1'500, /*.largest_required_pool_block =*/40},
        std::pmr::new_delete_resource());
    std::pmr::map<int, int> v(&memory_resource);

    steady_clock::time_point begin = steady_clock::now();

    for (int i = 0; i < 100; ++i)
    {
        for (int j = 0; j < 1'000; ++j)
        {
            v.emplace(j, j);
        }
        v.clear();
    }

    steady_clock::time_point end = steady_clock::now();
    return duration_cast<nanoseconds>(end - begin).count();
}

static uint64_t monotonic_buffer_resource_BENCHMARK()
{
    std::pmr::monotonic_buffer_resource memory_resource(1'000'000'000);
    std::pmr::map<int, int> v(&memory_resource);

    steady_clock::time_point begin = steady_clock::now();

    // This for loop will not cause `monotonic_buffer_resouce` to reallocate because it does not reach to the end.
    for (int i = 0; i < 100; ++i)
    {
        for (int j = 0; j < 1'000; ++j)
        {
            v.emplace(j, j);
        }
        v.clear();
    }

    steady_clock::time_point end = steady_clock::now();
    return duration_cast<nanoseconds>(end - begin).count();
}

int main()
{
    SetThreadAffinity(7);

    const int warm_count = 10;
    const int avg_count = 100;

    auto UnsynchronizedArenaMR_avg_time = WarmAndRun(warm_count, avg_count, UnsynchronizedArenaMR_BENCHMARK);
    auto new_delete_resource_avg_time = WarmAndRun(warm_count, avg_count, new_delete_resource_BENCHMARK);
    auto unsynchronized_pool_resource_avg_time = WarmAndRun(warm_count, avg_count, unsynchronized_pool_resource_BENCHMARK);
    auto monotonic_buffer_resource_avg_time = WarmAndRun(warm_count, avg_count, monotonic_buffer_resource_BENCHMARK);

    std::cout << "UnsynchronizedArenaMR_BENCHMARK: " << UnsynchronizedArenaMR_avg_time << "[ns]" << std::endl;
    std::cout << "new_delete_resource_BENCHMARK: " << new_delete_resource_avg_time << "[ns]" << std::endl;
    std::cout << "unsynchronized_pool_resource_BENCHMARK: " << unsynchronized_pool_resource_avg_time << "[ns]" << std::endl;
    std::cout << "monotonic_buffer_resource_BENCHMARK: " << monotonic_buffer_resource_avg_time << "[ns]" << std::endl;
}