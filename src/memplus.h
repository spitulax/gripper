/* Copyright 2024 Bintang Adiputra Pratama <bintangadiputrapratama@gmail.com>

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
associated documentation files (the “Software”), to deal in the Software without restriction,
including without limitation the rights to use, copy, modify, merge, publish, distribute,
sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial
portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES
OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#ifndef MEMPLUS_H__
#define MEMPLUS_H__

/* #define MEMPLUS_IMPLEMENTATION */

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef _MEMPLUS_ASSERT
#include <assert.h>
#define _MEMPLUS_ASSERT assert
#endif

/***********
 * ALLOCATOR
 ***********/

/* Default size of a single region in qwords. You can adjust this to your liking. */
#ifndef MP_REGION_DEFAULT_SIZE
#define MP_REGION_DEFAULT_SIZE (8 * 1024)
#endif

typedef struct mp_Region mp_Region;

/* Holds certain size of allocated memory. */
struct mp_Region {
    mp_Region *next;        // The next region in the linked list if any
    size_t     count;       // The amount of data (in qwords) used
    size_t     capacity;    // The amount of data (in qwords) allocated
    uintptr_t  data[];      // The data (aligned)
};

/* Manages regions in a linked list. */
typedef struct {
    mp_Region *begin, *end;
} mp_Arena;

/* Interface to wrap functions to allocate memory.
 * The method of allocation can be costumized by the user. */
typedef struct {
    // The object that manages or holds the memory.
    // In case of allocator that works with global memory, this could be specified as NULL.
    void *context;
    /* These functions is not meant to be used directly.
     * To call them use the macros defined below. */
    // Allocates the memory.
    void *(*alloc)(void *context, size_t size);
    // Takes a pointer to a data and reallocate it with a new size.
    void *(*realloc)(void *context, void *old_ptr, size_t old_size, size_t new_size);
    // Takes a pointer to a data and allocate its duplicate.
    void *(*dup)(void *context, void *data, size_t size);
} mp_Allocator;

/* Macros that wrap the functions above */
// self: mp_Allocator*
// size: number of bytes
#define mp_allocator_alloc(self, size) ((self)->alloc((self)->context, (size)))
// self: mp_Allocator*
// type: typename
#define mp_allocator_create(self, type) ((self)->alloc((self)->context, (sizeof(type))))
// self: mp_Allocator*
// old_ptr: pointer
// old_size: number of bytes
// new_size: number of bytes
#define mp_allocator_realloc(self, old_ptr, old_size, new_size)                                    \
    ((self)->realloc((self)->context, (old_ptr), (old_size), (new_size)))
// self: mp_Allocator*
// data: pointer
// size: number of bytes
#define mp_allocator_dup(self, data, size) ((self)->dup((self)->context, (data), (size)))

/* Creates a custom allocator given the context and respective function pointers. */
// context: pointer
// alloc_func, realloc_func, dup_func: function pointer
#define mp_allocator_new(context, alloc_func, realloc_func, dup_func)                              \
    ((mp_Allocator){                                                                               \
        (void *)(context),                                                                         \
        (void *(*)(void *, size_t))(alloc_func),                                                   \
        (void *(*)(void *, void *, size_t, size_t))(realloc_func),                                 \
        (void *(*)(void *, void *, size_t))(dup_func),                                             \
    })

/* Allocates a new region with `capacity` * 8  bytes of size. */
mp_Region *mp_region_new(size_t capacity);
/* Frees region from memory. */
void mp_region_free(mp_Region *self);

/* Creates a new, unallocated arena. */
#define mp_arena_new()                                                                             \
    (mp_Arena) {                                                                                   \
        0                                                                                          \
    }
/* Frees the arena and its regions. */
void mp_arena_free(mp_Arena *self);
/* Returns an allocator that works with `arena`. */
mp_Allocator mp_arena_new_allocator(mp_Arena *arena);

/***********
 * END OF ALLOCATOR
 ***********/

/***********
 * STRING
 ***********/

/* Holds a null-terminated string and the size of the string (excluding the null-terminator). */
typedef struct {
    size_t size;
    char  *cstr;
} mp_String;

/* Allocates a new `mp_String` from a null-terminated string. */
mp_String mp_string_new(mp_Allocator *allocator, const char *str);
/* Allocates a new `mp_String` from formatted input. */
mp_String mp_string_newf(mp_Allocator *allocator, const char *fmt, ...);
/* Allocates duplicate of `str`. */
mp_String mp_string_dup(mp_Allocator *allocator, mp_String str);

/***********
 * END OF STRING
 ***********/

