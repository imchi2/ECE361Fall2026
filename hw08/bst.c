/*
 * bst.c - a binary search tree of int keys, ported from the
 * TutorialsPoint BinarySearchTree.c into a small ADT.
 *
 * Both structs are complete here and nowhere else: bst.h only says
 * "typedef struct bst bst_t", so callers can hold a pointer but cannot
 * look inside. Nothing is global: every function gets its tree passed
 * in, so any number of trees can exist side by side.
 *
 * Which function each one came from, as the spec asks:
 *   bst_create   - new; the original's tree simply existed as a global
 *   bst_destroy  - new; the original never freed anything
 *   bst_insert   - the original's insert(), plus error returns and a
 *                  size count
 *   bst_contains - the original's search(), without the printing
 *   bst_size     - new; the original had no way to ask
 *   bst_height   - new; the original had no way to ask
 *   bst_remove   - new (bonus); the original could not remove
 */
#include <stdlib.h>

#include "bst.h"

/* One node of the tree. The field names are kept from the original
 * program: data, leftChild, rightChild. A missing child is NULL. */
struct node {
    int data;
    struct node *leftChild;    /* keys smaller than data */
    struct node *rightChild;   /* keys greater than or equal to data */
};

/* The tree itself: where the root is, plus the number of keys, so
 * bst_size does not have to walk the tree. "root" was the original's
 * global variable; here it is one per tree. */
struct bst {
    struct node *root;
    size_t size;
};

/*
 * bst_create - make a new, empty tree (see bst.h).
 *
 * One allocation, checked before anything is stored in it. A new tree
 * has no root yet and holds no keys.
 */
bst_t *bst_create(void)
{
    bst_t *t;

    t = malloc(sizeof *t);
    if (t == NULL) {
        /* Memory ran out: return NULL and leak nothing. */
        return NULL;
    }

    t->root = NULL;
    t->size = 0;

    return t;
}

/*
 * free_nodes - free a node and everything under it.
 *
 * Recursion is used on purpose: a tree is a recursive shape (every
 * child is itself a tree), so "free the children, then the node"
 * reads exactly like the shape. The children are freed FIRST, because
 * after free(n) the pointers to them would be gone.
 */
static void free_nodes(struct node *n)
{
    if (n == NULL) {
        return;
    }

    free_nodes(n->leftChild);
    free_nodes(n->rightChild);
    free(n);
}

/*
 * bst_destroy - free every node and the tree (see bst.h).
 *
 * NULL does nothing, exactly as the interface promises; otherwise the
 * nodes go first and the struct that pointed at them goes last.
 */
void bst_destroy(bst_t *t)
{
    if (t == NULL) {
        return;
    }

    free_nodes(t->root);
    free(t);
}

/*
 * bst_insert - add key to the tree (see bst.h).
 *
 * This is the original's insert() with three changes: the tree is
 * passed in instead of read from a global, malloc is checked, and the
 * size is counted. The walk is the original's, unchanged: smaller
 * keys go left, all others go right, and the new node is attached
 * where the walk falls off the tree. All checks run BEFORE anything
 * is changed, so a failure leaves the tree exactly as it was.
 */
int bst_insert(bst_t *t, int key)
{
    struct node *tempNode;
    struct node *current;
    struct node *parent;

    if (t == NULL) {
        return -1;
    }

    /* The new node is built first; a new node is always a leaf. */
    tempNode = malloc(sizeof *tempNode);
    if (tempNode == NULL) {
        /* Memory ran out: the tree is unchanged. */
        return -1;
    }
    tempNode->data = key;
    tempNode->leftChild = NULL;
    tempNode->rightChild = NULL;

    /* Empty tree: the new node becomes the root. */
    if (t->root == NULL) {
        t->root = tempNode;
        t->size = t->size + 1;
        return 0;
    }

    /* Walk down: current is where we are, parent stays one step
     * behind, so parent still knows where to attach the new node when
     * current falls off the bottom of the tree. */
    current = t->root;
    parent = NULL;
    while (1) {
        parent = current;

        if (key < parent->data) {
            /* Smaller: the gap must be somewhere to the left. */
            current = current->leftChild;
            if (current == NULL) {
                parent->leftChild = tempNode;
                t->size = t->size + 1;
                return 0;
            }
        } else {
            /* Greater or EQUAL: to the right. This is why a duplicate
             * key is kept, one step to the right of the key it
             * equals. */
            current = current->rightChild;
            if (current == NULL) {
                parent->rightChild = tempNode;
                t->size = t->size + 1;
                return 0;
            }
        }
    }
}

/*
 * bst_contains - whether key is in the tree (see bst.h).
 *
 * This is the original's search(), minus the printing and plus the
 * visit count. Part 2 fix: the faithful port kept the original's loop
 * condition, while (current->data != key), which reads current->data
 * BEFORE any NULL check -- so a search on an empty tree dereferenced
 * NULL and crashed (the report is in README.md). Testing
 * current != NULL first keeps the same walk but never touches a node
 * that is not there.
 */
