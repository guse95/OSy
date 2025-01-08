#include "FreeList.h"
#include <stdlib.h>
#include <unistd.h>

typedef struct Block {
    size_t size;
    struct Block* next;
} Block;

typedef struct Allocator {
    Block* free_list;
    void* memory_start;
    size_t total_size;
} Allocator;

Allocator* allocator_create(void *const memory, const size_t size) {
    if (!memory || size < sizeof(Block)) return NULL;

    Allocator* allocator = (Allocator*)memory;
    allocator->memory_start = memory;
    allocator->total_size = size;

    allocator->free_list = (Block*)((char*)memory + sizeof(Allocator));
    allocator->free_list->size = size - sizeof(Allocator);
    allocator->free_list->next = NULL;
    
    return allocator;
}

void* allocator_alloc(Allocator *const allocator, const size_t size) {
    if (!allocator || size == 0) return NULL;

    Block** current = &allocator->free_list;
    while (*current) {
        if ((*current)->size >= size) {
            if ((*current)->size > size + sizeof(Block)) {
                Block* remaining = (struct Block*)((char*)(*current) + sizeof(struct Block) + size);
                remaining->size = (*current)->size - size - sizeof(Block);
                remaining->next = (*current)->next;

                (*current)->size = size;
                *current = remaining;
            } else {
                *current = (*current)->next;
            }
            return (void*)((char*)(*current) + sizeof(struct Block));
        }
        current = &(*current)->next;
    }
    return NULL;
}

void allocator_free(Allocator *const allocator, void *const memory) {
    if (!allocator || !memory) return;

    Block* block = (struct Block*)((char*)memory - sizeof(struct Block));
    block->next = allocator->free_list;
    allocator->free_list = block;
}

void allocator_destroy(Allocator* allocator) {
    if (!allocator) {
        const char* msg = "FreeList Allocator destroy: NULL pointer\n";
        write(1, msg, 39);
        return;
    }
    const char* msg = "FreeList Allocator destroyed successfully\n";
    write(1, msg, 42);
}