/***********
 * VECTOR
 ***********/

/* Starting capacity of a vector. You can adjust this to your liking. */
#ifndef MP_VECTOR_INIT_CAPACITY
#define MP_VECTOR_INIT_CAPACITY 256
#endif

/* You can define a vector struct with any type as long as it's in this format. */
/*
    typedef struct {
        mp_Allocator *alloc;     // The allocator that manages the allocation of the vector
        size_t       size;      // The size of the vector
        size_t       capacity;  // The capacity of the vector
        <type>       *data;     // Pointer to the data (points to the first element)
        // The data is continuous in memory.
    } Vector;
*/

/* Defines a compatible vector struct given `name` and the data `type`. */
// name: identifier
// type: typename
#define mp_vector_create(name, type)                                                               \
    typedef struct {                                                                               \
        mp_Allocator *alloc;                                                                       \
        size_t        size;                                                                        \
        size_t        capacity;                                                                    \
        type         *data;                                                                        \
    } name

/* Initializes a new vector and tell it to use `allocator`. */
// allocator: mp_Allocator*
// name: typename
#define mp_vector_new(allocator, name)                                                             \
    (name) {                                                                                       \
        .alloc = (allocator), .size = 0, .capacity = 0, .data = NULL,                              \
    }

/* Gets an item at index `i`. */
// self: Vector*
// i: size_t
#define mp_vector_get(self, i) (self)->data[i]

/* Resizes vector to `offset` of the current size.
 * If the current capacity is 0, allocates for `MP_VECTOR_INIT_CAPACITY` items.
 * If the current capacity is not large enough, allocates for double the current capacity.
 * Positive `offset` grows the vector.
 * Negative `offset` shrinks the vector. */
// self: Vector*
// offset: int
#define mp_vector_resize(self, offset)                                                             \
    do {                                                                                           \
        if ((self)->size + (offset) > (self)->capacity && (offset) > 0) {                          \
            if ((self)->capacity == 0) {                                                           \
                (self)->capacity = MP_VECTOR_INIT_CAPACITY;                                        \
            }                                                                                      \
            while ((self)->size + (offset) > (self)->capacity) {                                   \
                (self)->capacity *= 2;                                                             \
            }                                                                                      \
            (self)->data = mp_allocator_realloc(                                                   \
                (self)->alloc, (self)->data, 0, (self)->capacity * sizeof(*(self)->data));         \
        }                                                                                          \
        (self)->size += (offset);                                                                  \
    } while (0)

/* Changes the capacity of the vector.
 * Shrinks the vector `capacity` is smaller than the current size.
 * Reallocate the vector if `capacity` is larger than the current capacity. */
// self: Vector*
// new_capacity: size_t
#define mp_vector_reserve(self, new_capacity)                                                      \
    do {                                                                                           \
        if ((new_capacity) < (self)->size) {                                                       \
            mp_vector_resize((self), (new_capacity) - (self)->size);                               \
        } else if ((new_capacity) > (self)->capacity) {                                            \
            (self)->data = mp_allocator_realloc(                                                   \
                (self)->alloc, (self)->data, 0, (new_capacity) * sizeof(*(self)->data));           \
        }                                                                                          \
        (self)->capacity = (new_capacity);                                                         \
    } while (0)

/* Resizes the vector and appends item to the end. */
// self: Vector*
// item: value of the same type as the vector data
#define mp_vector_append(self, item)                                                               \
    do {                                                                                           \
        mp_vector_resize(self, 1);                                                                 \
        (self)->data[(self)->size - 1] = (item);                                                   \
    } while (0)

/* Resizes the vector and appends items from `items_ptr` to the end. */
// self: Vector*
// items_ptr: pointer to the same type as the vector data
// items_amount: size_t
#define mp_vector_append_many(self, items_ptr, items_amount)                                       \
    do {                                                                                           \
        mp_vector_resize((self), (items_amount));                                                  \
        memcpy((self)->data + ((self)->size - (items_amount)),                                     \
               (items_ptr),                                                                        \
               (items_amount) * sizeof(*(self)->data));                                            \
    } while (0)

/* Gets the first or the last item in the vector. */
// self: Vector*
#define mp_vector_first(self) (self)->data[0]
#define mp_vector_last(self)  (self)->data[(self)->size - 1]

/* Deletes the last item in the vector and returns it. */
// self: Vector*
#define mp_vector_pop(self) (--(self)->size, (self)->data[(self)->size])

/* Sets the vector size to 0. */
// self: Vector*
#define mp_vector_clear(self)                                                                      \
    do {                                                                                           \
        (self)->size = 0;                                                                          \
    } while (0)

