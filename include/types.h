/* Scalar types and rectangles used everywhere. */
typedef unsigned char byte;
typedef signed char sbyte;
typedef unsigned int word;
typedef void far **Handle;               /* a movable block of memory */

typedef struct {
    int top, left, bottom, right;
} Rect;

typedef struct {
    int y, x;
} POINT;
