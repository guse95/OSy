#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include "McKarels.h"

#define MAX_CLASSES 16
#define PAGE_SIZE 4096

typedef struct Block {
    size_t size;
    struct Block* next;
} Block;

typedef struct Allocator {
    Block* free_list[MAX_CLASSES];
    size_t class_sizes[MAX_CLASSES];
    void* memory_start;
    size_t memory_size;
} Allocator;

static size_t find_class(size_t size, size_t* class_sizes, size_t num_classes) {
    for (size_t i = 0; i < num_classes; i++) {
        if (size <= class_sizes[i]) {
            return i;
        }
    }
    return num_classes;
}

Allocator* allocator_create(void* const memory, const size_t size) {
    Allocator* allocator = (Allocator*)memory;
    if (size < sizeof(Allocator)) return NULL;

    allocator->memory_start = (char*)memory + sizeof(Allocator);
    allocator->memory_size = size - sizeof(Allocator);

    size_t block_size = 32;
    for (size_t i = 0; i < MAX_CLASSES; i++) {
        allocator->class_sizes[i] = block_size;
        allocator->free_list[i] = NULL;
        block_size *= 2;
    }

    return allocator;
}

void* allocator_alloc(Allocator* const allocator, const size_t size) {
    size_t class_index = find_class((size + sizeof(Block)), allocator->class_sizes, MAX_CLASSES);
    if (class_index >= MAX_CLASSES) return NULL;

    Block* block = allocator->free_list[class_index];
    if (block) {
        allocator->free_list[class_index] = block->next;
        return (void*)block;
    }

    size_t block_size = allocator->class_sizes[class_index];
    if (allocator->memory_size < block_size) return NULL;

    void* memory = allocator->memory_start;
    Block* bl = (Block*)memory;
    bl->size = block_size + sizeof(Block);
    allocator->memory_start = (char*)allocator->memory_start + block_size;
    allocator->memory_size -= block_size;
    return (void*)((char*)memory + sizeof(Block));
}

void allocator_free(Allocator* const allocator, void* const memory) {
    if (!memory) return;

    uintptr_t include = (uintptr_t)memory - (uintptr_t)allocator;
    if (include >= allocator->memory_size) return;

    size_t class_ind = 0;
    size_t block_size = 0;
    Block* block = (Block*)((char*)memory - sizeof(Block));
    for (; class_ind < MAX_CLASSES; class_ind++) {
        block_size = allocator->class_sizes[class_ind];
        if (block->size <= block_size) {
            break;
        }
    }

    if (class_ind >= MAX_CLASSES) return;

    block->next = allocator->free_list[class_ind];
    allocator->free_list[class_ind] = block;
}

void allocator_destroy(Allocator* const allocator) {
    for (size_t i = 0; i < MAX_CLASSES; i++) {
        allocator->free_list[i] = NULL;
    }
}