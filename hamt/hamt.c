#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <assert.h>
#include <limits.h>
#include <string.h>
#include <stdio.h>
#include "hamt.h"

#ifdef HAMT_DEBUG_ENABLE
#define HAMT_DEBUG(fmt, ...) fprintf(stderr, "[hamt] %s(): " fmt, __func__, ##__VA_ARGS__)
#else
#define HAMT_DEBUG(fmt, ...)
#endif

/* key_size: 32
 * bitmap_size: 32
 * max_depth: roundup(key_size / log(bitmap_size)) = 7
 * */
#define HAMT_KEY_SIZE 32
#define HAMT_BITMAP_SIZE 32
#define HAMT_MAX_SLOTS HAMT_BITMAP_SIZE
#define HAMT_MAX_DEPTH 7

typedef intptr_t Slot;

typedef struct hamt_node {
    uint32_t bitmap;
    Slot slots[0];
} Node;

/* to store the hash collision keys at the bottom of HAMT */
typedef struct hamt_vector {
    int32_t length;
    Item *items[0];
} Vector;

/* Leaf Slots: Vector Slot, Data Slot
 * */
#define SLOT_PTR(t, p) ((t)(void*)((Slot)(p) & ~3))
#define VSLOT(p) ((Slot)(p) | 1)
#define IS_VSLOT(s) ((Slot)(void*)s & 1)
#define IS_DSLOT(s) (!IS_VSLOT(s))
#define EMPTY_SLOT 0xfffffff

/* Hash Array Mapped Trie */

typedef struct hamt {
    struct hamt_node *root;
    struct hamt_ops ops;
} Hamt;

/* Bitmap Utilities */

static void bitmap_dump(int32_t bitmap);

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
size_t
node_calc_size(uint16_t slots_count) {
    size_t power = 2;

    while (power < (sizeof(Node) + slots_count * sizeof(Slot))) {
        power <<= 2;
    }
    return power;
}

uint16_t
node_slots_count(Node *node) {
    return bitcount(node->bitmap);
}

uint16_t
node_max_slots_count(Node *node) {
    size_t slots_count = node_slots_count(node);
    return node_calc_size(slots_count) - sizeof(Node);
}

static Node*
node_new(uint16_t slots_count)
{
    Node *node;
    Node node_zero = {0, };
    int max_slots, i;

    assert(slots_count <= HAMT_MAX_SLOTS);
    node = (Node *)malloc(node_calc_size(slots_count));
    if (! node) {
        HAMT_DEBUG("out of memory");
        return NULL;
    }
    *node = node_zero;

    max_slots = node_max_slots_count(node);
    for (i=0; i<max_slots; i++) {
        node->slots[i] = EMPTY_SLOT;
    }
    return node;
}

int16_t
node_map_slot_n(Node *node, uint8_t index) {
    uint64_t mask;
        
    /* intel processor masks the shift count to 5 bits (maximum 31).*/
    mask = (index == 31) ? \
        0xffffffff: \
        (1 << (index+1)) - 1;

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
node_resize(Node *node, uint16_t slots_count2) {
    uint16_t slots_count = node_slots_count(node);

    if (node_calc_size(slots_count2) == node_calc_size(slots_count)) {
        return node;
    }
    return (Node *)realloc(node, node_calc_size(slots_count2));
}

Node*
node_resize2(Node *node, int8_t diff) {
    uint16_t slots_count;

    slots_count = node_slots_count(node);
    return node_resize(node, slots_count + diff);
}

/* this routine does NOT check slot overwrite, and do REALLOC 
 * the input node. */
static Node*
_node_insert_slot(Node *node, uint16_t index, Slot slot) {
    Slot *slot_p;
    int slot_n, slots_count;
    int i;

    slot_p = node_find_slot(node, index);
    if (slot_p) {
        *slot_p = slot;
        return node;
    }

    node = node_resize2(node, 1);
    slots_count = node_slots_count(node);
    slot_n = node_map_slot_n(node, index);
    assert(slot_n <= HAMT_MAX_SLOTS);
    assert(slots_count <= HAMT_MAX_SLOTS);
    printf("slots_count: %d slot_n: %d sizeof(Slot): %ld\n", slots_count, slot_n, sizeof(Slot));
    for (i=slots_count; i>slot_n; i--) {
        node->slots[i] = node->slots[i-1];
    }
    node->slots[slot_n] = slot;
    node->bitmap |= (1 << index);
    return node;
}

int
node_insert_slot(Node **node, uint16_t index, Slot slot) {
    *node = _node_insert_slot(*node, index, slot);
    if (*node)
        return 0;
    return -1;
}

/* return 0 if Node is empty */
uint16_t
node_delete_slot(Node *node, uint16_t index) {
    unsigned int slots_count, slot_n;

    slots_count = node_slots_count(node);
    if (slots_count == 0) {
        return 0;
    }

    slot_n = node_map_slot_n(node, index);
    memmove(
        (void*)&node->slots[slot_n+1],
        (void*)&node->slots[slot_n], 
        slots_count - slot_n);
    node->bitmap &= ~(1 << index);
    return slots_count - 1;
}

static void
bitmap_dump(int32_t bitmap) {
    int size, i, mask;

    size = HAMT_BITMAP_SIZE;
    for (i=0; i < size; i++) {
        mask = 1 << i;
        putchar(bitmap & mask? '1': '0');
    }
}

static void
node_dump(Node *node) {
    unsigned int slots_count, i = 1, j = 0;

    printf("node ");
    bitmap_dump(node->bitmap);
    printf(" => \n");
    for (i=0; i<HAMT_MAX_SLOTS; i++) {
        if (node->bitmap & (1 << i)) {
            printf(" [%d]: %p ", i, (void*)node->slots[j]);
            j++;
        }
    }
    printf("\n");
}

/* hamt utilities */

Hamt*
hamt_new() {
    Hamt *hamt;
    Hamt hamt_zero = { 0, };
    
    hamt = (Hamt*)malloc(sizeof(Hamt));
    if (! hamt) {
        HAMT_DEBUG("out of memory");
        return NULL;
    }
    *hamt = hamt_zero;
    return hamt;
}

Slot*
hamt_find_slotref(Hamt *hamt, uint32_t hash) {
    return NULL;
}

Slot
hamt_find_slot() {
    return (Slot)NULL;
}

int main(int argc, char *argv[]){
    Node *node1, *node2;
    assert(bitcount(0x01) == 1);
    assert(bitcount(0x02) == 1);
    assert(bitcount(0x03) == 2);
    assert(bitcount(0x05) == 2);
    assert(bitcount(0xff) == 8);
    assert(bitcount(0xffff) == 16);
    assert(powerup(2) == 2);
    assert(powerup(3) == 4);
    node1 = node_new(2);
    node_insert_slot(&node1, 0, (Slot)(void*)0);
    node_dump(node1);
    node_insert_slot(&node1, 1, (Slot)(void*)1);
    node_dump(node1);
    node_insert_slot(&node1, 4, (Slot)(void*)4);
    node_dump(node1);
    node_insert_slot(&node1, 2, (Slot)(void*)2);
    node_dump(node1);
    node_insert_slot(&node1, 3, (Slot)(void*)3);
    node_insert_slot(&node1, 28, (Slot)(void*)28);
    node_insert_slot(&node1, 29, (Slot)(void*)29);
    node_insert_slot(&node1, 30, (Slot)(void*)30);
    node_insert_slot(&node1, 31, (Slot)(void*)31);
    node_insert_slot(&node1, 31, (Slot)(void*)31);
    // node_insert_slot(&node1, 100, (Slot)(void*)1); // assert error

    return 0;
}
