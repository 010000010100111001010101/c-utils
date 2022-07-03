#include "json_utils.h"

#include "list.h"
#include "log.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

static logctx *logger = NULL;

static bool add_json_array_value_to_map(map *m, const char *key, json_object *value){
    list *tmp = json_array_to_list(value);

    if (!tmp){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] add_json_array_value() - failed to create list\n",
            __FILE__
        );

        return false;
    }

    return map_set_pointer(
        m,
        M_TYPE_STRING,
        strlen(key),
        key,
        M_TYPE_LIST,
        sizeof(*tmp),
        tmp
    );
}

static bool add_json_bool_value_to_map(map *m, const char *key, json_object *value){
    bool tmp = json_object_get_boolean(value);

    return map_set(
        m,
        M_TYPE_STRING,
        strlen(key),
        key,
        M_TYPE_BOOL,
        sizeof(tmp),
        &tmp
    );
}

static bool add_json_double_value_to_map(map *m, const char *key, json_object *value){
    double tmp = json_object_get_double(value);

    return map_set(
        m,
        M_TYPE_STRING,
        strlen(key),
        key,
        M_TYPE_DOUBLE,
        sizeof(tmp),
        &tmp
    );
}

static bool add_json_int_value_to_map(map *m, const char *key, json_object *value){
    int64_t tmp = json_object_get_int64(value);

    return map_set(
        m,
        M_TYPE_STRING,
        strlen(key),
        key,
        M_TYPE_INT,
        sizeof(tmp),
        &tmp
    );
}

static bool add_json_object_value_to_map(map *m, const char *key, json_object *value){
    map *tmp = json_to_map(value);

    if (!tmp){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] add_json_object_value() - failed to create map\n",
            __FILE__
        );

        return false;
    }

    return map_set_pointer(
        m,
        M_TYPE_STRING,
        strlen(key),
        key,
        M_TYPE_MAP,
        sizeof(*tmp),
        tmp
    );
}

static bool add_json_string_value_to_map(map *m, const char *key, json_object *value){
    size_t tmplen = json_object_get_string_len(value);
    const char *tmp = json_object_get_string(value);

    return map_set(
        m,
        M_TYPE_STRING,
        strlen(key),
        key,
        M_TYPE_STRING,
        tmplen,
        tmp
    );
}

list *json_array_to_list(json_object *value){
    if (json_object_get_type(value) != json_type_array){
        log_write(
            logger,
            LOG_WARNING,
            "[%s] json_array_to_list() - value is not a JSON array\n",
            __FILE__
        );

        return NULL;
    }

    size_t length = json_object_array_length(value);
    list *l = list_init();

    if (!l){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] json_array_to_list() - list initialization failed\n",
            __FILE__
        );

        return NULL;
    }

    bool success = true;

    for (size_t index = 0; index < length; ++index){
        json_object *item = json_object_array_get_idx(value, index);

        if (!item){
            log_write(
                logger,
                LOG_WARNING,
                "[%s] json_array_to_list() - array item is NULL\n",
                __FILE__
            );

            continue;
        }

        json_type type = json_object_get_type(item);

        /* this one calls recursively... any better way? */
        if (type == json_type_array){
            list *tmp = json_array_to_list(item);

            if (!tmp){
                log_write(
                    logger,
                    LOG_ERROR,
                    "[%s] json_array_to_list() - recursive call failed\n",
                    __FILE__
                );

                success = false;

                break;
            }

            success = list_append_pointer(
                l,
                L_TYPE_LIST,
                sizeof(*tmp),
                tmp
            );
        }
        else if (type == json_type_boolean){
            bool tmp = json_object_get_boolean(item);

            success = list_append(
                l,
                L_TYPE_BOOL,
                sizeof(tmp),
                &tmp
            );
        }
        else if (type == json_type_double){
            double tmp = json_object_get_double(item);

            success = list_append(
                l,
                L_TYPE_DOUBLE,
                sizeof(tmp),
                &tmp
            );
        }
        else if (type == json_type_int){
            int64_t tmp = json_object_get_int64(item);

            success = list_append(
                l,
                L_TYPE_INT,
                sizeof(tmp),
                &tmp
            );
        }
        else if (type == json_type_object){
            map *tmp = json_to_map(item);

            if (!tmp){
                log_write(
                    logger,
                    LOG_ERROR,
                    "[%s] json_array_to_list() - conversion to map failed\n",
                    __FILE__
                );

                success = false;

                break;
            }

            success = list_append_pointer(
                l,
                L_TYPE_MAP,
                sizeof(*tmp),
                tmp
            );
        }
        else if (type == json_type_string){
            const char *tmp = json_object_get_string(item);

            success = list_append(
                l,
                L_TYPE_STRING,
                json_object_get_string_len(item),
                tmp
            );
        }
        else {
            log_write(
                logger,
                LOG_ERROR,
                "[%s] json_array_to_list() - unexpected object type\n",
                __FILE__
            );

            success = false;
        }

        if (!success){
            break;
        }
    }

    if (!success){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] json_array_to_list() - failed to set list items\n",
            __FILE__
        );

        list_free(l);

        return NULL;
    }

    return l;
}