bool bst_contains(const bst_t *t, int key, size_t *visits)
{
    struct node *current;
    size_t n;

    if (t == NULL) {
        if (visits != NULL) {
            *visits = 0;
        }
        return false;
    }

    n = 0;
    current = t->root;

    /* The original's walk: compare the node, then go left when the
     * key is smaller, right when it is greater. The NULL test comes
     * FIRST, and && short-circuits, so current->data is only read
     * when current is a real node. */
    while (current != NULL && current->data != key) {
        n = n + 1;
        if (current->data > key) {
            /* The key is smaller: continue to the left. */
            current = current->leftChild;
        } else {
            /* The key is greater: continue to the right. */
            current = current->rightChild;
        }
    }

    if (current != NULL) {
        /* Found: the loop condition compared this node as well. */
        n = n + 1;
    }
    if (visits != NULL) {
        *visits = n;
    }
    return (current != NULL);
}

/*
 * bst_size - how many keys are held (see bst.h).
 *
 * The count is kept up to date by insert and remove, so this is a
 * lookup, not a walk.
 */
size_t bst_size(const bst_t *t)
{
    if (t == NULL) {
        return 0;
    }

    return t->size;
}

/*
 * height_of - nodes on the longest path from n down; 0 for NULL.
 *
 * Recursive on purpose, for the same reason as free_nodes: the height
 * of a tree IS one more than the taller of its two subtrees, so the
 * code reads like the definition.
 */
static size_t height_of(const struct node *n)
{
    size_t left;
    size_t right;

    if (n == NULL) {
        return 0;
    }

    left = height_of(n->leftChild);
    right = height_of(n->rightChild);

    if (left > right) {
        return left + 1;
    }
    return right + 1;
}

/*
 * bst_height - nodes on the longest root-to-leaf path (see bst.h).
 *
 * The NULL check comes first, because ->root would crash on NULL.
 */
size_t bst_height(const bst_t *t)
{
    if (t == NULL) {
        return 0;
    }

    return height_of(t->root);
}

/*
 * bst_remove - remove one copy of key (see bst.h; bonus part).
 *
 * The walk down is the same one bst_contains uses, with parent kept
 * one step behind so the found node can be unlinked. Three cases:
 *   1. no children (a leaf): detach it and free it;
 *   2. one child: link the parent straight to that child;
 *   3. two children: copy the smallest key of the RIGHT subtree up
 *      into the node, and remove THAT key's node instead -- it never
 *      has a left child, so it is case 1 or 2 again.
 * Either way exactly one node is freed, and the size drops by one.
 */
int bst_remove(bst_t *t, int key)
{
    struct node *current;
    struct node *parent;

    if (t == NULL) {
        return -1;
    }

    /* Find the node, parent staying one step behind (NULL at root). */
    current = t->root;
    parent = NULL;
    while (current != NULL && current->data != key) {
        parent = current;
        if (key < current->data) {
            current = current->leftChild;
        } else {
            current = current->rightChild;
        }
    }
    if (current == NULL) {
        /* The key is not in the tree: the tree stays unchanged. */
        return -1;
    }

    if (current->leftChild != NULL && current->rightChild != NULL) {
        /* Case 3: two children. The smallest key of the right subtree
         * sits at the end of a walk that only goes left. */
        struct node *smallest;
        struct node *smallestParent;

        smallest = current->rightChild;
        smallestParent = current;
        while (smallest->leftChild != NULL) {
            smallestParent = smallest;
            smallest = smallest->leftChild;
        }

        /* Take the key; THAT node is removed instead of current. */
        current->data = smallest->data;

        /* "smallest" has no left child (the walk would have gone
         * on), so it has at most a right child: link its parent
         * straight to it. Which side of the parent it hangs on is
         * found by comparing pointers -- if the walk never moved,
         * smallest is the RIGHT child of current. */
        if (smallestParent->leftChild == smallest) {
            smallestParent->leftChild = smallest->rightChild;
        } else {
            smallestParent->rightChild = smallest->rightChild;
        }
        free(smallest);
    } else {
        /* Cases 1 and 2: at most one child. child is that one child,
         * or NULL when current is a leaf; either way, linking the
         * parent to child removes current from the tree. */
        struct node *child;

        if (current->leftChild != NULL) {
            child = current->leftChild;
        } else {
            child = current->rightChild;
        }

        if (parent == NULL) {
            /* The root was removed: the child is the new root. */
            t->root = child;
        } else if (parent->leftChild == current) {
            parent->leftChild = child;
        } else {
            parent->rightChild = child;
        }
        free(current);
    }

    t->size = t->size - 1;
    return 0;
}