/* Inserts an item in the given `pos`. */
// self: Vector*
// pos: size_t
// item: value of the same type of the vector data
#define mp_vector_insert(self, pos, item)                                                          \
    do {                                                                                           \
        size_t actual_pos = (pos) > (self)->size ? (self)->size : (pos);                           \
        mp_vector_resize((self), 1);                                                               \
        for (int i = (self)->size - 2; i > actual_pos; --i)                                        \
            (self)->data[i + 1] = (self)->data[i];                                                 \
        (self)->data[actual_pos + 1] = (self)->data[actual_pos];                                   \
        (self)->data[actual_pos]     = (item);                                                     \
    } while (0)

/* Inserts items from `items_ptr` in the given `pos`.
 * Items previously in and after `pos` are shifted. */
// self: Vector*
// pos: size_t
// items_ptr: pointer to the same type as the vector data
// items_amount: size_t
#define mp_vector_insert_many(self, pos, items_ptr, amount)                                        \
    do {                                                                                           \
        size_t actual_pos = (pos) > (self)->size ? (self)->size : (pos);                           \
        mp_vector_resize((self), (amount));                                                        \
        for (int i = (self)->size - 1 - (amount); i > actual_pos; --i)                             \
            (self)->data[i + amount] = (self)->data[i];                                            \
        (self)->data[actual_pos + amount] = (self)->data[actual_pos];                              \
        memcpy((self)->data + actual_pos, (items_ptr), (amount) * sizeof(*(self)->data));          \
    } while (0)

/* Deletes an item in the given `pos`. */
// self: Vector*
// pos: size_t
#define mp_vector_erase(self, pos)                                                                 \
    do {                                                                                           \
        _MEMPLUS_ASSERT((pos) < (self)->size && "index out of bounds");                            \
        mp_vector_resize((self), -1);                                                              \
        for (size_t i = (pos) + 1; i < (self)->size + 1; ++i)                                      \
            (self)->data[i - 1] = (self)->data[i];                                                 \
    } while (0)

/* Deletes an item in the given `pos` and return that item. */
// self: Vector*
// pos: size_t
#define mp_vector_erase_ret(self, pos)                                                             \
    (self)->data[pos];                                                                             \
    mp_vector_erase((self), (pos))

/* Deletes items from `items_ptr` in the given `pos`.
 * Items are shifted to fill the spaces left by deleted items. */
// self: Vector*
// pos: size_t
// amount: size_t
#define mp_vector_erase_many(self, pos, amount)                                                    \
    do {                                                                                           \
        _MEMPLUS_ASSERT((pos) + (amount) <= (self)->size && "index out of bounds");                \
        mp_vector_resize((self), -(amount));                                                       \
        for (size_t i = (pos) + (amount); i < (self)->size + (amount); ++i)                        \
            (self)->data[i - (amount)] = (self)->data[i];                                          \
    } while (0)

/* Same as `mp_vector_erase_many`, but also writes the deleted items to `buf`. */
// self: Vector*
// pos: size_t
// buf: pointer to a buffer containing the same type as the vector data
// amount: size_t
#define mp_vector_erase_many_to_buf(self, pos, buf, amount)                                        \
    do {                                                                                           \
        _MEMPLUS_ASSERT((pos) + (amount) <= (self)->size && "index out of bounds");                \
        mp_vector_resize((self), -(amount));                                                       \
        for (size_t i = 0; i < (amount); ++i)                                                      \
            (buf)[i] = (self)->data[(pos) + i];                                                    \
        for (size_t i = (pos) + (amount); i < (self)->size + (amount); ++i)                        \
            (self)->data[i - (amount)] = (self)->data[i];                                          \
    } while (0)

/* Iterates over a vector. Usage:
 * ```
    mp_foreach(int * v, &vector);
    {
        // `v` is a pointer to the current element
        // `i` is a size_t representing the index that's exposed to this block
        printf("%d: %d\n", i, *v);
    }
    mp_endforeach();
 * ```
 */
