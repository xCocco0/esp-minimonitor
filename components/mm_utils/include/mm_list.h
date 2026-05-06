/**
 * @file mm_list.h
 * @mainpage Circular Doubly Linked List
 *
 * @brief
 * This is a lightweight, circular doubly linked list implementation with basic
 * functions
 *
 *******************************************************************************
 *
 * # List of functions
 *
 * - @ref MM_LIST_DECLARE
 * - @ref MM_LIST_DEFINE
 * - @ref MM_LIST_BEGIN
 * - @ref MM_LIST_NEXT
 * - @ref MM_LIST_END
 * - @ref MM_LIST_BEGIN
 * - @ref MM_LIST_NEXT
 * - @ref MM_LIST_END
 * - @ref MM_LIST_FOREACH
 * - @ref MM_LIST_FOREACH_REVERSE
 * - @ref MM_LIST_FOREACH_READ
 * - @ref MM_LIST_FOREACH_REVERSE_READ
 * - @ref MM_LIST_ITEM
 * - @ref MM_LIST_LEN
 * - @ref MM_LIST_ADD_BACK
 * - @ref MM_LIST_ADD_FRONT
 * - @ref MM_LIST_INSERT_BEFORE
 * - @ref MM_LIST_INSERT_AFTER
 * - @ref MM_LIST_REMOVE
 *
 *******************************************************************************
 *
 * # How to Use
 *
 * ## Define your structure
 *
 * Embed a `mm_list_node_t` member inside your struct:
 *
 * ```c
 * struct my_data {
 *     int value;
 *     mm_list_node_t node;
 * };
 * ```
 *
 * ## Declare and Initialize the root of the list
 *
 * ```
 * MM_LIST_DEFINE(my_mm_list);
 * ```
 *
 * If you need separated definition and declaration, you can use `MM_LIST_DECLARE`:
 *
 * ```
 * // In file.h
 * MM_LIST_DECLARE(my_mm_list);
 *
 * // In file.c
 * MM_LIST_DEFINE(my_mm_list);
 * ```
 *
 * ## Adding Elements
 *
 * Pass the list root and the `mm_list_node_t` member of the new node:
 *
 * ```
 * struct my_data d1 = { .value = 42 };
 * MM_LIST_ADD_BACK(my_mm_list, d1.node);
 * ```
 *
 * ## Iterating and Accessing Data
 *
 * Use `MM_LIST_FOREACH` to loop, and `MM_LIST_ITEM` to recover the parent struct:
 *
 * ```
 * MM_LIST_FOREACH(my_mm_list, it) {
 *     struct my_data *obj = MM_LIST_ITEM(it, struct my_data, node);
 *     printf("%d\n", obj->value);
 *
 *     if (obj->value == 42) {
 *         MM_LIST_REMOVE(my_mm_list, it); // Safe to delete in-place
 *     }
 * }
 * ```
 *
 * For other uses, you can use an iterator with `MM_LIST_BEGIN/NEXT/END` calls.
 *
 * ```
 * mm_list_iterator_t it1 = MM_LIST_BEGIN(my_mm_list);
 * mm_list_iterator_t it2 = MM_LIST_RBEGIN(my_mm_list);
 * while(it1 != MM_LIST_END(my_mm_list) && it2 != MM_LIST_REND(my_mm_list)) {
 *     // loop body ...
 *     it1 = MM_LIST_NEXT(my_mm_list, it1);
 *     it2 = MM_LIST_PREV(my_mm_list, it2);
 * }
 * ```
 *
 */

#ifndef __MM_LIST_H
#define __MM_LIST_H

#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

/*----------------------------------------------------------------------------*
 * BASIC TYPE DEFINITIONS                                                     *
 *----------------------------------------------------------------------------*/

struct mm_list_node_s;
struct mm_list_s;

/** @brief The list node typedef.
 */
typedef struct mm_list_node_s mm_list_node_t;

/** @brief The list root typedef.
 */
typedef struct mm_list_s mm_list_t;

struct mm_list_node_s {
        mm_list_node_t *next;
        mm_list_node_t *prev;
};

struct mm_list_s {
        struct mm_list_node_s base;
        unsigned long n;
};

/** @brief A list iterator, advance with MM_LIST_NEXT/PREV and dereference with MM_LIST_ITEM.
 */
