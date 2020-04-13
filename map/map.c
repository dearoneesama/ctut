#include <string.h>
#include <stdlib.h>
#include "map.h"

#define NEW(TYPE, LEN) (TYPE*)malloc(sizeof(TYPE) * LEN)

static size_t df_hashcode(Map *map, void *key) {
    // assumes key is string
    const char *k = (const char*)key;
    size_t h = 0;
    while (*k) {
        h = (h << 4) + *k++;
        size_t g = h & 0xF0000000L;
        if (g)
            h ^= g >> 24;
        h &= ~g;
    }
    return h % map->list_size;
}

static bool df_k_equal(void *key1, void *key2) {
    return strcmp((const char*)key1, (const char*)key2) ? false : true;
}

static int reset_hashmap(Map *map, size_t newlistsize) {
    if (newlistsize == 0)
        newlistsize = 8;
    if (newlistsize < 8)
        return 0;

    // put everything in temp storage
    size_t npairs = map->size;
    MapEntry *tmplist = NEW(MapEntry, newlistsize);
    if (tmplist == NULL)
        return 1;
    MapIterator *it = createHashMapIterator(map);
    if (it == NULL) {
        free(tmplist);
        return 1;
    }
    for (size_t i = 0; it->has_next(it); ++i) {
        it = it->next(it);
        tmplist[i].key = it->curr->key;
        tmplist[i].value = it->curr->value;
        tmplist[i].next = NULL;
    }
    it->free(it);

    // clear data of existing k-v pairs
    map->size = 0;
    for (size_t i = 0; i < map->list_size; ++i) {
        MapEntry *curr = &map->list[i];
        curr->key = NULL;
        curr->value = NULL;
        while (curr->next != NULL) {
            MapEntry *tmp = curr->next->next;
            free(curr->next);
            curr->next = tmp;
        }
    }

    // reinitialize list
    MapEntry *newlist = (MapEntry*)realloc(map->list, sizeof(MapEntry) * newlistsize);
    if (newlist != NULL) {
        map->list_size = newlistsize;
        map->list = newlist;
    }
    for (size_t i = 0; i < map->list_size; ++i)
        memset(&map->list[i], 0, sizeof(MapEntry));

    // re-put all the k-v pairs
    for (size_t i = 0; i < npairs; ++i)
        map->put(map, tmplist[i].key, tmplist[i].value);

    free(tmplist);
    if (newlist == NULL)
        return 1;
    return 0;
}

static int df_put(Map *map, void *key, void *value) {
    // scatter v-k pairs by incrementing max index
    if (map->auto_assign && map->size >= map->list_size)
        if (reset_hashmap(map, map->list_size * 2) != 0)
            return 1;

    size_t index = map->hashcode(map, key);
    if (map->list[index].key == NULL) {
        // if no such key
        ++map->size;
        map->list[index].key = key;
        map->list[index].value = value;
    } else {
        // existing key
        MapEntry *curr = &map->list[index];
        while (curr != NULL) {
            if (map->k_equal(key, curr->key)) {
                curr->value = value;
                return 0;
            }
            curr = curr->next;
        }
        // name collision!
        MapEntry *newentry = NEW(MapEntry, 1);
        if (newentry == NULL)
            return 1;
        ++map->size;
        newentry->key = key;
        newentry->value = value;
        newentry->next = map->list[index].next;
        map->list[index].next = newentry;
    }
    return 0;
}

static void *df_get(Map *map, void *key) {
    size_t index = map->hashcode(map, key);
    MapEntry *entry = &map->list[index];
    while (entry->key != NULL && !map->k_equal(key, entry->key))
        entry = entry->next;
    return entry->value;
}

static void df_remove(Map *map, void *key) {
    size_t index = map->hashcode(map, key);
    MapEntry *entry = &map->list[index];
    if (entry == NULL)
        return;

    bool res = false;
    if (map->k_equal(key, entry->key)) {
        --map->size;
        if (entry->next != NULL) {
            MapEntry *tmp = entry->next;
            entry->key = tmp->key;
            entry->value = tmp->value;
            entry->next = tmp->next;
            free(tmp);
        } else {
            entry->key = NULL;
            entry->value = NULL;
        }
        res = true;
    } else {
        MapEntry *prev = entry;
        entry = entry->next;
        while (entry != NULL) {
            if (map->k_equal(key, entry->key)) {
                --map->size;
                prev->next = entry->next;
                free(entry);
                res = true;
                break;
            }
            prev = entry;
            entry = entry->next;
        }
    }

    // shirnk size if half of list is not occupied
    if (res && map->auto_assign && map->size < map->list_size / 2)
        (void)reset_hashmap(map, map->list_size / 2);
}

static bool df_exists(Map *map, void *key) {
    size_t index = map->hashcode(map, key);
    MapEntry *entry = &map->list[index];
    if (entry->key == NULL)
        return false;
    if (map->k_equal(map, entry->key))
        return true;
    entry = entry->next;
    while (entry != NULL) {
        if (map->k_equal(map, entry->key))
            return true;
        entry = entry->next;
    }
    return false;
}

static void df_clear(Map *map) {
    // free conflict lists
    for (size_t i = 0; i < map->list_size; ++i) {
        MapEntry *entry = map->list[i].next;
        while (entry != NULL) {
            MapEntry *next = entry->next;
            free(entry);
            entry = next;
        }
        map->list[i].next = NULL;
    }
    // free store (first level MapEntry)
    free(map->list);
    map->list = NULL;
    map->size = 0;
    map->list_size = 0;
}

static void df_free(Map *map) {
    map->clear(map);
    free(map);
    map = NULL;
}

Map *createHashMap(hashcode_func hashcode, key_equal_func k_equal) {
    Map *map = NEW(Map, 1);
    if (map == NULL)
        return NULL;
    MapEntry *list = NEW(MapEntry, 8);
    if (list == NULL) {
        free(map);
        return NULL;
    }
    for (size_t i = 0; i < 8; ++i)
        memset(&list[i], 0, sizeof(MapEntry));
    map->size = 0;
    map->list_size = 8;
    map->auto_assign = true;
    map->list = list;
    map->hashcode = hashcode == NULL ? df_hashcode : hashcode;
    map->k_equal = k_equal == NULL ? df_k_equal : k_equal;
    map->put = df_put;
    map->get = df_get;
    map->remove = df_remove;
    map->exists = df_exists;
    map->clear = df_clear;
    map->free = df_free;
    return map;
}

static bool mapiter_has_next(MapIterator *it) {
    return it->count < it->map->size;
}

static MapIterator *mapiter_next(MapIterator *it) {
    if (it->has_next(it)) {
        // collision
        if (it->curr != NULL && it->curr->next != NULL) {
            ++it->count;
            it->curr = it->curr->next;
            return it;
        }
        // different key hashcode
        while (++it->hashcode < it->map->list_size) {
            MapEntry *entry = &it->map->list[it->hashcode];
            if (entry->key != NULL) {
                ++it->count;
                it->curr = entry;
                break;
            }
        }
    }
    return it;
}

static void mapiter_free(MapIterator *it) {
    free(it);
    it = NULL;
}

MapIterator *createHashMapIterator(Map *map) {
    MapIterator *it = NEW(MapIterator, 1);
    if (it == NULL)
        return NULL;
    it->map = map;
    it->curr = NULL;
    it->count = 0;
    it->hashcode = (size_t)-1;
    it->has_next = mapiter_has_next;
    it->next = mapiter_next;
    it->free = mapiter_free;
}


