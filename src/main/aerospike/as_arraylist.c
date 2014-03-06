/******************************************************************************
 *	Copyright 2008-2013 by Aerospike.
 *
 *	Permission is hereby granted, free of charge, to any person obtaining a copy 
 *	of this software and associated documentation files (the "Software"), to 
 *	deal in the Software without restriction, including without limitation the 
 *	rights to use, copy, modify, merge, publish, distribute, sublicense, and/or 
 *	sell copies of the Software, and to permit persons to whom the Software is 
 *	furnished to do so, subject to the following conditions:
 * 
 *	The above copyright notice and this permission notice shall be included in 
 *	all copies or substantial portions of the Software.
 * 
 *	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 *	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 *	IN THE SOFTWARE.
 *****************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include <citrusleaf/cf_alloc.h>

#include <aerospike/as_arraylist.h>
#include <aerospike/as_arraylist_iterator.h>
#include <aerospike/as_list.h>

#include "internal.h"

/*******************************************************************************
 *	EXTERNS
 ******************************************************************************/

extern const as_list_hooks as_arraylist_list_hooks;

/*******************************************************************************
 *	INLINE FUNCTIONS
 ******************************************************************************/


/*******************************************************************************
 *	INSTANCE FUNCTIONS
 ******************************************************************************/

/**
 *	Initialize an arraylist, with room for  "capacity" number of elements
 *	and with the new (delta) allocation amount of "block_size" elements.
 *	Use calloc() for the new element memory so that it is initialized to zero
 */
as_arraylist * as_arraylist_init(as_arraylist * list, uint32_t capacity, uint32_t block_size) 
{
	if ( !list ) return list;

	as_list_cons((as_list *) list, false, NULL, &as_arraylist_list_hooks);
	list->block_size = block_size;
	list->capacity = capacity;
	list->size = 0;
	if ( list->capacity > 0 ) {
		list->free = true;
		list->elements = (as_val **) calloc( capacity, sizeof(as_val *) );
	}
	else {
		list->free = false;
		list->elements = NULL;
	}
	return list;
}

/**
 *	Create a new arraylist, with room for  "capacity" number of elements
 *	and with the new (delta) allocation amount of "block_size" elements.
 *	Use calloc() for the new element memory so that it is initialized to zero
 */
as_arraylist * as_arraylist_new(uint32_t capacity, uint32_t block_size) 
{
	as_arraylist * list = (as_arraylist *) malloc(sizeof(as_arraylist));
	if ( !list ) return list;

	as_list_cons((as_list *) list, true, NULL, &as_arraylist_list_hooks);
	list->block_size = block_size;
	list->capacity = capacity;
	list->size = 0;
	if ( list->capacity > 0 ) {
		list->free = true;
		list->elements = (as_val **) calloc( capacity, sizeof(as_val *) );
	}
	else {
		list->free = false;
		list->elements = NULL;
	}
	return list;
}

/**
 *	@private
 *	Release resources allocated to the list.
 *
 *	@param list	The list.
 *
 *	@return TRUE on success.
 */
bool as_arraylist_release(as_arraylist * list)
{
	if ( list->elements ) {
		for (int i = 0; i < list->size; i++ ) {
			if (list->elements[i]) {
				as_val_destroy(list->elements[i]);
			}
			list->elements[i] = NULL;
		}

		if ( list->free ) {
			free(list->elements);
		}
	}
	
	list->elements = NULL;
	list->size = 0;
	list->capacity = 0;

	return true;
}

/**
 *	as_arraylist_destroy
 *	helper function for those who like the joy of as_arraylist_new
 */
void as_arraylist_destroy(as_arraylist * l)
{
	as_list_destroy((as_list *) l);
}


/*******************************************************************************
 *	STATIC FUNCTIONS
 ******************************************************************************/

/**
 *	Ensure delta elements can be added to the list, growing the list if necessary.
 * 
 *	@param l – the list to be ensure the capacity of.
 *	@param delta – the number of elements to be added.
 */
