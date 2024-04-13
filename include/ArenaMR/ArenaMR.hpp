#ifndef ARENA_MR
#define ARENA_MR

#include <array>
#include <vector>
#include <map>
#include <numeric>
#include <atomic>
#include <mutex>
#include <type_traits>
#include <utility>
#include <functional>
#include <memory>
#include <memory_resource>
#include <cstddef>
#include <cstdint>
#include <new>
#include <algorithm>
#include <cmath>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iostream>

namespace arena_mr
{
    void *Align(void *ptr, size_t alignment)
    {
        uintptr_t uintptr = reinterpret_cast<uintptr_t>(ptr);
        uintptr_t aligned = (uintptr + alignment - 1) & ~(alignment - 1);
        return reinterpret_cast<void *>(aligned);
    }

    bool IsPowerOf2(size_t num)
    {
        // return std::popcount(num) == 1;
        return (num > 0) && ((num & (num - 1)) == 0);
    }

    struct ArenaInfo
    {
        ArenaInfo() = default;

        ArenaInfo(std::size_t num_of_allocation, std::size_t capacity, char *cursor)
            : num_of_allocation{num_of_allocation},
              bytes_left{capacity},
              cursor{cursor},
              capacity_{capacity}
        {
        }

        std::size_t num_of_allocation = 0;
        std::size_t bytes_left = 0;
        char *cursor = nullptr;

        std::size_t Capacity() const { return capacity_; }

    private:
        std::size_t capacity_ = 0;
    };

    // Exception for memory corruption detection.
    struct ArenaMrCorruption : std::runtime_error
    {
        ArenaMrCorruption(void *address, std::size_t bytes, std::size_t alignment)
            : std::runtime_error("Double-free or memory corruption in memory resource."),
              address(address),
              bytes(bytes),
              alignment(alignment)
        {
        }
        void *address = nullptr;
        std::size_t bytes = 0;
        std::size_t alignment = 0;
    };

    class UnsynchronizedArenaMR : public std::pmr::memory_resource
    {
    public:
        explicit UnsynchronizedArenaMR(std::size_t num_of_arenas, std::size_t size_per_arena, std::pmr::memory_resource *upstream = std::pmr::new_delete_resource())
            : num_of_arenas_(num_of_arenas), size_per_arena_(size_per_arena), upstream_{upstream}, arena_info_map_(upstream_), free_arena_list_(upstream_)
        {
            assert(num_of_arenas > 0);
            assert(size_per_arena % alignof(std::max_align_t) == 0);
            InitializeArenas();
        }

        std::size_t NumOfArenas() const { return num_of_arenas_; }

        std::size_t SizePerArena() const { return size_per_arena_; }

    private:
        void AllocateArena(std::size_t bytes, std::size_t alignment = alignof(std::max_align_t))
        {
            auto *arena = upstream_->allocate(bytes, alignment);
            arena_info_map_.insert(std::make_pair(arena, ArenaInfo(0, bytes, (char *)arena)));
            free_arena_list_.push_back(arena);
        }

        void InitializeArenas()
        {
            arena_info_map_.emplace(MIN_POINTER, ArenaInfo{}); // Limit to the bottom of the map
            arena_info_map_.emplace(MAX_POINTER, ArenaInfo{}); // Limit to the top of the map

            for (size_t i = 0; i < NumOfArenas(); i++)
            {
                AllocateArena(SizePerArena());
            }

            // get the first active arena
            auto arena_it = std::prev(arena_info_map_.upper_bound(free_arena_list_.back()));
            active_arena_info_ = &arena_it->second;
            free_arena_list_.pop_back();
        }

