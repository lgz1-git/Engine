#include "h_mem.h"

void mem_pool_create(mem_pool* pool, size_t size)
{
	pool->total_size = size;
	pool->root = (uint8_t*)malloc(pool->total_size);
	pool->used_size = 0;
	pool->ptr = pool->root;
}

void mem_pool_destroy(mem_pool* pool)
{
	free(pool->root);
	pool->ptr = nullptr;
	pool->total_size = 0;
	pool->used_size = 0;
}

void mem_pool_alloc_linear_or(mem_pool* pool, allocator_type  type, size_t size, linear_allocator* allocator)
{
	if (allocator->head != nullptr) {
		LERR("re malloc memory to pointer which is not empty!")
			return;
	}
	size_t temp = pool->total_size - pool->used_size;
	if (temp > size) {
		allocator->head = pool->ptr;
		allocator->total_size = size;
		allocator->used_size = 0;
		allocator->ptr = pool->ptr;

		pool->ptr += size;
		pool->used_size += size;

	}
	else {
		LERR("sorry! pool can not alloc allocator!");
		return;
	}
}

void* mem_allocator_alloc()
{
	return nullptr;
}

