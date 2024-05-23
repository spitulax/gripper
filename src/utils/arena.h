#ifndef ARENA_H
#define ARENA_H

#include <stddef.h>
#include <stdint.h>

typedef struct Region Region;

struct Region {
    Region   *next;
    size_t    count;
    size_t    capacity;
    uintptr_t data[];
};

typedef struct {
    Region *begin, *end;
} Arena;

#define REGION_DEFAULT_CAP (8 * 1024)
Region *region_new(size_t capacity);
void    region_free(Region *self);

void *arena_alloc(Arena *self, size_t bytes);
void *arena_realloc(Arena *self, void *old_ptr, size_t old_size, size_t new_size);
void  arena_free(Arena *self);

char *string_alloc(Arena *arena, const char *fmt, ...);

#endif /* ifndef ARENA_H */
