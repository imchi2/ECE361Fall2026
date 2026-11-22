# hw08 bonus — bst_remove

`int bst_remove(bst_t *t, int key)` removes one copy of `key` — the
first one a search reaches — and frees exactly one node. It returns 0
on success, or -1 if `t` is NULL or `key` is not in the tree; in those
cases the tree is unchanged. The walk down is the same one
`bst_contains` uses, with `parent` kept one step behind so the found
node can be unlinked.

Three cases:

1. **A leaf** — detach it from its parent and free it.
2. **One child** — link the parent straight to that child, so the
   child moves up into the removed node's place.
3. **Two children** — copy the smallest key of the right subtree up
   into the node, and remove THAT key's node instead. It never has a
   left child (the walk to it only goes left), so it is case 1 or 2
   again.

The three removals from the original's seven-key tree, in the order
`tests/test_remove.c` performs them:

## After removing 10 — a leaf

```
        27
       /  \
     14    35
      \    / \
      19  31  42
```

## After removing 14 — one child (19 moves up)

```
        27
       /  \
     19    35
          /  \
        31   42
```

## After removing 27 — two children (31 moves up)

31 is the smallest key of the right subtree (35 → 31), so it takes
27's place; 31's old node is removed instead.

```
        31
       /  \
     19    35
            \
            42
```

The tests in `tests/test_remove.c` check that after this last removal
31 is found in exactly one visit — proof that it really is the root.
`make bonus` builds and runs them with `-fsanitize=address,undefined`,
so "frees exactly one node" is checked by the sanitizers: a leak or a
double free would fail the run.
