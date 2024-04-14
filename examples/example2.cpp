#include "ArenaMR/ArenaMR.hpp"

#include <array>
#include <chrono>
#include <iostream>
#include <map>
#include <memory_resource>
#include <string>
#include <thread>
#include <unordered_map>

int main()
{
    using namespace std::chrono;

    {
        arena_mr::UnsynchronizedArenaMR memory_resource(10, 100'000); // tune your arena

        steady_clock::time_point begin = steady_clock::now();

        {
            std::pmr::map<std::string, std::vector<std::string>> v(&memory_resource);
            for (int i = 0; i < 1'000'000; ++i)
            {
                v.emplace(std::to_string(i), std::vector{std::to_string(i)});
            }
        }
        steady_clock::time_point end = steady_clock::now();
        std::cout << "UnsynchronizedArenaMR: " << duration_cast<nanoseconds>(end - begin).count() << "[ns]" << std::endl;
    }

    {
        auto *memory_resource = std::pmr::new_delete_resource();

        steady_clock::time_point begin = steady_clock::now();

        {
            std::pmr::map<std::string, std::vector<std::string>> v(memory_resource);
            for (int i = 0; i < 1'000'000; ++i)
            {
                v.emplace(std::to_string(i), std::vector{std::to_string(i)});
            }
        }
        steady_clock::time_point end = steady_clock::now();
        std::cout << "new_delete_resource: " << duration_cast<nanoseconds>(end - begin).count() << "[ns]" << std::endl;
    }

    {
        std::pmr::monotonic_buffer_resource memory_resource(1'000'000);

        steady_clock::time_point begin = steady_clock::now();

        {
            std::pmr::map<std::string, std::vector<std::string>> v(&memory_resource);
            for (int i = 0; i < 1'000'000; ++i)
            {
                v.emplace(std::to_string(i), std::vector{std::to_string(i)});
            }
        }
        steady_clock::time_point end = steady_clock::now();
        std::cout << "monotonic_buffer_resource: " << duration_cast<nanoseconds>(end - begin).count() << "[ns]" << std::endl;
    }
}
