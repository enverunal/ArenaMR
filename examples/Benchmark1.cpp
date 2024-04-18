#include "ArenaMR/ArenaMR.hpp"
#include "BenchmarkUtility.hpp"

#include <chrono>
#include <iostream>

using namespace std::chrono;

static uint64_t UnsynchronizedArenaMR_BENCHMARK()
{
    arena_mr::UnsynchronizedArenaMR memory_resource(10, 100'000); // tune your arena
    std::pmr::map<int, int> v(&memory_resource);

    steady_clock::time_point begin = steady_clock::now();

    // This for loop will not cause arena_mr to reallocate new space because it can reuse its deallocated space
    for (int i = 0; i < 10; ++i)
    {
        for (int j = 0; j < 100'000; ++j)
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

    for (int i = 0; i < 10; ++i)
    {
        for (int j = 0; j < 100'000; ++j)
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
        std::pmr::pool_options{/*.max_blocks_per_chunk =*/150'000, /*.largest_required_pool_block =*/40},
        std::pmr::new_delete_resource());
    std::pmr::map<int, int> v(&memory_resource);

    steady_clock::time_point begin = steady_clock::now();

    for (int i = 0; i < 10; ++i)
    {
        for (int j = 0; j < 100'000; ++j)
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
    std::pmr::monotonic_buffer_resource memory_resource(1'000'000);
    std::pmr::map<int, int> v(&memory_resource);

    steady_clock::time_point begin = steady_clock::now();

    // This for loop will cause monotonic buffer to reallocate new space
    for (int i = 0; i < 10; ++i)
    {
        for (int j = 0; j < 100'000; ++j)
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
    const int avg_count = 10;

    for (int i = 0; i < warm_count; ++i)
    {
        UnsynchronizedArenaMR_BENCHMARK();
        new_delete_resource_BENCHMARK();
        unsynchronized_pool_resource_BENCHMARK();
        monotonic_buffer_resource_BENCHMARK();
    }

    uint64_t UnsynchronizedArenaMR_count = 0;
    uint64_t new_delete_resource_count = 0;
    uint64_t unsynchronized_pool_resource_count = 0;
    uint64_t monotonic_buffer_resource_count = 0;

    for (int i = 0; i < avg_count; ++i)
    {
        UnsynchronizedArenaMR_count += UnsynchronizedArenaMR_BENCHMARK();
        new_delete_resource_count += new_delete_resource_BENCHMARK();
        unsynchronized_pool_resource_count += unsynchronized_pool_resource_BENCHMARK();
        monotonic_buffer_resource_count += monotonic_buffer_resource_BENCHMARK();
    }

    std::cout << "UnsynchronizedArenaMR_BENCHMARK: " << UnsynchronizedArenaMR_count / avg_count << "[ns]" << std::endl;
    std::cout << "new_delete_resource_BENCHMARK: " << new_delete_resource_count / avg_count << "[ns]" << std::endl;
    std::cout << "unsynchronized_pool_resource_BENCHMARK: " << unsynchronized_pool_resource_count / avg_count << "[ns]" << std::endl;
    std::cout << "monotonic_buffer_resource_BENCHMARK: " << monotonic_buffer_resource_count / avg_count << "[ns]" << std::endl;
}