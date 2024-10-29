#include "memory.h"

#include "platform/platform.h"

typedef struct memory_stats_t {
    u64 total_number_allocations;
    u64 total_memory_allocated;
    u64 tagged_allocations[MEMORY_TAG_MAX_TAGS];
} memory_stats_t;

static memory_stats_t* memory_stats = 0;

b8 memory_initialize() {
    if (!memory_stats) {
        memory_stats = platform_allocate(sizeof(memory_stats_t));

        for (int index = 0; index < MEMORY_TAG_MAX_TAGS; ++index) {
            memory_stats->tagged_allocations[index] = 0;
        }

        memory_stats->total_number_allocations = 0;
        memory_stats->total_memory_allocated = 0;
    }

    return true;
}

void memory_shutdown() {
    if (memory_stats) {
        platform_free(memory_stats);
    }
}

void* memory_allocate(u64 size, memory_allocation_tag tag) {
    // TODO: Custom allocators
    void* block = 0;

    block = platform_allocate(size);

    memory_stats->total_number_allocations++;
    memory_stats->total_memory_allocated += size;
    memory_stats->tagged_allocations[tag] += size;

    return block;
}

void memory_free(void* block, u64 size, memory_allocation_tag tag) {
    platform_free(block);

    memory_stats->total_number_allocations--;
    memory_stats->total_memory_allocated -= size;
    memory_stats->tagged_allocations[tag] -= size;
}

void* memory_copy(void* dest, const void* source, u64 size) {
    return platform_memcpy(dest, source, size);
}

void memory_set(void* block, i32 value, u64 size) {
    platform_memset(block, value, size);
}

void memory_zero(void* block, u64 size) {
    memory_set(block, 0, size);
}