typedef mm_list_node_t *mm_list_iterator_t;

/** @brief A list iterator that forbids modifications to the list structure.
 */
typedef const mm_list_node_t *mm_list_const_iterator_t;


/*----------------------------------------------------------------------------*
 * MM_LIST DEFINITION MACROS                                                     *
 *----------------------------------------------------------------------------*/

/** @brief Declares a list.
 * @param name Name of the list variable.
 */
#define MM_LIST_DECLARE(name) mm_list_t name

/** @brief An empty list reference. You can assign this value to an mm_list_t object.
 * @param name Name of the list variable.
 */
#define MM_LIST_INIT(name) {.base = {.next = &(name.base), .prev = &(name.base)}, .n = 0}

/** @brief Defines and initializes an empty list.
 * @param name Name of the list variable.
 */
#define MM_LIST_DEFINE(name) mm_list_t name = MM_LIST_INIT(name)


/*----------------------------------------------------------------------------*
 * MM_LIST ITERATORS                                                             *
 *----------------------------------------------------------------------------*/

/** @brief Returns a list iterator to the first element.
 * @note Always point to a valid element when `BEGIN != END`.
 */
#define MM_LIST_BEGIN(mm_list) ((mm_list).base.next)

/** @brief Advance a list iterator.
 */
#define MM_LIST_NEXT(mm_list, iter) ((iter)->next)

/** @brief Marks the end of a list iterator.
 * @note This iterator does not to a valid element in the list.
 */
#define MM_LIST_END(mm_list) (&((mm_list).base))

/** @brief Returns a list iterator to the last element.
 * @note Always point to a valid element when `RBEGIN != REND`.
 */
#define MM_LIST_RBEGIN(mm_list) ((mm_list).base.prev)

/** @brief Advance a list reverse iterator.
 */
#define MM_LIST_PREV(mm_list, iter) ((iter)->prev)

/** @brief Marks the end of a mm_list reverse iterator.
 * @note This iterator does not to a valid element in the list.
 */
#define MM_LIST_REND(mm_list) (&((mm_list).base))

/** @brief Iterates forward through the list.
 * @param mm_list (mm_list_t) The list head.
 * @param iter (identifier) Loop variable name to use.
 */
#define MM_LIST_FOREACH(mm_list, iter) for( \
        mm_list_iterator_t iter = MM_LIST_BEGIN(mm_list), iter##_next = MM_LIST_NEXT(mm_list, MM_LIST_BEGIN(mm_list)); \
        iter != MM_LIST_END(mm_list); \
        iter = iter##_next, iter##_next = MM_LIST_NEXT(mm_list, iter))

/** @brief Iterates backwords through the list.
 * @param mm_list (mm_list_t) The list head.
 * @param iter (identifier) Loop variable name to use.
 */
#define MM_LIST_FOREACH_REVERSE(mm_list, iter) for( \
        mm_list_iterator_t iter = MM_LIST_RBEGIN(mm_list), iter##_prev = MM_LIST_PREV(mm_list, MM_LIST_RBEGIN(mm_list)); \
        iter != MM_LIST_REND(mm_list); \
        iter = iter##_prev, iter##_prev = MM_LIST_PREV(mm_list, iter))

/** @brief A modification of MM_LIST_FOREACH optimized for operations that preserve the list structure.
 * @param mm_list (mm_list_t) The list.
 * @param iter (identifier) Loop variable name to use.
 */
#define MM_LIST_FOREACH_READ(mm_list, iter) for( \
        mm_list_const_iterator_t iter = MM_LIST_BEGIN(mm_list); \
        iter != MM_LIST_END(mm_list); \
        iter = MM_LIST_NEXT(mm_list, iter))

/** @brief A modification of MM_LIST_FOREACH_REVERSE optimized for operations that preserve the list structure.
 * @param mm_list (mm_list_t) The list.
 * @param iter (identifier) Loop variable name to use.
 */
#define MM_LIST_FOREACH_REVERSE_READ(mm_list, iter) for( \
        mm_list_const_iterator_t iter = MM_LIST_RBEGIN(mm_list); \
        iter != MM_LIST_REND(mm_list); \
        iter = MM_LIST_PREV(mm_list, iter))

