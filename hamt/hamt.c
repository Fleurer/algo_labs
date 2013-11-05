#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "hamt.h"

#ifdef HAMT_DEBUG_ENABLE
#define HAMT_DEBUG(fmt, ...) fprintf(stderr, "[hamt] %s(): " fmt, __func__, ##__VA_ARGS__)
#else
#define HAMT_DEBUG(fmt, ...)
#endif

#define HAMT_BITMAP_SIZE (sizeof(intptr_t) * 8)
#define HAMT_MAX_SLOTS HAMT_BITMAP_SIZE

typedef intptr_t Slot;

typedef struct hamt_node {
    intptr_t bitmap;
    Slot slots[0];
} Node;

/* to store the hash collision keys at the bottom of HAMT */
typedef struct hamt_vector {
    int32_t length;
    Item *items[0];
} Vector;

#define HAMT_LEAF_SLOT(p) ((Slot)((void*)(p)))
#define HAMT_NODE_SLOT(p) ((void*)HAMT_LEAF_SLOT(p) | 1)
#define HAMT_VECTOR_SLOT(p) (HAMT_LEAF_SLOT(p) | 2)

#define _SLOT_PTR(p) ((void*)((p) & ~3))
#define HAMT_SLOT_AS_NODE(p) ((Node*)_SLOT_PTR(p))
#define HAMT_SLOT_AS_VECTOR(p) ((Vector*)_SLOT_PTR(p))
#define HAMT_SLOT_AS_LEAF(p) ((Item*)_SLOT_PTR(p))

#define HAMT_SLOT_IS_NODE(s) ((Slot)(void*)s & 1)
#define HAMT_SLOT_IS_VECTOR(s) ((Slot)(void*)s & 2)
#define HAMT_SLOT_IS_LEAF(s) (! ((Slot)(void*)s & 3))

/* Hash Array Mapped Trie */

typedef struct hamt {
    struct hamt_node *root;
    struct hamt_ops ops;
} Hamt;

/* Bitmap Utilities */

/* count the set bits from a bitmap */
static uint8_t bitcount(uint32_t bitmap) {
    int mask = 1, i = 0, count = 0;

    for (i; i < HAMT_BITMAP_SIZE; i++) {
        if (mask & bitmap) {
            count++;
        }
        mask <<= 1;
    }
    return count;
}

unsigned int 
powerup(unsigned int n) 
{
    unsigned int power = 2;
    
    assert(n <= (INT_MAX >> 1));
    while (power < n) {
        power <<= 1;
    }
    return power;
}

/* hamt_node Utilities */

/* ensure sizeof(Node) is power of two */
uint16_t
node_calc_size(uint16_t slots_count) {
    uint16_t power = 2;

    while (power < (sizeof(Node) + slots_count * sizeof(Slot))) {
        power <<= 1;
    }
    return power;
}

static Node*
node_new(Hamt *hamt, uint8_t depth, Item *item1, Item *item2)
{
    Node *node;

    node = (Node *)malloc(node_calc_size(2));
    if (! node) {
        HAMT_DEBUG("out of memory");
        return NULL;
    }
    return node;
}


uint16_t
node_slots_count(Node *node) {
    return bitcount(node->bitmap);
}

uint16_t
node_map_slot_n(Node *node, uint16_t index) {
    intptr_t mask = (1 << index - 1);

    assert(index < HAMT_BITMAP_SIZE);
    return bitcount(node->bitmap & mask);
}

Slot*
node_find_slot(Node *node, uint16_t index) {
    uint16_t slot_n;
    intptr_t mask = 1 << index;

    if (! (node->bitmap & mask)) {
        return NULL;
    }
    slot_n = node_map_slot_n(node, index);
    return &node->slots[slot_n];
}

Node*
node_resize(Node *node, uint16_t slots_count) {
    return (Node *)realloc(node, node_calc_size(slots_count));
}

Node*
node_resize2(Node *node, int8_t diff) {
    uint16_t slots_count;

    slots_count = node_slots_count(node);
    if (slots_count == powerup(slots_count)) {
        return node;
    }
    slots_count += diff;

    assert(slots_count <= HAMT_MAX_SLOTS);
    return node_resize(node, slots_count);
}

/* this routine does NOT check slot overwrite, and do REALLOC 
 * the input node. */
Node*
node_insert_slot(Node *node, uint16_t index, Slot slot) {
    Slot *slot_p;
    unsigned int slot_n, slots_count;

    slot_p = node_find_slot(node, index);
    if (slot_p) {
        *slot_p = slot;
        return node;
    }

    node = node_resize2(node, 1);
    slot_n = node_map_slot_n(node, index);
    slots_count = node_slots_count(node);
    assert(slot_n < slots_count);
    memmove(
        (void*)&node->slots[slot_n], 
        (void*)&node->slots[slot_n+1], 
        slots_count - slot_n);
    return node;
}

int main(int argc, char *argv[]){
    assert(bitcount(0x01) == 1);
    assert(bitcount(0x02) == 1);
    assert(bitcount(0x03) == 2);
    assert(bitcount(0xff) == 8);
    assert(bitcount(0xffff) == 16);
    assert(powerup(2) == 2);
    assert(powerup(3) == 4);
    return 0;
}
