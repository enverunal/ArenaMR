#include "ArenaMR/ArenaMR.hpp"

#include <iostream>
#include <memory_resource>
#include <map>
#include <string>
#include <array>

int main()
{
    // std::pmr::monotonic_buffer_resource mbr(10'000);

    arena_mr::UnsynchronizedArenaMR arena_resource(10, 128);

    std::pmr::vector<int> x(&arena_resource);
    for (int i = 0; i < 240; ++i)
    {
        x.push_back(i);
        std::cout << "v: " << x.size() << " i: " << i << std::endl;
    }
}