#ifdef container_of
/** @brief Retrieves the structure containing the list node.
 * @param iter (mm_list_iterator_t) The current list node pointer.
 * @param type (type) The struct type the node is embedded in.
 * @param member (identifier) The name of the mm_list_node_t member in the struct.
 */
#define MM_LIST_ITEM(iter, type, member) container_of(iter, type, member)
#else
/** @brief Retrieves the structure containing the list node.
 * @param iter (mm_list_iterator_t) The current list node pointer.
 * @param type (type) The struct type the node is embedded in.
 * @param member (identifier) The name of the mm_list_node_t member in the struct.
 */
#define MM_LIST_ITEM(iter, type, member) ((type *)((char *) (iter) - (unsigned long) &((type *) 0)->member))
#endif

/** @brief Return the number of elements in the list
 * @param mm_list (mm_list_t) The list.
 */
#define MM_LIST_LEN(mm_list) ((mm_list).n)


/*----------------------------------------------------------------------------*
 * MM_LIST STRUCTURE MODIFICATION                                                *
 *----------------------------------------------------------------------------*/

/** @brief Inserts a node after a specific index.
 * @param mm_list (mm_list_t) The list.
 * @param index (mm_list_iterator_t) The node to insert after.
 * @param node (mm_list_node_t) The new node to insert (must be a variable, not a pointer).
 */
#define MM_LIST_INSERT_AFTER(mm_list, index, node) do { \
        mm_list_node_t *__index = (index); \
        mm_list_node_t *__node_ptr = &(node); \
        __node_ptr->prev = __index; \
        __node_ptr->next = __index->next; \
        __index->next->prev = __node_ptr; \
        __index->next = __node_ptr; \
        mm_list.n++; \
} while(0)


/** @brief Inserts a node before a specific index.
 * @param mm_list (mm_list_t) The list.
 * @param index (mm_list_iterator_t) The node to insert after.
 * @param node (mm_list_node_t) The new node to insert (must be a variable, not a pointer).
 */
#define MM_LIST_INSERT_BEFORE(mm_list, index, node) do { \
        mm_list_node_t *__index = (index); \
        mm_list_node_t *__node_ptr = &(node); \
        __node_ptr->next = __index; \
        __node_ptr->prev = __index->prev; \
        __index->prev->next = __node_ptr; \
        __index->prev = __node_ptr; \
        mm_list.n++; \
} while(0)


/** @brief Appends a node in front of the list (first place).
 * @param mm_list (mm_list_t) The list.
 * @param node (mm_list_node_t) The new node to insert (must be a variable, not a pointer).
 */
#define MM_LIST_ADD_FRONT(mm_list, node) MM_LIST_INSERT_AFTER(mm_list, &(mm_list).base, node)

/** @brief Appends a node as the tail of the list (last place).
 * @param mm_list (mm_list_t) The list.
 * @param node (mm_list_node_t) The new node to insert (must be a variable, not a pointer).
 */
#define MM_LIST_ADD_BACK(mm_list, node) MM_LIST_INSERT_BEFORE(mm_list, &(mm_list).base, node)

/** @brief Removes a node from the list.
 * @param mm_list (mm_list_t) The list.
 * @param index (mm_list_iterator_t) The node to remove.
 * @note After removal, the node cannot be used anymore in MM_LIST_* functions.
 */
#define MM_LIST_REMOVE(mm_list, index) do { \
        mm_list_node_t *__index = (index); \
        __index->prev->next = __index->next; \
        __index->next->prev = __index->prev; \
        __index->prev = __index; \
        __index->next = __index; \
        mm_list.n--; \
} while(0)

/** @brief Removes the first node in the list.
 * @param mm_list (mm_list_t) The list.
 * @note After removal, the node cannot be used anymore in MM_LIST_* functions.
 */
#define MM_LIST_REMOVE_FRONT(mm_list) MM_LIST_REMOVE(mm_list, MM_LIST_BEGIN(mm_list))

/** @brief Removes the first node in the list.
 * @param mm_list (mm_list_t) The list.
 * @note After removal, the node cannot be used anymore in MM_LIST_* functions.
 */
#define MM_LIST_REMOVE_BACK(mm_list) MM_LIST_REMOVE(mm_list, MM_LIST_RBEGIN(mm_list))

#ifdef __cplusplus
}
#endif

#endif /* __MM_LIST_H */
