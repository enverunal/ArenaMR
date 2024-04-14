#ifndef ARENA_MR
#define ARENA_MR

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <map>
#include <memory_resource>
#include <numeric>
#include <vector>

namespace arena_mr
{
    namespace detail
    {
        void *Align(void const *ptr, size_t alignment) noexcept
        {
            uintptr_t uintptr = reinterpret_cast<uintptr_t>(ptr);
            uintptr_t aligned = (uintptr + alignment - 1) & ~(alignment - 1);
            return reinterpret_cast<void *>(aligned);
        }

        bool IsPowerOf2(size_t num) noexcept
        {
            // return std::popcount(num) == 1;
            return (num > 0) && ((num & (num - 1)) == 0);
        }
    }

    struct ArenaInfo
    {
        ArenaInfo() = default;

        ArenaInfo(std::size_t num_of_allocation, std::size_t capacity, std::byte *cursor) noexcept
            : num_of_allocation{num_of_allocation},
              bytes_left{capacity},
              cursor{cursor},
              capacity_{capacity}
        {
        }

        std::size_t num_of_allocation = 0;
        std::size_t bytes_left = 0;
        std::byte *cursor = nullptr;

        std::size_t Capacity() const noexcept
        {
            return capacity_;
        }

        void Reduce(std::size_t bytes) noexcept
        {
            bytes_left -= bytes;
            cursor += bytes;
            num_of_allocation += 1;
        }

        void *AlignedCursor(std::size_t alignment) noexcept
        {
            return detail::Align(cursor, alignment);
        }

    private:
        std::size_t capacity_ = 0;
    };

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
            : num_of_arenas_(num_of_arenas),
              size_per_arena_(size_per_arena),
              upstream_{upstream},
              arena_info_map_(upstream_),
              free_arena_list_(upstream_)
        {
            assert(num_of_arenas > 0);
            assert(size_per_arena % alignof(std::max_align_t) == 0);
            InitializeArenas();
        }

        std::size_t NumOfArenas() const noexcept
        {
            return num_of_arenas_;
        }

        std::size_t SizePerArena() const noexcept
        {
            return size_per_arena_;
        }

        // Test Function
        // Number of free arenas
        std::size_t FreeArenaSize() const noexcept
        {
            return free_arena_list_.size();
        }

        // Test Function
        // Memory used by the user.
        // Can be greater than user allocated space because of the alignment.
        std::size_t UsedMemory() const noexcept
        {
            std::size_t total_allocated_bytes = 0;
            for (auto const &[_, info] : arena_info_map_)
            {
                total_allocated_bytes += info.Capacity() - info.bytes_left;
            }
            return total_allocated_bytes;
        }

        // Test Function
        // Return the memory that is end of the arena but the arena itself is not active
        // The end of the arena cannot be used until it is free again. Thus it is wasted space.
        std::size_t WastedMemory() const noexcept
        {
            std::size_t wasted_bytes = 0;
            for (auto const &[_, info] : arena_info_map_)
            {
                auto is_arena_free = std::find(free_arena_list_.begin(), free_arena_list_.end(), &info) != free_arena_list_.end();
                if (!is_arena_free && (&info != active_arena_info_))
                {
                    wasted_bytes += info.bytes_left;
                }
            }
            return wasted_bytes;
        }

    private:
        void AllocateArena(std::size_t bytes, std::size_t alignment = alignof(std::max_align_t))
        {
            auto *arena = upstream_->allocate(bytes, alignment);
            [[maybe_unused]] auto [it, suc] = arena_info_map_.insert(std::make_pair(arena, ArenaInfo(0, bytes, (std::byte *)arena)));
            assert(suc);
            free_arena_list_.push_back(&it->second);
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
            active_arena_info_ = free_arena_list_.back();
            free_arena_list_.pop_back();
        }

