#ifndef _HAMT_H
#define _HAMT_H

/* Interfaces */

struct hamt;

typedef struct hamt_item {
    void *key;
    void *value;
    intptr_t hash;
} Item;

struct hamt_ops {
    int (*eq)(void *k1, void *k2);
    int (*hash)(void *k);
    int (*unreference)(struct hamt_item *k);
};

#endif
