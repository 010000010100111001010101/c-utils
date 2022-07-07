#include "list.h"

#include "log.h"
#include "str.h"

#include <stdio.h>
#include <stdlib.h>

#define LIST_MINIMUM_SIZE 8

/*#define LIST_GROWTH_LOAD_FACTOR 1*/
#define LIST_GROWTH_FACTOR 1.5

#define LIST_SHRINK_LOAD_FACTOR 0.25
#define LIST_SHRINK_FACTOR 0.5

typedef struct node {
    ltype type;
    size_t size;
    void *data;
} node;

static logctx *logger = NULL;

static size_t calculate_new_size(size_t s){
    return (s <= 1 ? s + 1 : s) * LIST_GROWTH_FACTOR;
}

static node *node_init_pointer(ltype type, size_t size, void *data){
    node *n = malloc(sizeof(*n));

    if (!n){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] node_init_pointer() - alloc for node object failed\n",
            __FILE__
        );

        return NULL;
    }

    n->type = type;
    n->size = size;
    n->data = data;

    return n;
}

static node *node_init(ltype type, size_t size, const void *data){
    node *n = malloc(sizeof(*n));

    if (!n){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] node_init() - node alloc failed\n",
            __FILE__
        );

        return NULL;
    }

    n->type = type;
    n->size = size;

    if (type == L_TYPE_LIST){
        n->data = list_copy(data);

        if (!n->data){
            log_write(
                logger,
                LOG_ERROR,
                "[%s] node_init() - list copy failed\n",
                __FILE__
            );

            free(n);

            return NULL;
        }
    }
    else if (type == L_TYPE_MAP){
        n->data = map_copy(data);

        if (!n->data){
            log_write(
                logger,
                LOG_ERROR,
                "[%s] node_init() - map copy failed\n",
                __FILE__
            );

            free(n);

            return NULL;
        }
    }
    else if (type == L_TYPE_NULL){
        n->data = NULL;
    }
    else if (type == L_TYPE_STRING){
        n->data = malloc(size + 1);

        if (!n->data){
            log_write(
                logger,
                LOG_ERROR,
                "[%s] node_init() - node data alloc failed\n",
                __FILE__
            );

            free(n);

            return NULL;
        }

        string_copy(data, n->data, size);
    }
    else {
        n->data = malloc(size);

        if (!n->data){
            log_write(
                logger,
                LOG_ERROR,
                "[%s] node_init() - node data alloc failed\n",
                __FILE__
            );

            free(n);

            return NULL;
        }

        memcpy(n->data, data, size);
    }

    return n;
}

static void node_free(node *n){
    if (!n){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] node_free() - node is NULL\n",
            __FILE__
        );

        return;
    }

    switch (n->type){
    case L_TYPE_LIST:
        list_free(n->data);

        break;
    case L_TYPE_MAP:
        map_free(n->data);

        break;
    case L_TYPE_NULL:
        break;
    default:
        free(n->data);
    }

    free(n);
}

static node *get_node(const list *l, size_t pos, ltype type){
    if (!l){
        log_write(
            logger,
            LOG_WARNING,
            "[%s] get_node() - list is NULL\n",
            __FILE__
        );

        return NULL;
    }
    else if (pos >= l->length){
        log_write(
            logger,
            LOG_WARNING,
            "[%s] get_node() - position out of range\n",
            __FILE__
        );

        return NULL;
    }

    node *n = l->nodes[pos];

    if (!n){
        log_write(
            logger,
            LOG_WARNING,
            "[%s] get_node() - unable to get node\n",
            __FILE__
        );
    }
    else if (type != L_TYPE_RESERVED_EMPTY && n->type != type){
        log_write(
            logger,
            LOG_WARNING,
            "[%s] get_node() - node type does *not* match!\n",
            __FILE__
        );
    }

    return n;
}

list *list_init(void){
    list *l = calloc(1, sizeof(*l));

    if (!l){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] list_init() - list alloc failed\n",
            __FILE__
        );

        return NULL;
    }

    l->length = 0;
    l->size = LIST_MINIMUM_SIZE;
    l->nodes = calloc(l->size, sizeof(*l->nodes));

    if (!l->nodes){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] list_init() - nodes allocation failed\n",
            __FILE__
        );

        free(l);

        return NULL;
    }

    return l;
}

