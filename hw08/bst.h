/*
 * bst.h - a binary search tree of int keys.
 *
 * The tree is opaque: only bst.c knows what is inside struct bst and
 * the nodes. Callers keep a bst_t pointer and pass it to every
 * function, so any number of trees can exist at once. (The original
 * TutorialsPoint program this is ported from had ONE global tree.)
 *
 * Ordering rule, kept from the original: a key smaller than a node's
 * key goes to the LEFT subtree; every other key, including an equal
 * one, goes to the RIGHT. Duplicate keys are therefore kept, to the
 * right of the key they equal.
 */
#ifndef BST_H
#define BST_H

#include <stdbool.h>
#include <stddef.h>

typedef struct bst bst_t;          /* opaque */

/*
 * bst_create - make a new, empty tree.
 * Takes: nothing.
 * Returns: an empty tree, or NULL if memory ran out.
 * Assumes: nothing; any caller may call it.
 */
bst_t  *bst_create(void);

/*
 * bst_destroy - free every node and the tree itself.
 * Takes: t, the tree to free, or NULL.
 * Returns: nothing.
 * Assumes: t was made by bst_create and is not used again afterwards.
 *          NULL does nothing.
 */
void    bst_destroy(bst_t *t);

/*
 * bst_insert - add one key to the tree.
 * Takes: t, the tree; key, the key to add.
 * Returns: 0 on success, or -1 if t is NULL or memory ran out; in
 *          those cases the tree is unchanged.
 * Assumes: nothing; an equal key is kept, inserted to the right,
 *          exactly like the original program did.
 */
int     bst_insert(bst_t *t, int key);

/*
 * bst_contains - whether a key is in the tree, and how long the
 *                search was.
 * Takes: t, the tree (may be NULL); key, the key to look for; visits,
 *        where to store the number of nodes compared during the
 *        search, found or not (visits may be NULL if the count is
 *        not wanted).
 * Returns: true if key is in the tree, false otherwise; false for a
 *          NULL tree.
 * Assumes: if visits is not NULL it points at a size_t the function
 *          may write; for a NULL tree, *visits is set to 0.
 */
bool    bst_contains(const bst_t *t, int key, size_t *visits);

/*
 * bst_size - how many keys the tree holds right now.
 * Takes: t, the tree (may be NULL).
 * Returns: the number of keys; 0 for NULL.
 * Assumes: nothing.
 */
size_t  bst_size(const bst_t *t);

/*
 * bst_height - the number of nodes on the longest path from the root
 *              down.
 * Takes: t, the tree (may be NULL).
 * Returns: the height; an empty tree has height 0, a tree with one
 *          node has height 1; 0 for NULL.
 * Assumes: nothing.
 */
size_t  bst_height(const bst_t *t);

/*
 * bst_remove - remove one copy of a key (bonus part).
 * Takes: t, the tree; key, the key to remove.
 * Returns: 0 on success, or -1 if t is NULL or key is not in the
 *          tree; in those cases the tree is unchanged.
 * Assumes: the copy removed is the first one a search reaches. A node
 *          with two children takes the smallest key of its right
 *          subtree, and THAT key's node is removed instead, so exactly
 *          one node is freed either way.
 */
int     bst_remove(bst_t *t, int key);

#endif /* BST_H */
