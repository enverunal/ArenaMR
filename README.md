# Polymorphic memory resource wrapper to reduce access to upstream memory allocator 

**ArenaMR** is a header-only library written in C++17.
The allocation is constant if there is free arena and allocated space is not greater than the given arena size(else it allocates new arena from the upstream and inserts new arena to internal map with complexity O(log n)). Deallocation complexity is O(log n) (binary search in the map).

### How it works

User decides the preallocated *size per arena*, *number of arenas* and *upstream allocator*.

Initially every arena is empty and they are placed in a list of free arenas. There is always one arena which is marked as active. All requested allocations takes place from the active arena until there is an allocation that is greater than the remaining space in the active arena. In this case, active arena is declared full and the next free arena is tapped from the list and marked as active.

Arenas know how many allocations they hold. When deallocation happens the allocation count is decremented. When the counter is zero, the arena returns to the free arena list.

If user cause to allocate more than *size per arena* ArenaMR allocates new arena with the required size from *upstream allocator*.

If user cause more allocations than the preallocated memory, ArenaMR allocates new arena from  *upstream allocator*.

### How to use

```c++
    arena_mr::UnsynchronizedArenaMR arena_resource(10, 10'000, std::pmr::new_delete_resource());
    std::pmr::vector<int> v(&arena_resource);
    v.push_back(123);
```
Or

```c++
    std::pmr::monotonic_buffer_resource monotonic_br(150'000); // Could be any pmr resource
    arena_mr::UnsynchronizedArenaMR arena_resource(10, 10'000, &monotonic_br);
    std::pmr::vector<int> v(&arena_resource);
    v.push_back(123);
```

### How to compile examples

Create a build file. Inside build file first run `cmake -DCMAKE_BUILD_TYPE=Release PATH_TO_/ArenaMR/examples/` and then Run `cmake --build . --config Release`.

## Benchmark

[benchmark1.cpp](examples/benchmark1.cpp) is a benchmark for many allocations and deallocations. 
[benchmark2.cpp](examples/benchmark2.cpp) is the similar to benchmark1 but it does not clear the map after insertions.

Results are highly depend on how you tune you ArenaMR. If you keep your arenas small and make bigger allocations than the *size per arena* then ArenaMR is equal to upstream allocator with extra steps. So do not forget to tune for arena options.

My results for benchmark1:

|                                     | Windows 11 with msvc | (WSL2) Ubuntu-22.04 gcc 12 |
| :-                                  | :-                   | :-                         |
| UnsynchronizedArenaMR_BENCHMARK     | 22697600 ns          | 26496114 ns                |
| new_delete_resource_BENCHMARK       | 38299800 ns          | 43735875 ns                |
| monotonic_buffer_resource_BENCHMARK | 31345000 ns          | 37285919 ns                |

My results for benchmark2:

|                                     | Windows 11 with msvc | (WSL2) Ubuntu-22.04 gcc 12 |
| :-                                  | :-                   | :-                         |
| UnsynchronizedArenaMR_BENCHMARK     | 6893900 ns           | 4684561 ns                 |
| new_delete_resource_BENCHMARK       | 7163100 ns           | 6900841 ns                 |
| monotonic_buffer_resource_BENCHMARK | 7785800 ns           | 6221367 ns                 |

In benchmark1 when `ArenaMR` is advantageous to `monotonic_buffer_resource` because `ArenaMR` can reuse the same space after clear but  `monotonic_buffer_resource` cannot.