list *list_copy(const list *l){
    if (!l){
        log_write(
            logger,
            LOG_WARNING,
            "[%s] list_copy() - list is NULL\n",
            __FILE__
        );

        return NULL;
    }

    list *copy = list_init();

    if (!copy){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] list_copy() - list alloc failed\n",
            __FILE__
        );

        return NULL;
    }

    for (size_t index = 0; index < l->length; ++index){
        const node *n = get_node(l, index, L_TYPE_RESERVED_EMPTY);

        if (!list_append(copy, n->type, n->size, n->data)){
            log_write(
                logger,
                LOG_ERROR,
                "[%s] list_copy() - node copy failed\n",
                __FILE__
            );

            list_free(copy);

            return NULL;
        }
    }

    return copy;
}

bool list_resize(list *l, size_t size){
    if (!l){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] list_resize() - list is NULL\n",
            __FILE__
        );

        return false;
    }
    else if (size == 0){
        log_write(
            logger,
            LOG_WARNING,
            "[%s] list_resize() - size cannot be 0\n",
            __FILE__
        );

        return false;
    }
    else if (size < LIST_MINIMUM_SIZE){
        log_write(
            logger,
            LOG_WARNING,
            "[%s] list_resize() - size cannot be less than LIST_MINIMUM_SIZE (%d)\n",
            __FILE__,
            LIST_MINIMUM_SIZE
        );

        return false;
    }

    if (size < l->length){
        for (size_t index = size; index < l->length; ++index){
            node_free(l->nodes[index]);
        }

        l->length = size;
    }

    node **nodes = realloc(l->nodes, size * sizeof(*nodes));

    if (!nodes){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] list_resize() - list initialization failed\n",
            __FILE__
        );

        return false;
    }

    l->nodes = nodes;
    l->size = size;

    return true;
}

size_t list_get_length(const list *l){
    if (!l){
        log_write(
            logger,
            LOG_WARNING,
            "[%s] list_get_length() - list is NULL\n",
            __FILE__
        );

        return 0;
    }

    return l->length;
}

size_t list_get_size(const list *l){
    if (!l){
        log_write(
            logger,
            LOG_WARNING,
            "[%s] list_get_size() - list is NULL\n",
            __FILE__
        );

        return 0;
    }

    return l->size;
}

size_t list_get_item_size(const list *l, size_t pos){
    const node *n = get_node(l, pos, L_TYPE_RESERVED_EMPTY);

    if (!n){
        return 0;
    }

    return n->size;
}

bool list_contains(const list *l, size_t size, const void *data){
    if (!l){
        log_write(
            logger,
            LOG_WARNING,
            "[%s] list_contains() - list is NULL\n",
            __FILE__
        );

        return false;
    }

    for (size_t index = 0; index < l->length; ++index){
        const node *n = get_node(l, index, L_TYPE_RESERVED_EMPTY);

        if (size == n->size && memcmp(data, n->data, n->size)){
            return true;
        }
    }

    return false;
}

ltype list_get_type(const list *l, size_t pos){
    const node *n = get_node(l, pos, L_TYPE_RESERVED_EMPTY);

    if (!n){
        return L_TYPE_RESERVED_ERROR;
    }

    return n->type;
}

bool list_get_bool(const list *l, size_t pos){
    const node *n = get_node(l, pos, L_TYPE_BOOL);

    if (!n){
        return false;
    }

    return *(bool *)n->data;
}

char list_get_char(const list *l, size_t pos){
    const node *n = get_node(l, pos, L_TYPE_CHAR);

    if (!n){
        return 0;
    }

    return *(char *)n->data;
}

double list_get_double(const list *l, size_t pos){
    const node *n = get_node(l, pos, L_TYPE_DOUBLE);

    if (!n){
        return 0.0;
    }

    return *(double *)n->data;
}

int64_t list_get_int(const list *l, size_t pos){
    const node *n = get_node(l, pos, L_TYPE_INT);

    if (!n){
        return 0;
    }

    return *(int64_t *)n->data;
}

