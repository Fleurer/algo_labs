#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

/* 
 * bst.c 
 * 
 * A quick & slow binary search tree implementation.
 *
 * */

typedef int32_t dst_key_t;
typedef int32_t dst_val_t;

#define NBIT 32
#define BIT_MSD (1 << (NBIT - 1))

struct dst_node {
    dst_key_t key;
    dst_val_t val;
    struct dst_node *left;
    struct dst_node *right;
};

struct dst_node* dst_node_new(dst_key_t key, dst_val_t val) 
{
    struct dst_node *np;

    np = (struct dst_node*)malloc(sizeof(struct dst_node));
    np->left = NULL;
    np->right = NULL;
    np->key = key;
    np->val = val;
    return np;
}

static struct dst_node** __dst_find_r(struct dst_node **npp, dst_key_t key, dst_key_t bit, unsigned eor) 
{
    struct dst_node **nextpp;

    if (*npp == NULL || eor) 
        return npp;
    if (key == (*npp)->key)
        return npp;
    if (key & bit) 
        nextpp = &(*npp)->right;
    else
        nextpp = &(*npp)->left;
    return __dst_find_r(nextpp, key, bit >> 1, bit == 0);
}

struct dst_node* dst_find(struct dst_node *np, dst_key_t key) 
{
    return *__dst_find_r(&np, key, BIT_MSD, 0);
}

struct dst_node* dst_insert(struct dst_node *np, dst_key_t key, dst_val_t val) 
{
    struct dst_node **npp;
    struct dst_node *new_np;

    npp = __dst_find_r(&np, key, BIT_MSD, 0);
    if (*npp == NULL) {
        new_np = dst_node_new(key, val);
        *npp = new_np;
    }
    else {
        (*npp)->val = val;
    }
    return *npp;
}

static void __test() 
{
    struct dst_node *dst, *np;

    dst = dst_node_new(1, 1);
    np = dst_insert(dst, 1, 2);
    assert(np->val == 2);
    np = dst_find(dst, 1);
    assert(np->val == 2);
    dst_insert(dst, 1, 0x3);
    dst_insert(dst, 2, 0xff);
    np = dst_find(dst, 2);
    assert(np->val == 0xff);
    np = dst_find(dst, 1);
    assert(np->val == 3);
}

int main(int argc, char *argv[]) 
{
    __test();
    return 0;
}
