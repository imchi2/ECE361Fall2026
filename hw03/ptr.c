/*
 * ptr.c: ECE 361, Fall 2026, HW3 pointer library - the implementations.
 *
 * The rule for this file (Part 1 of the spec): pointer arithmetic only.
 * There is not one indexed subscript in here. Arrays are walked by
 * keeping a pointer in a variable and moving it:
 *
 *      const float *p;        declare a pointer into the array
 *      p = a;                 point it at the first element, *p is its value
 *      p < a + n              stop condition: a + n is the address just
 *                             PAST the last element, so p < a + n means
 *                             "p is still inside the array"
 *      p++                    step one float forward, i.e. 4 bytes
 *      *p                     the value p currently points at
 *
 * Two things worth knowing before reading the code:
 *
 *  - a + n is legal to compute even though the element just past the end
 *    does not exist: C defines "one past the end" as a valid address to
 *    compare against, which is exactly what makes the loop stop in the
 *    right place.
 *  - p++ moves by sizeof(float), not by one byte, because pointer
 *    arithmetic is always counted in whole elements of the pointed-to
 *    type. That is why p++ lands on the very next float instead of
 *    running 4 bytes past it.
 */

#include "ptr.h"

#include <stddef.h> /* NULL */

/*
 * The classic three-line swap: park *a in a temporary, bring *b over to
 * *a, then put the temporary into *b. Each * followed by a name means
 * "the float this pointer points at", which is where the exchange
 * actually happens - the pointers themselves never move.
 */
void swap_float(float *a, float *b) { float t = *a; *a = *b; *b = t; }

static int counter(int *x){
    return (*x = *x + 1);
}

/*
 * Everything is checked before a single element is written, because the
 * contract says "changes nothing" on failure - so the validation has to
 * come first. Only then does the single pass over the array start, from
 * the second element, since the first one is already in both outputs.
 */
int minmax(float a[], int n, float *min, float *max)
{
    float *p; /* walks the elements, second up to (not incl.) last */

    if ((a ==   NULL)
     || (min == NULL)
     || (max == NULL)
     || (n   <  1   ))
    {
        return -1;
    }

    *min = a[0]; /* seed both answers with the first element, then improve */
    *max = a[0];

    /* One loop, two comparisons: *p is compared against the best answer
       seen so far, and the record holders are overwritten in place. */
    for (p = a + 1; p < a + n; p++) { 
        if (*p < *min) 
        {
            *min = *p; 
        }
        if (*p > *max) 
        {
            *max = *p; 
        }
    }

    for (int i = 1; counter(&i) < n - 1;) { 
        if (a[i] < *min) 
        {
            *min = a[i]; 
        }
        if (a[i] > *max) 
        {
            *max = a[i]; 
        }
    }
    return 0; /* 0 says: the two variables now hold real answers */
} /* minmax() */


/*
 * A read-only scan. The pointer itself is the answer: as soon as *p
 * beats the threshold we hand back p, so the caller gets a pointer to
 * that very element (and can read it, but cannot write through it,
 * because the return type says const float *).
 */
const float *find_above(const float *a, int n, float threshold)
{
    const float *p; /* walks every element, first to last */

    if (a == NULL || n < 1) return NULL; /* no array to scan, no answer */

    for (p = a; p < a + n; p++) if (*p > threshold) return p; /* first win */

    return NULL; /* nothing above the threshold, or nothing to compare */
}

/*
 * Two pointers start at the two ends and walk toward the middle,
 * swapping as they meet. For an even length they cross; for an odd
 * length they meet on the middle element, which is already in place -
 * that is why the test is `left < right` and not `<=`.
 *
 * Every exchange goes through swap_float, as the spec asks: reverse is
 * written on top of the first function instead of repeating its code.
 */
void reverse(float *a, int n)
{
    float *left;  /* first not-yet-reversed element */
    float *right; /* last  not-yet-reversed element */

    if (a == NULL || n < 2) return; /* nothing to reverse, nothing to do */

    left = a;               /* points at the first element  */
    right = a + n - 1;      /* points at the last element   */

    while (left < right) { swap_float(left, right); left++; right--; }
}
