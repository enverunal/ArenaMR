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
        arena_mr::UnsynchronizedArenaMR memory_resource(10, 10'000); // tune your arena

        steady_clock::time_point begin = steady_clock::now();

        {
            std::pmr::vector<std::string> x1(&memory_resource);
            std::pmr::unordered_map<std::string, std::vector<std::string>> x2(&memory_resource);
            std::pmr::map<std::string, std::vector<std::string>> x3(&memory_resource);
            for (int i = 0; i < 100'000; ++i)
            {
                x1.emplace_back(std::to_string(i));
                x2.emplace(std::to_string(i), std::vector{std::to_string(i)});
                x3.emplace(std::to_string(i), std::vector{std::to_string(i)});
            }
        }
        steady_clock::time_point end = steady_clock::now();
        std::cout << "Time difference = " << duration_cast<nanoseconds>(end - begin).count() << "[ns]" << std::endl;
    }

    {
        auto *memory_resource = std::pmr::new_delete_resource();

        steady_clock::time_point begin = steady_clock::now();

        {
            std::pmr::vector<std::string> x1(memory_resource);
            std::pmr::unordered_map<std::string, std::vector<std::string>> x2(memory_resource);
            std::pmr::map<std::string, std::vector<std::string>> x3(memory_resource);
            for (int i = 0; i < 100'000; ++i)
            {
                x1.emplace_back(std::to_string(i));
                x2.emplace(std::to_string(i), std::vector{std::to_string(i)});
                x3.emplace(std::to_string(i), std::vector{std::to_string(i)});
            }
        }
        steady_clock::time_point end = steady_clock::now();
        std::cout << "Time difference = " << duration_cast<nanoseconds>(end - begin).count() << "[ns]" << std::endl;
    }
}
