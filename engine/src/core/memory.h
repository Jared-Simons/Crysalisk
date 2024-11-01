#pragma once

#include "defines.h"

typedef enum memory_allocation_tag {
    MEMORY_TAG_DARRAY,
    MEMORY_TAG_CORE,
    MEMORY_TAG_APPLICATION,
    MEMORY_TAG_ENGINE,

    MEMORY_TAG_MAX_TAGS
} memory_allocation_tag;

b8 memory_initialize();
void memory_shutdown();

void* memory_allocate(u64 size, memory_allocation_tag tag);
void memory_free(void* memory, u64 size, memory_allocation_tag tag);
void* memory_copy(void* dest, const void* source, u64 size);
void memory_set(void* block, i32 value, u64 size);
void memory_zero(void* block, u64 size);
