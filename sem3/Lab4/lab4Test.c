#include <stddef.h>
#include <dlfcn.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>

void ptr_to_str(void* ptr, char* buf) {
    unsigned long addr = (unsigned long)ptr;
    char hex_digits[] = "0123456789abcdef";
    buf[0] = '0';
    buf[1] = 'x';

    for (int i = 15; i >= 2; i--) {
        buf[i] = hex_digits[addr & 0xF];
        addr >>= 4;
    }

    buf[16] = '\0';
}

typedef struct Allocator {
    struct Block* free_list;
    void* memory_start;
    size_t total_size;
} Allocator;

typedef Allocator* (*allocator_create_func)(void *const memory, const size_t size);
typedef void (*allocator_destroy_func)(Allocator *const allocator);
typedef void* (*allocator_alloc_func)(Allocator *const allocator, const size_t size);
typedef void (*allocator_free_func)(Allocator *const allocator, void *const memory);

allocator_create_func allocator_create = NULL;
allocator_destroy_func allocator_destroy = NULL;
allocator_alloc_func allocator_alloc = NULL;
allocator_free_func allocator_free = NULL;

long get_current_time_ns() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (ts.tv_sec * 1000000000) + ts.tv_nsec;
}

Allocator* fallback_allocator_create(void *const memory, const size_t size) {
    (void)memory;
    (void)size;
    Allocator* dummy = mmap(NULL, sizeof(Allocator), PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
    return dummy;
}

void fallback_allocator_destroy(Allocator *const allocator) {
    if (allocator) munmap(allocator, sizeof(Allocator));
}

void* fallback_allocator_alloc(Allocator *const allocator, const size_t size) {
    (void)allocator;
    return mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
}

void fallback_allocator_free(Allocator *const allocator, void *const memory) {
    (void)allocator;
    if (memory) munmap(memory, 0);
}

void load_allocator_library(const char* library_path) {
    void* handle = dlopen(library_path, RTLD_LAZY);
    if (!handle) {
        const char* errorMsg = "Error loading library. Falling back to system allocator.\n";
        write(1, errorMsg, 55);
        allocator_create = fallback_allocator_create;
        allocator_destroy = fallback_allocator_destroy;
        allocator_alloc = fallback_allocator_alloc;
        allocator_free = fallback_allocator_free;
        return;
    }

    allocator_create = (allocator_create_func)dlsym(handle, "allocator_create");
    allocator_destroy = (allocator_destroy_func)dlsym(handle, "allocator_destroy");
    allocator_alloc = (allocator_alloc_func)dlsym(handle, "allocator_alloc");
    allocator_free = (allocator_free_func)dlsym(handle, "allocator_free");

    if (!allocator_create || !allocator_destroy || !allocator_alloc || !allocator_free) {
        const char* symbolError = "Error loading symbols. Falling back to system allocator.\n";
        write(1, symbolError, 58);
        dlclose(handle);
        allocator_create = fallback_allocator_create;
        allocator_destroy = fallback_allocator_destroy;
        allocator_alloc = fallback_allocator_alloc;
        allocator_free = fallback_allocator_free;
        return;
    }

    const char* successMsg = "Library loaded successfully.\n";
    write(1, successMsg, 29);
}

int main(int argc, char* argv[]) {
    const size_t memory_size = 1 * 1024 * 1024 * 1024;

    void* memory = mmap(NULL, memory_size, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
    if (memory == MAP_FAILED) {
        const char* mmapError = "Failed to allocate memory using mmap.\n";
        write(1, mmapError, 39);
        return 1;
    }

    if (argc < 2) {
        const char* noLibraryMsg = "No library path provided. Using system allocator.\n";
        write(1, noLibraryMsg, 52);
        allocator_create = fallback_allocator_create;
        allocator_destroy = fallback_allocator_destroy;
        allocator_alloc = fallback_allocator_alloc;
        allocator_free = fallback_allocator_free;
    } else {
        load_allocator_library(argv[1]);
    }

    Allocator* allocator = allocator_create(memory, memory_size);
    if (!allocator) {
        const char* createError = "Failed to create allocator.\n";
        write(1, createError, 28);
        return 1;
    }
    long start = get_current_time_ns();
    void* ptr1[1000];
    for (int i = 0; i < 1000; ++i) {
        ptr1[i] = allocator_alloc(allocator, 64 * 1024);
        if (!ptr1[i]) {
            const char* allocError1 = "Failed to allocate 64 bytes.\n";
            write(1, allocError1, 30);
        }
    }
    long stop = get_current_time_ns();
    printf("%ld\n", (stop - start));

    {
        const char* allocMsg1 = "64 bytes allocated successfully. Pointer address: ";
        write(1, allocMsg1, 50);
        char ptr1Str[18];
        ptr_to_str(ptr1, ptr1Str);
        write(1, ptr1Str, 18);
        const char* endMsg1 = "\n";
        write(1, endMsg1, 1);

        long start2 = get_current_time_ns();
        for (int i = 0; i < 1000; ++i) {
            allocator_free(allocator, ptr1[i]);
        }
        printf("%ld\n", (get_current_time_ns() - start2));
    }

   
    allocator_destroy(allocator);
}