#include "arena.h"
#include "stdio.h"
#include "utils.h"
#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

Region *region_new(size_t capacity) {
    size_t  bytes  = sizeof(Region) + sizeof(uintptr_t) * capacity;
    Region *region = malloc(bytes);
    assert(region != NULL);
    region->next     = NULL;
    region->count    = 0;
    region->capacity = capacity;
    return region;
}

void region_free(Region *self) {
    free(self);
}

void *arena_alloc(Arena *self, size_t bytes) {
    size_t size = (bytes + sizeof(uintptr_t) - 1) / sizeof(uintptr_t);

    if (self->end == NULL) {
        assert(self->begin == NULL);
        size_t capacity = REGION_DEFAULT_CAP;
        if (capacity < size) capacity = size;
        self->end   = region_new(capacity);
        self->begin = self->end;
    }

    while (self->end->count + size > self->end->capacity && self->end->next != NULL) {
        self->end = self->end->next;
    }

    if (self->end->count + size > self->end->capacity) {
        assert(self->end->next == NULL);
        size_t capacity = REGION_DEFAULT_CAP;
        if (capacity < size) capacity = size;
        self->end->next = region_new(capacity);
        self->end       = self->end->next;
    }

    void *result = &self->end->data[self->end->count];
    self->end->count += size;
    return result;
}

void *arena_realloc(Arena *self, void *old_ptr, size_t old_size, size_t new_size) {
    if (new_size <= old_size) return old_ptr;
    void    *new_ptr      = arena_alloc(self, new_size);
    uint8_t *new_ptr_byte = (uint8_t *)new_ptr;
    uint8_t *old_ptr_byte = (uint8_t *)old_ptr;
    for (size_t i = 0; i < old_size; ++i) {
        new_ptr_byte[i] = old_ptr_byte[i];
    }
    return new_ptr;
}

void arena_free(Arena *self) {
    Region *region = self->begin;
    while (region) {
        Region *region_temp = region;
        region              = region->next;
        region_free(region_temp);
    }
    self->begin = NULL;
    self->end   = NULL;
}

char *string_alloc(Arena *arena, const char *fmt, ...) {
    va_list args;

    va_start(args, fmt);
    int size = vsnprintf(NULL, 0, fmt, args);
    assert(size >= 0 && "failed to count string size");
    va_end(args);

    char *ptr = arena_alloc(arena, size + 1);

    va_start(args, fmt);
    int result_size = vsnprintf(ptr, size + 1, fmt, args);
    assert(result_size == size);
    va_end(args);

    return ptr;
}
