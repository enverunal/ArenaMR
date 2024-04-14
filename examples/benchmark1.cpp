#include "ArenaMR/ArenaMR.hpp"

#include <chrono>
#include <iostream>

using namespace std::chrono;

static void UnsynchronizedArenaMR_BENCHMARK()
{
    arena_mr::UnsynchronizedArenaMR memory_resource(10, 100'000); // tune your arena
    std::pmr::map<int, int> v(&memory_resource);

    steady_clock::time_point begin = steady_clock::now();

    for (int j = 0; j < 10; ++j)
    {
        for (int i = 0; i < 100'000; ++i)
        {
            v.emplace(i, i);
            ++i;
        }
        v.clear();
    }

    steady_clock::time_point end = steady_clock::now();
    std::cout << "UnsynchronizedArenaMR_BENCHMARK: " << duration_cast<nanoseconds>(end - begin).count() << "[ns]" << std::endl;
}

static void new_delete_resource_BENCHMARK()
{
    auto *memory_resource = std::pmr::new_delete_resource();
    std::pmr::map<int, int> v(memory_resource);
    steady_clock::time_point begin = steady_clock::now();

    for (int j = 0; j < 10; ++j)
    {
        for (int i = 0; i < 100'000; ++i)
        {
            v.emplace(i, i);
            ++i;
        }
        v.clear();
    }

    steady_clock::time_point end = steady_clock::now();
    std::cout << "new_delete_resource_BENCHMARK: " << duration_cast<nanoseconds>(end - begin).count() << "[ns]" << std::endl;
}

static void monotonic_buffer_resource_BENCHMARK()
{
    std::pmr::monotonic_buffer_resource memory_resource(1'000'000);
    std::pmr::map<int, int> v(&memory_resource);

    steady_clock::time_point begin = steady_clock::now();

    for (int j = 0; j < 10; ++j)
    {
        for (int i = 0; i < 100'000; ++i)
        {
            v.emplace(i, i);
            ++i;
        }
        v.clear();
    }
    steady_clock::time_point end = steady_clock::now();
    std::cout << "monotonic_buffer_resource_BENCHMARK: " << duration_cast<nanoseconds>(end - begin).count() << "[ns]" << std::endl;
}

int main()
{
    UnsynchronizedArenaMR_BENCHMARK();
    new_delete_resource_BENCHMARK();
    monotonic_buffer_resource_BENCHMARK();
}