json_object *list_to_json_array(const list *l){
    if (!l){
        log_write(
            logger,
            LOG_WARNING,
            "[%s] list_to_json_array() - list is NULL\n",
            __FILE__
        );

        return NULL;
    }

    json_object *array = json_object_new_array();

    if (!array){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] list_to_json_array() - array object intitialization failed\n",
            __FILE__
        );

        return NULL;
    }

    size_t length = list_get_length(l);

    for (size_t index = 0; index < length; ++index){
        ltype type = list_get_type(l, index);

        char tmpstr[2] = {0};
        json_object *obj = NULL;

        switch (type){
        case L_TYPE_BOOL:
            obj = json_object_new_boolean(list_get_bool(l, index));

            break;
        case L_TYPE_CHAR:
            tmpstr[0] = list_get_char(l, index);
            obj = json_object_new_string(tmpstr);

            break;
        case L_TYPE_DOUBLE:
            obj = json_object_new_double(list_get_double(l, index));

            break;
        case L_TYPE_INT:
            obj = json_object_new_int64(list_get_int(l, index));

            break;
        case L_TYPE_LIST:
            obj = list_to_json_array(list_get_list(l, index));

            break;
        case L_TYPE_MAP:
            log_write(
                logger,
                LOG_WARNING,
                "[%s] list_to_json_array() - map_to_json is not implemented yet\n",
                __FILE__
            );

            break;
        case L_TYPE_NULL:
            obj = NULL;

            break;
        case L_TYPE_STRING:
            obj = json_object_new_string(list_get_string(l, index));

            break;
        default:
            log_write(
                logger,
                LOG_WARNING,
                "[%s] list_to_json_array() - unexpected object type at index %ld\n",
                __FILE__,
                index
            );
        }

        if (type != L_TYPE_NULL && !obj){
            log_write(
                logger,
                LOG_ERROR,
                "[%s] list_to_json_array() - failed to get json object\n",
                __FILE__
            );

            json_object_put(array);

            return NULL;
        }

        json_object_array_add(array, obj);
    }

    return array;
}

map *json_to_map(json_object *json){
    if (!json){
        log_write(
            logger,
            LOG_WARNING,
            "[%s] json_to_map() - json is NULL\n",
            __FILE__
        );

        return NULL;
    }

    map *m = map_init();

    if (!m){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] json_to_map() - map alloc failed\n",
            __FILE__
        );

        return NULL;
    }

    struct json_object_iterator curr = json_object_iter_begin(json);
    struct json_object_iterator end = json_object_iter_end(json);

    while (!json_object_iter_equal(&curr, &end)){
        const char *key = json_object_iter_peek_name(&curr);
        json_object *value = json_object_iter_peek_value(&curr); 

        bool success = false;
        json_type type = json_object_get_type(value);

        switch (type){
        case json_type_array:
            success = add_json_array_value_to_map(m, key, value);

            break;
        case json_type_boolean:
            success = add_json_bool_value_to_map(m, key, value);

            break;
        case json_type_double:
            success = add_json_double_value_to_map(m, key, value);

            break;
        case json_type_int:
            success = add_json_int_value_to_map(m, key, value);

            break;
        case json_type_object:
            success = add_json_object_value_to_map(m, key, value);

            break;
        case json_type_string:
            success = add_json_string_value_to_map(m, key, value);

            break;
        default:
            log_write(
                logger,
                LOG_ERROR,
                "[%s] json_to_map() - unexpected json type - %d\n",
                __FILE__,
                type
            );
        }

        if (!success){
            log_write(
                logger,
                LOG_ERROR,
                "[%s] json_to_map() - failed to add key: %s\n",
                __FILE__,
                key
            );

            map_free(m);

            return NULL;
        }

        json_object_iter_next(&curr);
    }

    return m;
}

json_object *map_to_json(const map *m){
    if (!m){
        log_write(
            logger,
            LOG_WARNING,
            "[%s] map_to_json() - map is NULL\n",
            __FILE__
        );

        return NULL;
    }

    json_object *json = json_object_new_object();

    if (!json){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] map_to_json() - alloc for json object failed\n",
            __FILE__
        );

        return NULL;
    }

    /* iterate through map and stack in json object --- BOOKMARK --- */

    return json;
}
