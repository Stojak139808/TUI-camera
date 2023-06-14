#ifndef FIXED_POINT_H
#define FIXED_POINT_H

/* function-like macros for fixed point arithmetic, needed for some graphics
 * calculations
 */

#define SHIFT_COUNT 16u
#define FRACTION_MASK  ((1 << SHIFT_COUNT) - 1)

/* int to fixed point conversion */
#define INT_TO_FIXED(x) ((x << SHIFT_COUNT) & (int)(~0u))

/* fixed to int point conversion */
#define FIXED_TO_INT(x) ((x >> SHIFT_COUNT) & FRACTION_MASK)

/* fixed point floor */
#define FLOOR(x)        (x & (FRACTION_MASK << SHIFT_COUNT))

/* fixed point ceil */
#define CEIL(x)         ((x | FRACTION_MASK) + 1)

/* fixed point multiplication */
#define MULTIPLY_FIXED(a, b) (((long int)a * b) >> SHIFT_COUNT)

#endif