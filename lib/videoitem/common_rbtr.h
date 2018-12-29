#ifndef RBT_H
#define RBT_H

//#include <xmvideoitem.h>


typedef enum {
    RBT_STATUS_OK,
    RBT_STATUS_MEM_EXHAUSTED,
    RBT_STATUS_DUPLICATE_KEY,
    RBT_STATUS_KEY_NOT_FOUND
} RbtStatus;

typedef void *RbtIterator;
typedef void *RbtHandle;

typedef enum { BLACK, RED } NodeColor;

typedef struct NodeTag {
    struct NodeTag *left;       // left child
    struct NodeTag *right;      // right child
    struct NodeTag *parent;     // parent
    NodeColor color;            // node color (BLACK, RED)

    void *key;                  // key used for searching
    void *val;                // user data
} NodeType;

typedef struct RbtTag {
    NodeType *root;   // root of red-black tree
    NodeType sentinel;
    int (*compare)(void *a, void *b);    // compare keys
} RbtType;

// all leafs are sentinels
#define SENTINEL &rbt->sentinel

void * rbtMalloc (int size);
void   rbtFree (void *p);

RbtHandle rbtNew(int(*compare)(void *a, void *b));
// create red-black tree
// parameters:
//     compare  pointer to function that compares keys
//              return 0   if a == b
//              return < 0 if a < b
//              return > 0 if a > b
// returns:
//     handle   use handle in calls to rbt functions


void rbtDelete(RbtHandle h);
// destroy red-black tree

RbtStatus rbtInsert(RbtHandle h, void *key, void *value);
// insert key/value pair

RbtStatus rbtErase(RbtHandle h, RbtIterator i);
// delete node in tree associated with iterator
// this function does not free the key/value pointers

RbtIterator rbtNext(RbtHandle h, RbtIterator i);
// return ++i

RbtIterator rbtBegin(RbtHandle h);
// return pointer to first node

RbtIterator rbtEnd(RbtHandle h);
// return pointer to one past last node

void rbtKeyValue(RbtHandle h, RbtIterator i, void **key, void **value);
// returns key/value pair associated with iterator

RbtIterator rbtFind(RbtHandle h, void *key);
// returns iterator associated with key

void   rbtr_int (void);


#endif
