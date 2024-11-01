#pragma once

#include "defines.h"

#define K_DARRAY_DEFAULT_CAPACITY 10

void* darray_create(u64 element_capacity, u64 element_stride);
void darray_destroy(void* darray);

void* darray_resize(void* darray, u64 new_capacity);
void darray_push(void* darray, void* element);
void darray_pop(void* darray, void* out_element);