        void *DoAllocateDetails(std::size_t bytes, std::size_t alignment)
        {
            assert(detail::IsPowerOf2(alignment));

            auto aligned_cursor = active_arena_info_->AlignedCursor(alignment);
            auto bytes_needed = ((std::byte *)aligned_cursor - active_arena_info_->cursor) + bytes;

            if (bytes_needed > active_arena_info_->bytes_left)
            {
                if (bytes > SizePerArena())
                {
                    // Needed bytes is greater than size per arena.
                    // Always cause new allocation.
                    AllocateArena(bytes); // We can return the inserted so that we will not need to search for the iterator.

                    // We do not want this to change active arena because this arena will be consumed immidiately.
                    auto cur_big_arena = free_arena_list_.back();
                    free_arena_list_.pop_back();

                    // Don't need to do these calculations. Bytes needed is equal to capacity.
                    aligned_cursor = cur_big_arena->AlignedCursor(alignment);
                    bytes_needed = ((std::byte *)aligned_cursor - cur_big_arena->cursor) + bytes;
                    cur_big_arena->Reduce(bytes_needed);
                    return aligned_cursor;
                }
                else if (free_arena_list_.empty())
                {
                    AllocateArena(SizePerArena());
                }

                // If the assertion below happens we will loose the arena pointed by current `active_arena_info_`.
                // However this case should not happen because we of the check `bytes > SizePerArena()`.
                assert(active_arena_info_->num_of_allocation != 0);

                active_arena_info_ = free_arena_list_.back();
                free_arena_list_.pop_back();

                // We know that there is enough space in the current arena.
                // Recalculate `aligned_cursor` and `bytes_needed`

                aligned_cursor = active_arena_info_->AlignedCursor(alignment);
                bytes_needed = ((std::byte *)aligned_cursor - active_arena_info_->cursor) + bytes;

                // active_arena_info_->Reduce(bytes_needed);
                // return aligned_cursor;
            }

            // Enough space in current arena.

            active_arena_info_->Reduce(bytes_needed);
            return aligned_cursor;
        }

    protected:
        void *do_allocate(std::size_t bytes, std::size_t alignment) override
        {
            if (bytes == 0)
                return nullptr;

            return DoAllocateDetails(bytes, alignment);
        }

        void do_deallocate(void *p, std::size_t bytes = 0, std::size_t alignment = alignof(std::max_align_t)) override
        {
            if (p == nullptr)
                return;

            auto arena_it = std::prev(arena_info_map_.upper_bound(p));

            // If pointer is allocated from this allocator it should be found in the `arena_info_map_`.
            if (arena_it->first == MIN_POINTER || arena_it->first == MAX_POINTER)
            {
                throw ArenaMrCorruption(p, bytes, alignment);
            }

            auto &arena = arena_it->second;

            assert(arena.num_of_allocation > 0);
            arena.num_of_allocation -= 1;

            // Does not free the allocated arena until num_of_allocation is 0.

            if (arena.num_of_allocation == 0)
            {
                arena.num_of_allocation = 0;
                arena.bytes_left = arena.Capacity();
                arena.cursor = (std::byte *)arena_it->first;

                if (&arena != active_arena_info_)
                {
                    free_arena_list_.push_back(&arena);
                }
            }
        }

        bool do_is_equal(std::pmr::memory_resource const &other) const noexcept override
        {
            return (this == &other);
        }

    private:
        static constexpr void *MIN_POINTER = nullptr;
        static constexpr void *MAX_POINTER = (void *)INT64_MAX;

        std::size_t num_of_arenas_;  // Number of arenas.
        std::size_t size_per_arena_; // Size of each arena in bytes.

        std::pmr::memory_resource *upstream_;

        // If nodes are rarely inserted, it might be better to use vector + binary search.
        std::pmr::map<void *, ArenaInfo> arena_info_map_;

        std::pmr::vector<ArenaInfo *> free_arena_list_;

        ArenaInfo *active_arena_info_;

    }; // UnsynchronizedArenaMR

} // namespace arena_mr

#endif // ARENA_MR