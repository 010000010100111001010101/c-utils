#ifndef MAP_H
#define MAP_H

#include "list.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct list list;

typedef enum {
    M_TYPE_BOOL,
    M_TYPE_CHAR,
    M_TYPE_DOUBLE,
    M_TYPE_GENERIC,
    M_TYPE_INT,
    M_TYPE_LIST,
    M_TYPE_MAP,
    M_TYPE_NULL,
    M_TYPE_SIZE_T,
    M_TYPE_STRING,

    M_TYPE_RESERVED_ERROR,
    M_TYPE_RESERVED_EMPTY
} mtype;

typedef struct node node;

typedef struct map {
    uint32_t seed;

    node **nodes;
    size_t length;
    size_t size;

    node *first;
    node *last;
} map;

typedef struct mapiter {
    const map *m;
    const node *n;
} mapiter;

map *map_init(void);
map *map_copy(const map *);
bool map_resize(map *, size_t);

size_t map_get_length(const map *);
size_t map_get_size(const map *);
/* const char *map_to_string(const map *); */

mapiter *map_iter_init(const map *);
bool map_iter_is_last(const mapiter *);
bool map_iter_get_key(const mapiter *, mtype *, size_t *, const void **);
bool map_iter_get_value(const mapiter *, mtype *, size_t *, const void **);
bool map_iter_next(mapiter *);
bool map_iter_prev(mapiter *);
void map_iter_free(mapiter *);

bool map_contains(const map *, size_t, const void *);
mtype map_get_type(const map *, size_t, const void *);
bool map_get_bool(const map *, size_t, const void *);
char map_get_char(const map *, size_t, const void *);
double map_get_double(const map *, size_t, const void *);
int64_t map_get_int(const map *, size_t, const void *);
size_t map_get_size_t(const map *, size_t, const void *);
const char *map_get_string(const map *, size_t, const void *);
const list *map_get_list(const map *, size_t, const void *);
const map *map_get_map(const map *, size_t, const void *);
const void *map_get_generic(const map *, size_t, const void *);

bool map_set_pointer(map *, mtype, size_t, const void *, mtype, size_t, void *);
bool map_set(map *, mtype, size_t, const void *, mtype, size_t, const void *);

bool map_remove(map *, size_t, const void *);
void map_free(map *);

#endif
