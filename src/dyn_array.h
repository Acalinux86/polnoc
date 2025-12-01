#ifndef DYN_ARRAY_H_
#define DYN_ARRAY_H_

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#define DYN_ARRAY_INITIAL_CAPACITY 256

// NOTE: New Dynamic Array Initialization
#define dyn_array_new(dyn_array)                                        \
    do {                                                                \
        (dyn_array)->count = 0;                                         \
        (dyn_array)->capacity = DYN_ARRAY_INITIAL_CAPACITY;             \
        (dyn_array)->contents = malloc((dyn_array)->capacity);          \
        assert((dyn_array)->contents != NULL &&                         \
               "Memory Allocation For Dynamic Array Contents Failed");  \
    } while (0)

// NOTE: Remove An Element of Specified Index and Shift the Dynamic Array
#define dyn_array_delete_content(dyn_array, index)                  \
    do {                                                            \
        /* Assert Dynamic Array count is greater than zero */       \
        assert(index < (dyn_array)->count);                         \
        /* Assert pointer to contents is not NULL */                \
        assert((dyn_array)->contents != NULL);                      \
        /* Shift the Dynamic Array elements if deleting is occur-
           ing at aspecific index */                                \
        if (index < (dyn_array)->count - 1) {                           \
            for (size_t i = index; i < (dyn_array)->count - 1; i++) {   \
                (dyn_array)->contents[i] = (dyn_array)->contents[i + 1];    \
            }                                                       \
        }                                                           \
        /* Decrease Dynamic Dyn_Array Count */                          \
        (dyn_array)->count--;                                           \
    } while (0)

// NOTE: Remove Last Element From Dynamic Array
#define dyn_array_pop(dyn_array)                                        \
    do {                                                                \
        if ((dyn_array)->count == 0) {                                  \
            fprintf(stderr,                                             \
                    "Warning: Attempting to Pop From An empty Dynamic Array.\n"); \
            abort();                                                    \
        } else {                                                        \
            dyn_array_delete_content((dyn_array), (dyn_array)->count - 1); \
        }                                                               \
    } while (0)

// NOTE: Resizes the Dynamic Array, making the new capacity
// double of the old one
#define dyn_array_resize(dyn_array)                                     \
    do {                                                                \
        if ((dyn_array)->count >= (dyn_array)->capacity) {              \
            (dyn_array)->capacity =                                     \
                (dyn_array)->capacity == 0 ?                            \
                DYN_ARRAY_INITIAL_CAPACITY :                            \
                ((dyn_array)->capacity * 2 + 1);                        \
            /* Reallocate the array contents to the new capacity*/      \
            (dyn_array)->contents =                                     \
                realloc(                                                \
                    (dyn_array)->contents,                              \
                    sizeof((dyn_array)->contents)*((dyn_array)->capacity)); \
            /* Check if The Reallocation pointer is not NULL */         \
            if ((dyn_array)->contents == NULL ) {                       \
                fprintf(stderr, "ERROR: Memory Reallocation For Dynamic Array Failed."); \
                abort(); /* abort*/                                     \
            }                                                           \
        }                                                               \
    } while (0)

// NOTE: Push An Element To A Specified Index in An Dynamic Array
#define dyn_array_push(dyn_array, content, index)                       \
    do {                                                                \
        /* Check if the array needs resizing*/                          \
        dyn_array_resize(dyn_array);                                    \
        /* Shift the elements if insert is happening at a specific
           index */                                                     \
        if (index != (dyn_array)->count) {                              \
            for (size_t i = (dyn_array)->count; i > index; --i) {       \
                (dyn_array)->contents[i] = (dyn_array)->contents[i - 1]; \
            }                                                           \
        }                                                               \
        /* Increase Dynamic Array count and append the content  */      \
        (dyn_array)->contents[index] = content;                         \
        (dyn_array)->count++;                                           \
    } while (0)

// NOTE: Append An Content in the end
#define dyn_array_append(dyn_array, content) \
    dyn_array_push((dyn_array), content, (dyn_array)->count)

// NOTE: Clear An Dynamic Array
#define dyn_array_clear(dyn_array) ((dyn_array)->count = 0)

// NOTE: Destroy the Dynamic Array
#define dyn_array_delete(dyn_array)             \
    do {                                        \
        free((dyn_array)->contents);            \
        (dyn_array)->count = 0;                 \
        (dyn_array)->capacity = 0;              \
        (dyn_array)->contents = NULL;           \
    } while(0)


#define dyn_array_fill(dyn_array, count, value) \
    do {                                        \
        for (size_t i = 0; i < count; ++i) {    \
            dyn_array_append(dyn_array, value); \
        }                                       \
    } while (0)

#endif // DYN_ARRAY_H_