static int as_arraylist_ensure(as_arraylist * list, uint32_t delta) 
{
	// Check for capacity (in terms of elements, NOT size in bytes), and if we
	// need to allocate more, do a realloc.
	if ( (list->size + delta) > list->capacity ) {
		// by convention - we allocate more space ONLY when the unit of
		// (new) allocation is > 0.
		if ( list->block_size > 0 ) {
			// Compute how much room we're missing for the new stuff
			int new_room = (list->size + delta) - list->capacity;
			// Compute new capacity in terms of multiples of block_size
			// This will get us (conservatively) at least one block
			int new_blocks = (new_room + list->block_size) / list->block_size;
			int new_capacity = list->capacity + (new_blocks * list->block_size);
			as_val ** elements = (as_val **) realloc(list->elements, sizeof(as_val *) * new_capacity);
			if ( elements != NULL ) {
				// Looks like it worked, so fill in the new values
				list->elements = elements;
				list->capacity = new_capacity;  // New, Improved Size
				return AS_ARRAYLIST_OK;
			}
			list->elements = elements;
			return AS_ARRAYLIST_ERR_ALLOC;
		}
		return AS_ARRAYLIST_ERR_MAX;
	}

	return AS_ARRAYLIST_OK;
}

/*******************************************************************************
 *	INFO FUNCTIONS
 ******************************************************************************/

/**
 *	The hash value of the list.
 */
uint32_t as_arraylist_hashcode(const as_arraylist * list) 
{
	return 0;
}

/**
 *	The number of elements in the list.
 */
uint32_t as_arraylist_size(const as_arraylist * list) 
{
	return list->size;
}

/*******************************************************************************
 *	GET FUNCTIONS
 ******************************************************************************/

/**
 *	Return the element at the specified index.
 */
as_val * as_arraylist_get(const as_arraylist * list, const uint32_t i) 
{
	if ( i >= list->size ) return(NULL);
	return list->elements[i];
}

int64_t as_arraylist_get_int64(const as_arraylist * list, const uint32_t i) 
{
	return as_integer_get(as_integer_fromval(as_arraylist_get(list, i)));
}

char * as_arraylist_get_str(const as_arraylist * list, const uint32_t i) 
{
	return as_string_get(as_string_fromval(as_arraylist_get(list, i)));
}

extern inline as_integer * as_arraylist_get_integer(const as_arraylist * list, const uint32_t i);
extern inline as_string * as_arraylist_get_string(const as_arraylist * list, const uint32_t i);
extern inline as_bytes * as_arraylist_get_bytes(const as_arraylist * list, const uint32_t i);
extern inline as_list * as_arraylist_get_list(const as_arraylist * list, const uint32_t i);
extern inline as_map * as_arraylist_get_map(const as_arraylist * list, const uint32_t i);

/*******************************************************************************
 *	SET FUNCTIONS
 ******************************************************************************/

/**
 *	Set the arraylist (L) at element index position (i) with element value (v).
 *	Notice that in order to maintain proper object/memory management, we
 *	just first destroy (as_val_destroy()) the old object at element position(i)
 *	before assigning the new element.  Also note that the object at element
 *	position (i) is assumed to exist, so all element positions must be
 *	appropriately initialized to zero.
 */
int as_arraylist_set(as_arraylist * list, const uint32_t index, as_val * value) 
{
	int rc = AS_ARRAYLIST_OK;
	if ( index >= list->capacity ) {
		rc = as_arraylist_ensure(list, (index + 1) - list->capacity);
		if ( rc != AS_ARRAYLIST_OK ) {
			return rc;
		}
	}
	as_val_destroy(list->elements[index]);
	list->elements[index] = value;
	if( index  >= list->size ){
		list->size = index + 1;
	}

	return rc;
}

int as_arraylist_set_int64(as_arraylist * list, const uint32_t i, int64_t value) 
{
	return as_arraylist_set(list, i, (as_val *) as_integer_new(value));
}

int as_arraylist_set_str(as_arraylist * list, const uint32_t i, const char * value) 
{
	return as_arraylist_set(list, i, (as_val *) as_string_new(strdup(value), true));
}

extern inline int as_arraylist_set_integer(as_arraylist * list, const uint32_t i, as_integer * value);
extern inline int as_arraylist_set_string(as_arraylist * list, const uint32_t i, as_string * value);
extern inline int as_arraylist_set_bytes(as_arraylist * list, const uint32_t i, as_bytes * value);
extern inline int as_arraylist_set_list(as_arraylist * list, const uint32_t i, as_list * value);
extern inline int as_arraylist_set_map(as_arraylist * list, const uint32_t i, as_map * value);

/*******************************************************************************
 *	APPEND FUNCTIONS
 ******************************************************************************/

/**
 *	Add the element to the end of the list.
 */
int as_arraylist_append(as_arraylist * list, as_val * value) 
{
	int rc = as_arraylist_ensure(list, 1);
	if ( rc != AS_ARRAYLIST_OK ) return rc;
	list->elements[list->size++] = value;
	return rc;
}