size_t list_get_size_t(const list *l, size_t pos){
    const node *n = get_node(l, pos, L_TYPE_SIZE_T);

    if (!n){
        return 0;
    }

    return *(size_t *)n->data;
}

/*
 * READ WARNING FOR THESE FUNCTIONS IN HEADER FILE
 */
char *list_get_string(const list *l, size_t pos){
    const node *n = get_node(l, pos, L_TYPE_STRING);

    if (!n){
        return NULL;
    }

    return n->data;
}

list *list_get_list(const list *l, size_t pos){
    const node *n = get_node(l, pos, L_TYPE_LIST);

    if (!n){
        return NULL;
    }

    return n->data;
}

map *list_get_map(const list *l, size_t pos){
    const node *n = get_node(l, pos, L_TYPE_MAP);

    if (!n){
        return NULL;
    }

    return n->data;
}

void *list_get_generic(const list *l, size_t pos){
    const node *n = get_node(l, pos, L_TYPE_GENERIC);

    if (!n){
        return NULL;
    }

    return n->data;
}

bool list_replace_pointer(list *l, size_t pos, ltype type, size_t size, void *data){
    if (!l){
        log_write(
            logger,
            LOG_WARNING,
            "[%s] list_replace_pointer() - list is NULL\n",
            __FILE__
        );

        return false;
    }
    else if (pos >= l->length){
        log_write(
            logger,
            LOG_WARNING,
            "[%s] list_replace_pointer() - pos greater than list length\n",
            __FILE__
        );

        return false;
    }

    node *n = node_init_pointer(type, size, data);

    if (!n){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] list_replace_pointer() - node initialization failed\n",
            __FILE__
        );

        return false;
    }

    node_free(l->nodes[pos]);

    l->nodes[pos] = n;

    return true;
}

bool list_replace(list *l, size_t pos, ltype type, size_t size, const void *data){
    if (!l){
        log_write(
            logger,
            LOG_WARNING,
            "[%s] list_replace() - list is NULL\n",
            __FILE__
        );

        return false;
    }
    else if (pos >= l->length){
        log_write(
            logger,
            LOG_WARNING,
            "[%s] list_replace() - pos greater than list length\n",
            __FILE__
        );

        return false;
    }

    node *n = node_init(type, size, data);

    if (!n){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] list_replace() - node initialization failed\n",
            __FILE__
        );

        return false;
    }

    node_free(l->nodes[pos]);

    l->nodes[pos] = n;

    return true;
}

bool list_insert_pointer(list *l, size_t pos, ltype type, size_t size, void *data){
    if (!l){
        log_write(
            logger,
            LOG_WARNING,
            "[%s] list_insert_pointer() - list is NULL\n",
            __FILE__
        );

        return false;
    }
    else if (pos > l->length){
        return list_append_pointer(l, type, size, data);
    }

    size_t newlen = l->length + 1;

    if (newlen >= l->size){
        if (!list_resize(l, calculate_new_size(l->size))){
            log_write(
                logger,
                LOG_ERROR,
                "[%s] list_insert_pointer() - list resizing failed\n",
                __FILE__
            );

            return false;
        }
    }

    node *n = node_init_pointer(type, size, data);

    if (!n){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] list_insert_pointer() - node initialization failed\n",
            __FILE__
        );

        return false;
    }

    for (size_t index = l->length; index > pos; --index){
        l->nodes[index] = l->nodes[index - 1];
    }

    l->nodes[pos] = n;
    l->length = newlen;

    return true;
}

bool list_insert(list *l, size_t pos, ltype type, size_t size, const void *data){
    if (!l){
        log_write(
            logger,
            LOG_WARNING,
            "[%s] list_insert() - list is NULL\n",
            __FILE__
        );

        return false;
    }
    else if (pos > l->length){
        return list_append(l, type, size, data);
    }

    size_t newlen = l->length + 1;

    if (newlen >= l->size){
        if (!list_resize(l, calculate_new_size(l->size))){
            log_write(
                logger,
                LOG_ERROR,
                "[%s] list_insert() - list resizing failed\n",
                __FILE__
            );

            return false;
        }
    }

    node *n = node_init(type, size, data);

    if (!n){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] list_insert() - node initialization failed\n",
            __FILE__
        );

        return false;
    }

    for (size_t index = l->length; index > pos; --index){
        l->nodes[index] = l->nodes[index - 1];
    }

    l->nodes[pos] = n;
    l->length = newlen;

    return true;
}

