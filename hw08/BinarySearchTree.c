/* ======================================================================
 * BinarySearchTree.c  --  the original TutorialsPoint binary search tree
 * ------------------------------------------------------------------------
 * Annotated line by line for HW8. This program is the STARTING POINT:
 * HW8 ports it into a proper ADT (bst.h / bst.c with no globals).
 *
 * What the program does:
 *   1. Builds ONE binary search tree from 7 hard-coded keys.
 *   2. Searches for 31 (found) and for 15 (not found).
 *
 * Binary search tree rule used everywhere below:
 *   keys SMALLER than a node's key live in its LEFT subtree,
 *   all other keys (including EQUAL ones) live in its RIGHT subtree.
 *
 * Known weaknesses (the HW8 spec calls these out on purpose):
 *   - the tree is one GLOBAL variable: only one tree can ever exist;
 *   - search() PRINTS while searching (an ADT should do no I/O);
 *   - nothing is ever free()d: all 7 nodes leak;
 *   - search() CRASHES on an empty tree: it reads current->data
 *     before ever checking current != NULL (see search() below);
 *   - malloc() is never checked: out-of-memory would crash too.
 * ======================================================================
 */

#include <stdio.h>   /* printf */
#include <stdlib.h>  /* malloc */

/* One node of the tree: a key plus pointers to the two children.
 * "struct node *" inside the struct's own definition is legal because
 * the compiler only needs the SIZE of a pointer here, not the full
 * definition of struct node. A missing child is stored as NULL. */
struct node {
   int data;                  /* the key stored in this node */

   struct node *leftChild;    /* subtree: keys SMALLER than data */
   struct node *rightChild;   /* subtree: keys >= data (duplicates go right) */
};

/* The one and only tree, as a global pointer to its root node.
 * NULL means "the tree is empty". Being global, it is shared by
 * insert() and search() -- exactly what the ADT version must remove. */
struct node *root = NULL;

/* ----------------------------------------------------------------------
 * insert(data): add a new node holding `data` to the tree.
 *
 * Works in two phases:
 *   1. allocate and fill the new node (its children start as NULL,
 *      because a brand-new node is always a leaf);
 *   2. walk down from the root to find the empty spot where the
 *      new node belongs, then attach it there.
 *
 * Note: the return value of malloc is never checked, and nothing
 * reports failure -- the ADT version returns -1 on problems instead.
 * -------------------------------------------------------------------- */
void insert(int data) {
   /* Phase 1: build the new node.
    * (The cast on malloc is not needed in C, but it is harmless.) */
   struct node *tempNode = (struct node*) malloc(sizeof(struct node));
   struct node *current;   /* walks down the tree looking for a gap */
   struct node *parent;    /* stays ONE STEP BEHIND current, so we know
                              where to attach the new node at the end   */

   tempNode->data = data;
   tempNode->leftChild = NULL;
   tempNode->rightChild = NULL;

   //if tree is empty
   if(root == NULL) {
      /* No root yet: the new node simply BECOMES the root. Done. */
      root = tempNode;
   } else {
      /* Phase 2: walk down. current = where we are,
       * parent = the node we came from (needed to attach the leaf). */
      current = root;
      parent = NULL;

      while(1) {            /* loops until a return below fires */
         parent = current;  /* remember where we came from */

         //go to left of the tree
         if(data < parent->data) {
            /* new key is smaller: the gap must be on the left */
            current = current->leftChild;

            //insert to the left
            if(current == NULL) {
               /* parent has no left child: attach the new node there */
               parent->leftChild = tempNode;
               return;
            }
            /* otherwise keep walking down the left subtree */
         }  //go to right of the tree
         else {
            /* new key is >= parent's key: gap must be on the right.
             * This is why DUPLICATE keys end up to the RIGHT. */
            current = current->rightChild;

            //insert to the right
            if(current == NULL) {
               /* parent has no right child: attach the new node there */
               parent->rightChild = tempNode;
               return;
            }
            /* otherwise keep walking down the right subtree */
         }
      }
   }
}

/* ----------------------------------------------------------------------
 * search(data): find the node holding `data`, or NULL if absent.
 *
 * Walks down from the root exactly like insert does: at each node,
 * go left if the wanted key is smaller, right if it is bigger.
 * Also PRINTS every node it visits (the "Visiting elements:" line) --
 * a side effect the ADT version must drop.
 *
 * BUG: the while condition reads current->data BEFORE any NULL check.
 * If the tree is EMPTY (root == NULL), the very first test
 * dereferences NULL and the program crashes. Inside the loop the
 * code does check for NULL (after moving), so only the empty-tree
 * case is broken. main() never triggers it because it inserts first.
 * -------------------------------------------------------------------- */
struct node* search(int data) {
   struct node *current = root;      /* start at the top of the tree */
   printf("Visiting elements: ");

   while(current->data != data) {    /* CRASHES here if root is NULL */
      if(current != NULL)            /* too late: the while condition */
         printf("%d ",current->data);/* already dereferenced current  */

      // data @ current node > data we are looking for
      // so we need to traverse the path where data < data current node
      //go to left tree
      if(current->data > data) {
         /* wanted key is smaller than this node's key: go left */
         current = current->leftChild;
      }
      //else go to right tree
      else {
         /* wanted key is bigger: go right */
         current = current->rightChild;
      }

      //not found
      if(current == NULL) {
         /* fell off the bottom of the tree: key is not present.
          * This NULL check (after moving) is what protects the NEXT
          * evaluation of the while condition -- the empty-tree case
          * is the only one that slips through, at the very first test. */
         return NULL;
      }
   }

   /* loop ended because current->data == data: found it */
   return current;
}



int main() {
   int i;
   int array[7] = { 27, 14, 35, 10, 19, 31, 42 };

   /* Insert the 7 keys in array order. The ORDER matters: it decides
    * the shape of the tree (HW8 Part 3 is all about this).
    * This builds:
    *
    *              27
    *             /  \
    *           14    35
    *          /  \   / \
    *        10   19 31  42
    */
   for(i = 0; i < 7; i++)
      insert(array[i]);

   /* Search 1: 31 IS in the tree.
    * Path: 27 (31 > 27, go right) -> 35 (31 < 35, go left) -> 31 found.
    * Prints "Visiting elements: 27 35 " then the found message. */
   i = 31;
   struct node * temp = search(i);

   if(temp != NULL) {
      printf("[%d] Element found.", temp->data);
      printf("\n");
   }else {
      printf("[ x ] Element not found (%d).\n", i);
   }

   /* Search 2: 15 is NOT in the tree.
    * Path: 27 (go left) -> 14 (go right) -> 19 (go left) -> NULL.
    * search() returns NULL, so the "not found" branch prints. */
   i = 15;
   temp = search(i);

   if(temp != NULL) {
      printf("[%d] Element found.", temp->data);
      printf("\n");
   }else {
      printf("[ x ] Element not found (%d).\n", i);
   }

   /* NOTE: all 7 nodes allocated by insert are never free()d.
    * The OS reclaims the memory when the program exits, so this
    * demo "works", but a library must clean up after itself:
    * that is what bst_destroy() is for in the ADT version. */
   return 0;
}

/*
If we compile and run the above program, it will produce the following result −

Output
Visiting elements: 27 35 [31] Element found.
Visiting elements: 27 14 19 [ x ] Element not found (15).

*/