        void *DoAllocateDetails(std::size_t bytes, std::size_t alignment)
        {
            assert(IsPowerOf2(alignment));

            auto aligned_cursor = Align(active_arena_info_->cursor, alignment);
            auto bytes_needed = ((char *)aligned_cursor - active_arena_info_->cursor) + bytes;

            if (bytes_needed > active_arena_info_->bytes_left)
            {
                if (bytes > SizePerArena())
                {
                    // Needed bytes is greater than size per arena
                    // Always cause new allocation
                    AllocateArena(bytes); // we can return the inserted so that we will not need to search for the iterator

                    // We do not want this to change active arena because this arena will be consumed immidiately
                    auto arena_it = std::prev(arena_info_map_.upper_bound(free_arena_list_.back()));
                    auto cur_big_arena = &arena_it->second;
                    free_arena_list_.pop_back();

                    aligned_cursor = Align(cur_big_arena->cursor, alignment);
                    bytes_needed = ((char *)aligned_cursor - cur_big_arena->cursor) + bytes;
                    cur_big_arena->bytes_left -= bytes_needed;
                    cur_big_arena->cursor += bytes_needed;
                    cur_big_arena->num_of_allocation += 1;
                    return aligned_cursor;
                }
                else if (free_arena_list_.empty())
                {
                    AllocateArena(SizePerArena());
                }

                // If the case below check was not here we will loose the arena pointed by current `active_arena_info_`
                if (active_arena_info_->num_of_allocation == 0)
                {
                    free_arena_list_.push_back(active_arena_info_->cursor);
                }

                auto arena_it = std::prev(arena_info_map_.upper_bound(free_arena_list_.back()));
                active_arena_info_ = &arena_it->second;
                free_arena_list_.pop_back();

                // We know that there is enough space in the current arena

                aligned_cursor = Align(active_arena_info_->cursor, alignment);
                bytes_needed = ((char *)aligned_cursor - active_arena_info_->cursor) + bytes;
                active_arena_info_->bytes_left -= bytes_needed;
                active_arena_info_->cursor += bytes_needed;
                active_arena_info_->num_of_allocation += 1;

                return aligned_cursor;
            }

            // Enough space in current arena

            active_arena_info_->bytes_left -= bytes_needed;
            active_arena_info_->cursor += bytes_needed;
            active_arena_info_->num_of_allocation += 1;

            return aligned_cursor;
        }

    protected:
        void *do_allocate(std::size_t bytes, std::size_t alignment) override
        {
            std::cout << "Allocated " << bytes << std::endl;
            if (bytes == 0)
                return nullptr;

            if (bytes > SizePerArena())
            {
                std::cout << "Allocating more than arena size: " << bytes << std::endl;
            }

            return DoAllocateDetails(bytes, alignment);
        }

        void do_deallocate(void *p, std::size_t bytes = 0, std::size_t alignment = alignof(std::max_align_t)) override
        {
            std::cout << "Deallocated " << bytes << std::endl;
            if (p == nullptr)
                return;

            auto arena_it = std::prev(arena_info_map_.upper_bound(p));

            // If pointer is allocated from this allocator it should be found in the arena_info_map_
            if (arena_it->first == MIN_POINTER || arena_it->first == MAX_POINTER)
            {
                throw ArenaMrCorruption(p, bytes, alignment);
            }

            auto &arena = arena_it->second;

            assert(arena.num_of_allocation > 0);
            arena.num_of_allocation -= 1;
            // Does not free the allocated arena until num_of_allocation is 0

            if (arena.num_of_allocation == 0)
            {
                arena.num_of_allocation = 0;
                arena.bytes_left = arena.Capacity();
                arena.cursor = (char *)arena_it->first;

                if (&arena != active_arena_info_)
                {
                    free_arena_list_.push_back(arena_it->first);
                }
            }
        }

        bool do_is_equal(const std::pmr::memory_resource &other) const noexcept override
        {
            return (this == &other);
        }

    private:
        static constexpr void *MIN_POINTER = nullptr;
        static constexpr void *MAX_POINTER = (void *)INT64_MAX;

        std::size_t num_of_arenas_;  // Number of arenas.
        std::size_t size_per_arena_; // Size of each arena in bytes.

        std::pmr::memory_resource *upstream_;

        std::pmr::map<void *, ArenaInfo> arena_info_map_;
        std::pmr::vector<void *> free_arena_list_;

        ArenaInfo *active_arena_info_;

    }; // UnsynchronizedArenaMR

} // namespace arena_mr

#endif // ARENA_MR