#include "darray.h"

#include "core/logging.h"
#include "core/memory.h"

#define K_DARRAY_DEFAULT_RESIZE_FACTOR 2

typedef struct darray_header {
    // The number of elements this darray can hold (without reallocation)
    u64 element_capacity;
    // The size in bytes of each element
    u64 element_stride;
    // The number of elements currently held in this darray
    u64 element_count;
} darray_header;

void* darray_create(u64 element_capacity, u64 element_stride) {
    if (element_capacity < 1 || element_stride < 1) {
        LOG_ERROR("darray_create requires that element_capacity and element_stride be > 0. Creation failed.");
        return 0;
    }

    // Allocate enough memory for the array and the header.
    void* memory = memory_allocate(sizeof(darray_header) + (element_capacity * element_stride), MEMORY_TAG_DARRAY);
    darray_header* header = (darray_header*)memory;
    header->element_capacity = element_capacity;
    header->element_stride = element_stride;
    header->element_count = 0;

    u8* darray_data = memory + sizeof(darray_header);
    memory_set(darray_data, 0, element_capacity * element_stride);
    return darray_data;
}

void darray_destroy(void* darray) {
    if (darray) {
        u8* darray_addr = (u8*)darray;
        darray_header* header = (darray_header*)(darray_addr - sizeof(darray_header));
        u64 capacity = header->element_capacity;
        u64 stride = header->element_stride;

        memory_free(header, (capacity * stride) + sizeof(darray_header), MEMORY_TAG_DARRAY);
    }
}

void* darray_resize(void* darray, u64 new_capacity) {
    if (darray) {
        u8* darray_addr = darray;
        darray_header* header = (darray_header*)(darray_addr - sizeof(darray_header));

        if (new_capacity <= header->element_capacity) {
            // NOTE: We can only grow our darray.
            LOG_WARNING("darray_resize is trying to shrink array, nothing was done.");
            return darray;
        }

        // Allocate new memory and copy the data over.
        void* new_darray = memory_allocate((new_capacity * header->element_stride) + sizeof(darray_header), MEMORY_TAG_DARRAY);
        darray_header* new_header = (darray_header*)new_darray;
        memory_copy(new_header, header, sizeof(darray_header));
        u8* new_darray_data = (u8*)new_header + sizeof(darray_header);
        memory_copy(new_darray_data, darray, header->element_capacity * header->element_stride);

        // Free the original darray
        memory_free(header, sizeof(darray_header) + (header->element_capacity * header->element_stride), MEMORY_TAG_DARRAY);

        // Update the new header
        new_header->element_capacity = new_capacity;
        return new_darray_data;
    }

    LOG_ERROR("darray_resize was not provided a darray, returning nullptr.");
    return 0;
}

void darray_push(void* darray, void* element) {
    if (darray && element) {
        u8* darray_addr = (u8*)darray;
        darray_header* header = (darray_header*)(darray_addr - sizeof(darray_header));
        if (header->element_count + 1 > header->element_capacity) {
            // TODO: Handle resizing.
            darray_resize(darray, header->element_capacity * K_DARRAY_DEFAULT_RESIZE_FACTOR);
        }

        u64 element_data = ((u64)darray) + header->element_count * header->element_stride;
        memory_copy((void*)element_data, element, header->element_stride);
        header->element_count++;
    }
}

void darray_pop(void* darray, void* out_element) {
    if (darray && out_element) {
        u8* darray_addr = (u8*)darray;
        darray_header* header = (darray_header*)(darray - sizeof(darray_header));
        if (header->element_count < 1) {
            // TODO: logging
        }

        u64 element_data = ((u64)darray) + (header->element_count - 1) * header->element_stride;
        memory_copy((void*)out_element, (void*)element_data, header->element_stride);
        header->element_count--;
    }
}

u64 darray_length_get(void* darray) {
    if (darray) {
        u8* darray_addr = (u8*)darray;
        darray_header* header = (darray_header*)(darray - sizeof(darray_header));

        return header->element_capacity;
    }

    return 0;
}