bool list_append_pointer(list *l, ltype type, size_t size, void *data){
    if (!l){
        log_write(
            logger,
            LOG_WARNING,
            "[%s] list_append_pointer() - list is NULL\n",
            __FILE__
        );

        return false;
    }

    size_t newlen = l->length + 1;

    if (newlen >= l->size){
        if (!list_resize(l, calculate_new_size(l->size))){
            log_write(
                logger,
                LOG_ERROR,
                "[%s] list_append_pointer() - list resizing failed\n",
                __FILE__
            );

            return false;
        }
    }

    node *n = node_init_pointer(type, size, data);

    if (!n){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] list_append_pointer() - node initialization failed\n",
            __FILE__
        );

        return false;
    }

    l->nodes[l->length] = n;
    l->length = newlen;

    return true;
}

bool list_append(list *l, ltype type, size_t size, const void *data){
    if (!l){
        log_write(
            logger,
            LOG_WARNING,
            "[%s] list_append() - list is NULL\n",
            __FILE__
        );

        return false;
    }

    size_t newlen = l->length + 1;

    if (newlen >= l->size){
        if (!list_resize(l, calculate_new_size(l->size))){
            log_write(
                logger,
                LOG_ERROR,
                "[%s] list_append() - list resizing failed\n",
                __FILE__
            );

            return false;
        }
    }

    node *n = node_init(type, size, data);

    if (!n){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] list_append() - node initialization failed\n",
            __FILE__
        );

        return false;
    }

    l->nodes[l->length] = n;
    l->length = newlen;

    return true;
}

void list_pop(list *l, size_t pos, ltype *type, size_t *size, void **data){
    node *n = get_node(l, pos, L_TYPE_RESERVED_EMPTY);

    if (type){
        *type = n ? n->type : L_TYPE_RESERVED_ERROR;
    }

    if (size){
        *size = n ? n->size : 0;
    }

    if (data){
        *data = n ? n->data : NULL;

        n->type = L_TYPE_NULL;
        n->size = 0;
        n->data = NULL;
    }

    list_remove(l, pos);
}

void list_remove(list *l, size_t pos){
    if (!l){
        log_write(
            logger,
            LOG_WARNING,
            "[%s] list_remove() - list is NULL\n",
            __FILE__
        );

        return;
    }
    else if (pos >= l->length){
        log_write(
            logger,
            LOG_WARNING,
            "[%s] list_remove() - pos out of range\n",
            __FILE__
        );

        return;
    }

    node_free(l->nodes[pos]);

    for (size_t index = pos; index < l->length; ++index){
        l->nodes[index] = l->nodes[index + 1];
    }

    --l->length;

    double load = (double)l->length / (double)l->size;

    if (load <= LIST_SHRINK_LOAD_FACTOR){
        size_t newsize = l->size - ((l->size - l->length) * LIST_SHRINK_FACTOR);

        if (newsize >= l->size){
            log_write(
                logger,
                LOG_WARNING,
                "[%s] list_remove() - newsize >= l->size\n",
                __FILE__
            );

            return;
        }

        if (!list_resize(l, newsize)){
            log_write(
                logger,
                LOG_WARNING,
                "[%s] list_remove() - unable to shrink list\n",
                __FILE__
            );

            return;
        }
    }
}

void list_empty(list *l){
    for (size_t index = 0; index < l->length; ++index){
        list_remove(l, index);
    }
}

void list_free(list *l){
    if (!l){
        log_write(
            logger,
            LOG_WARNING,
            "[%s] list_free() - list is NULL\n",
            __FILE__
        );

        return;
    }

    for (size_t index = 0; index < l->length; ++index){
        node_free(l->nodes[index]);
    }

    free(l->nodes);
    free(l);
}