int as_arraylist_append_int64(as_arraylist * list, int64_t value) 
{
	return as_arraylist_append(list, (as_val *) as_integer_new(value));
}

int as_arraylist_append_str(as_arraylist * list, const char * value) 
{
	return as_arraylist_append(list, (as_val *) as_string_new(strdup(value), true));
}

extern inline int as_arraylist_append_integer(as_arraylist * list, as_integer * value);
extern inline int as_arraylist_append_string(as_arraylist * list, as_string * value);
extern inline int as_arraylist_append_bytes(as_arraylist * list, as_bytes * value);
extern inline int as_arraylist_append_list(as_arraylist * list, as_list * value);
extern inline int as_arraylist_append_map(as_arraylist * list, as_map * value);

/*******************************************************************************
 *	PREPEND FUNCTIONS
 ******************************************************************************/

/**
 *	Add the element to the beginning of the list.
 */
int as_arraylist_prepend(as_arraylist * list, as_val * value) 
{
	int rc = as_arraylist_ensure(list, 1);
	if ( rc != AS_ARRAYLIST_OK ) return rc;

	for (int i = list->size; i > 0; i-- ) {
		list->elements[i] = list->elements[i-1];
	}

	list->elements[0] = value;
	list->size++;

	return rc;
}

int as_arraylist_prepend_int64(as_arraylist * list, int64_t value) 
{
	return as_arraylist_prepend(list, (as_val *) as_integer_new(value));
}

int as_arraylist_prepend_str(as_arraylist * list, const char * value) 
{
	return as_arraylist_prepend(list, (as_val *) as_string_new(strdup(value), true));
}


extern inline int as_arraylist_prepend_integer(as_arraylist * list, as_integer * value);
extern inline int as_arraylist_prepend_string(as_arraylist * list, as_string * value);
extern inline int as_arraylist_prepend_bytes(as_arraylist * list, as_bytes * value);
extern inline int as_arraylist_prepend_list(as_arraylist * list, as_list * value);
extern inline int as_arraylist_prepend_map(as_arraylist * list, as_map * value);

/*******************************************************************************
 *	ACCESSOR & MODIFICATION FUNCTIONS
 ******************************************************************************/

as_val * as_arraylist_head(const as_arraylist * list) 
{
	return list->elements[0];
}

/**
 *	returns all elements other than the head
 */
as_arraylist * as_arraylist_tail(const as_arraylist * list) 
{
	if ( list->size == 0 ) return NULL;

	as_arraylist * list2 = as_arraylist_new(list->size-1, list->block_size);

	for(int i = 1, j = 0; i < list->size; i++, j++) {
		if (list->elements[i]) {
			as_val_reserve(list->elements[i]);
			list2->elements[j] = list2->elements[i];
		}
		else {
			list2->elements[j] = 0;
		}
	}

	return list2;
}

/**
 *	Return a new list with the first n elements removed.
 */
as_arraylist * as_arraylist_drop(const as_arraylist * list, uint32_t n) 
{
	uint32_t		sz		= list->size;
	uint32_t		c		= n < sz ? n : sz;
	as_arraylist *	list2	= as_arraylist_new(sz-c, list->block_size);
	
	list2->size = sz-c;
	for(int i = c, j = 0; j < list2->size; i++, j++) {
		if (list->elements[i]) {
			as_val_reserve(list->elements[i]);
			list2->elements[j] = list->elements[i];
		}
		else {
			list2->elements[j] = 0;
		}
	}

	return list2;
}

/**
 *	Return a new list containing the first n elements.
 */
as_arraylist * as_arraylist_take(const as_arraylist * list, uint32_t n) 
{
	uint32_t		sz		= list->size;
	uint32_t		c		= n < sz ? n : sz;
	as_arraylist *	list2	= as_arraylist_new(c, list->block_size);

	list2->size = c;
	for(int i = 0; i < c; i++) {
		if (list->elements[i]) {
			as_val_reserve(list->elements[i]);
			list2->elements[i] = list->elements[i];
		}
		else {
			list2->elements[i] = 0;
		}
	}

	return list2;
}

/*******************************************************************************
 *	ITERATION FUNCTIONS
 ******************************************************************************/

/** 
 *	Call the callback function for each element in the list.
 */
bool as_arraylist_foreach(const as_arraylist * list, as_list_foreach_callback callback, void * udata) 
{
	for(int i = 0; i < list->size; i++ ) {
		if ( callback(list->elements[i], udata) == false ) {
			return false;
		}
	}
	return true;
}
