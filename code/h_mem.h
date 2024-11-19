#pragma once
#include "h_clogger.h"

constexpr size_t B  = 1;
constexpr size_t KB = 1024;
constexpr size_t MB = 1024 * 1024;
constexpr size_t GB = 1024 * 1024 * 1024;

enum allocator_type {
	LINEAR_ALLOCATOR,
	STACK_ALLOCATOR
};

struct linear_allocator {
	uint8_t* head;
	uint8_t* ptr;
	size_t total_size;
	size_t used_size;
};

struct mem_pool {
	uint8_t* root;
	uint8_t* ptr;
	size_t total_size;
	size_t used_size;

};


void mem_pool_create(mem_pool* pool, size_t size);


void mem_pool_destroy(mem_pool* pool);

void mem_pool_alloc_linear_or(mem_pool* pool, allocator_type  type, size_t size, linear_allocator* allocator);


void* mem_allocator_alloc();



