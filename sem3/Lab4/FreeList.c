#include "FreeList.h"
#include <stdlib.h>
#include <unistd.h>

typedef struct Block {
    size_t size;
    struct Block* next;
    struct Block* prev;
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
    allocator->free_list->prev = NULL;

    return allocator;
}

void* allocator_alloc(Allocator *const allocator, const size_t size) {
    if (!allocator || size == 0) return NULL;
    
    Block* best_prior = NULL;
    Block* current = allocator->free_list;
    while (current != NULL) {
        if (current->size >= size + sizeof(Block)) {
            if (best_prior == NULL || current->size < best_prior->size) {
                best_prior = current;
            }
        }
        current = current->next;
    }
    
    if (best_prior != NULL) {
        Block* remaining = (Block*)((char*)best_prior + sizeof(Block) + size);
        remaining->size = best_prior->size - size - sizeof(Block);
        remaining->next = best_prior->next;
        remaining->prev = best_prior;
        if (remaining->next != NULL) {
            remaining->next->prev = remaining;
        }

        if (best_prior->prev != NULL) {
            best_prior->prev->next = remaining;
        } else {
            allocator->free_list = remaining;
        }
        
        return (void*)((char*)best_prior + sizeof(Block));
    }

    return NULL;
}



void allocator_free(Allocator *const allocator, void *const memory) {
    if (!allocator || !memory) return;

    Block* block = (Block*)((char*)memory - sizeof(Block));
    block->next = allocator->free_list;
    block->prev = NULL;
    allocator->free_list = block;
}

void allocator_destroy(Allocator* allocator) {
    allocator->free_list = NULL;
}