#define mp_foreach(element, vector)                                                                \
    for (size_t i = 0; i < (vector)->size; ++i) {                                                  \
        element = &(vector)->data[i];
#define mp_endforeach() }

/***********
 * END OF VECTOR
 ***********/

/***********
 * IMPLEMENTATION
 ***********/
#ifdef MEMPLUS_IMPLEMENTATION

/* Functions that are used by `mp_arena_new_allocator` to define the arena allocator. */
static void *mp_arena_alloc(mp_Arena *self, size_t size);
static void *mp_arena_realloc(mp_Arena *self, void *old_ptr, size_t old_size, size_t new_size);
static void *mp_arena_dup(mp_Arena *self, void *data, size_t size);

mp_Region *mp_region_new(size_t capacity) {
    size_t     bytes  = sizeof(mp_Region) + sizeof(uintptr_t) * capacity;
    mp_Region *region = calloc(bytes, 1);
    _MEMPLUS_ASSERT(region != NULL);
    region->next     = NULL;
    region->count    = 0;
    region->capacity = capacity;
    return region;
}

void mp_region_free(mp_Region *self) {
    free(self);
}

void mp_arena_free(mp_Arena *self) {
    mp_Region *region = self->begin;
    while (region) {
        mp_Region *region_temp = region;
        region                 = region->next;
        mp_region_free(region_temp);
    }
    self->begin = NULL;
    self->end   = NULL;
}

mp_Allocator mp_arena_new_allocator(mp_Arena *arena) {
    return mp_allocator_new(arena, mp_arena_alloc, mp_arena_realloc, mp_arena_dup);
}

static void *mp_arena_alloc(mp_Arena *self, size_t size) {
    // size in word/qword (64 bits)
    size_t size_word = (size + sizeof(uintptr_t) - 1) / sizeof(uintptr_t);

    if (self->end == NULL) {
        _MEMPLUS_ASSERT(self->begin == NULL);
        size_t capacity = MP_REGION_DEFAULT_SIZE;
        if (capacity < size_word) capacity = size_word;
        self->end   = mp_region_new(capacity);
        self->begin = self->end;
    }

    while (self->end->count + size_word > self->end->capacity && self->end->next != NULL) {
        self->end = self->end->next;
    }

    if (self->end->count + size_word > self->end->capacity) {
        _MEMPLUS_ASSERT(self->end->next == NULL);
        size_t capacity = MP_REGION_DEFAULT_SIZE;
        if (capacity < size_word) capacity = size_word;
        self->end->next = mp_region_new(capacity);
        self->end       = self->end->next;
    }

    void *result = &self->end->data[self->end->count];
    self->end->count += size_word;
    return result;
}

static void *mp_arena_realloc(mp_Arena *self, void *old_ptr, size_t old_size, size_t new_size) {
    if (new_size <= old_size) return old_ptr;
    void    *new_ptr      = mp_arena_alloc(self, new_size);
    uint8_t *new_ptr_byte = (uint8_t *)new_ptr;
    uint8_t *old_ptr_byte = (uint8_t *)old_ptr;
    for (size_t i = 0; i < old_size; ++i) {
        new_ptr_byte[i] = old_ptr_byte[i];
    }
    return new_ptr;
}

static void *mp_arena_dup(mp_Arena *self, void *data, size_t size) {
    return memcpy(mp_arena_alloc(self, size), data, size);
}

mp_String mp_string_new(mp_Allocator *allocator, const char *str) {
    int size = snprintf(NULL, 0, "%s", str);
    _MEMPLUS_ASSERT(size > 0 && "failed to count string size");
    char *result      = mp_allocator_alloc(allocator, (size_t)size + 1);
    int   result_size = snprintf(result, (size_t)size + 1, "%s", str);
    _MEMPLUS_ASSERT(result_size == size);
    return (mp_String){ (size_t)result_size, result };
}

mp_String mp_string_newf(mp_Allocator *allocator, const char *fmt, ...) {
    va_list args;

    va_start(args, fmt);
    int size = vsnprintf(NULL, 0, fmt, args);
    _MEMPLUS_ASSERT(size > 0 && "failed to count string size");
    va_end(args);

    char *result = mp_allocator_alloc(allocator, (size_t)size + 1);

    va_start(args, fmt);
    int result_size = vsnprintf(result, (size_t)size + 1, fmt, args);
    _MEMPLUS_ASSERT(result_size == size);
    va_end(args);

    return (mp_String){ (size_t)result_size, result };
}

mp_String mp_string_dup(mp_Allocator *allocator, mp_String str) {
    int size = snprintf(NULL, 0, "%s", str.cstr);
    _MEMPLUS_ASSERT((size > 0 || (size_t)size != str.size) && "failed to count string size");
    char *ptr = mp_allocator_dup(allocator, str.cstr, (size_t)size);
    return (mp_String){ (size_t)size, ptr };
}

#endif /* ifdef MEMPLUS_IMPLEMENTATION */
/***********
 * END OF IMPLEMENTATION
 ***********/

#endif /* ifndef MEMPLUS_H__ */
