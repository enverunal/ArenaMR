#include "ArenaMR/ArenaMR.hpp"

#include <array>
#include <chrono>
#include <iostream>
#include <map>
#include <memory_resource>
#include <string>
#include <thread>
#include <unordered_map>
#include <map>

int main()
{
    arena_mr::UnsynchronizedArenaMR arena_resource(10, 10'000);

    {
        std::pmr::vector<int> x(&arena_resource);
        for (int i = 0; i < 240; ++i)
        {
            x.push_back(i);
        }
        std::cout << "Wasted Memory " << arena_resource.WastedMemory() << std::endl;
        std::cout << "Used Memory " << arena_resource.UsedMemory() << std::endl;
    }

    std::cout << "Free arena size: " << arena_resource.FreeArenaSize() << std::endl;
    std::cout << "Used Memory " << arena_resource.UsedMemory() << std::endl;

    {
        std::pmr::map<int, std::vector<std::string>> x(&arena_resource);
        for (int i = 0; i < 240; ++i)
        {
            x.emplace(i, std::vector{std::to_string(i)});
        }
        std::cout << "Wasted Memory " << arena_resource.WastedMemory() << std::endl;
        std::cout << "Used Memory " << arena_resource.UsedMemory() << std::endl;
    }

    std::cout << "Free arena size: " << arena_resource.FreeArenaSize() << std::endl;
    std::cout << "Used Memory " << arena_resource.UsedMemory() << std::endl;

    {
        std::pmr::unordered_map<int, std::vector<std::string>> x(&arena_resource);
        for (int i = 0; i < 240; ++i)
        {
            x.emplace(i, std::vector{std::to_string(i)});
        }
        std::cout << "Wasted Memory " << arena_resource.WastedMemory() << std::endl;
        std::cout << "Used Memory " << arena_resource.UsedMemory() << std::endl;
    }

    std::cout << "Free arena size: " << arena_resource.FreeArenaSize() << std::endl;
    std::cout << "Used Memory " << arena_resource.UsedMemory() << std::endl;

    {
        std::pmr::vector<std::string> x1(&arena_resource);
        std::pmr::unordered_map<std::string, std::vector<std::string>> x2(&arena_resource);
        std::pmr::map<std::string, std::vector<std::string>> x3(&arena_resource);
        for (int i = 0; i < 100'000; ++i)
        {
            x1.emplace_back(std::to_string(i));
            x2.emplace(std::to_string(i), std::vector{std::to_string(i)});
            x3.emplace(std::to_string(i), std::vector{std::to_string(i)});
        }
        std::cout << "Wasted Memory " << arena_resource.WastedMemory() << std::endl;
        std::cout << "Used Memory " << arena_resource.UsedMemory() << std::endl;
    }

    std::cout << "Free arena size: " << arena_resource.FreeArenaSize() << std::endl;
    std::cout << "Used Memory " << arena_resource.UsedMemory() << std::endl;
}
