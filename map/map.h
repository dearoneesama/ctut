#pragma once

#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

struct Map;

typedef size_t (*hashcode_func)(struct Map *map, void *key);
typedef bool (*key_equal_func)(void *key1, void *key2);

// a key-value pair with same hashcode id. pairs with same hashcode
// are linked
typedef struct MapEntry {
    void *key;
    void *value;
    struct MapEntry *next; // linked list for conflict keys, may be NULL
} MapEntry;

// hashmap
typedef struct Map {
    size_t size;           // current k-v pairs
    size_t list_size;      // current max array size
    bool auto_assign;      // aynamic allocate memory?
    MapEntry *list;        // store. first level MapEntry must exist
    hashcode_func hashcode;                              // hashcode function
    key_equal_func k_equal;                              // checks whether two keys are equal
    int (*put)(struct Map *map, void *key, void *value); // add k and v
    void *(*get)(struct Map *map, void *key);            // gets k
    void (*remove)(struct Map *map, void *key);          // remove k
    bool (*exists)(struct Map *map, void *key);          // checks if key exists
    void (*clear)(struct Map *map);                      // clears all keys
    void (*free)(struct Map *map);                       // free the map, not key/values if they are from malloc
} Map;

typedef struct MapIterator {
    Map *map;
    MapEntry *curr;        // current k-v pair
    size_t count;          // nth iteration
    size_t hashcode;       // hashcode of curr
    bool (*has_next)(struct MapIterator *it);            // has next element?
    struct MapIterator *(*next)(struct MapIterator *it); // advance to next pair and return current it
    void (*free)(struct MapIterator *it);
} MapIterator;

// creates a new map with custom hashcode and equal functions.
// default ones for string are used if they are NULL
Map *createHashMap(hashcode_func hashcode, key_equal_func k_equal);

// create an iterator of map
MapIterator *createHashMapIterator(Map *map);

#ifdef __cplusplus
}
#endif
