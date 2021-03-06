/*
 * Copyright 2008-2013 by Aerospike.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is furnished to do
 * so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/**
 *  double linked list functionality
 */

#include <errno.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>
#include <unistd.h>

#include <citrusleaf/cf_ll.h>

/**
 * SYNOPSIS
 * LinkedList
 * Sometimes the answer is a doubly linked list. It's not that frequent, but
 * all the corner cases in a double linked list can be annoying.
 */
 
/******************************************************************************
 * CONSTANTS
 ******************************************************************************/

#define DEBUG 1

/******************************************************************************
 * MACROS
 ******************************************************************************/

#ifdef EXTERNAL_LOCKS
#include <citrusleaf/cf_hooks.h>
#define LL_UNLOCK(_ll) 	if ( _ll->uselock ) { cf_hooked_mutex_unlock(_ll->LOCK); }
#define LL_LOCK(_ll) 	if ( _ll->uselock ) { cf_hooked_mutex_lock(_ll->LOCK); }
#else
#define LL_UNLOCK(_ll) 	if ( _ll->uselock ) { pthread_mutex_unlock(&(_ll->LOCK)); }
#define LL_LOCK(_ll)	if ( _ll->uselock ) { pthread_mutex_lock(&(_ll->LOCK)); }
#endif


/******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

void cf_ll_prepend_lockfree(cf_ll * ll, cf_ll_element * e) {
	// empty list
	if (ll->head == 0) { 
		ll->head = e;
		ll->tail = e;
		e->next = 0;
		e->prev = 0;
	}

	// at least one element - add to head
	else {
		e->next = ll->head;
		e->prev = 0;
		
		ll->head->prev = e;
		ll->head = e;
	}
	
	ll->sz++;
}

void cf_ll_prepend(cf_ll *ll, cf_ll_element *e  ) {
	LL_LOCK(ll);
	cf_ll_prepend_lockfree(ll, e);
	LL_UNLOCK(ll)
}

void cf_ll_append_lockfree(cf_ll *ll, cf_ll_element *e  ) {
	if (ll->head == 0) { 
		ll->head = e;
		ll->tail = e;
		e->next = 0;
		e->prev = 0;
	}
	// at least one element - add to tail
	else {
		e->next = 0;
		e->prev = ll->tail;
		
		ll->tail->next = e;
		ll->tail = e;
	}
	
	ll->sz++;
}


void cf_ll_append(cf_ll *ll, cf_ll_element *e  ) {
	LL_LOCK(ll);

	cf_ll_append_lockfree(ll, e);

	LL_UNLOCK(ll);
}

void cf_ll_insert_after_lockfree(cf_ll *ll, cf_ll_element *cur, cf_ll_element *ins) {
	ins->next = cur->next;
	ins->prev = cur;
	
	if (cur->next == 0)
		ll->tail = ins;
	else
		cur->next->prev = ins;

	cur->next = ins;
	
	ll->sz++;
}


void cf_ll_insert_after(cf_ll *ll, cf_ll_element *cur, cf_ll_element *ins) {
	LL_LOCK(ll);
	cf_ll_insert_after_lockfree(ll, cur, ins);
	LL_UNLOCK(ll);
}

void cf_ll_insert_before_lockfree(cf_ll *ll, cf_ll_element *cur, cf_ll_element *ins) {

	ins->next = cur;
	ins->prev = cur->prev;

	if (cur->prev == 0)
		ll->head = ins;
	else
		cur->prev->next = ins;
	cur->prev = ins;
	
	ll->sz++;
}

void cf_ll_insert_before(cf_ll *ll, cf_ll_element *cur, cf_ll_element *ins) {
	LL_LOCK(ll);
	cf_ll_insert_before_lockfree( ll, cur, ins);
	LL_UNLOCK(ll);
}


void cf_ll_delete_lockfree(cf_ll *ll, cf_ll_element *e ) {
	// make empty
	if (ll->sz == 1) {
		ll->head = 0;
		ll->tail = 0;
	}
	
	// head delete
	else if (e == ll->head) {
		ll->head = e->next;
		e->next->prev = 0;
	}
	// tail delete
	else if (e == ll->tail) {
		ll->tail = e->prev;
		e->prev->next = 0;
	}
	// we're in the middle somewhere
	else {
		e->prev->next = e->next;
		e->next->prev = e->prev;
	}
	
	ll->sz --;
	
	if (ll->destroy_fn)
		(ll->destroy_fn) (e);
}

void cf_ll_delete(cf_ll *ll, cf_ll_element *e ) {
	// extra check for fun
	if (ll->sz == 0)	return;

	LL_LOCK(ll);

	cf_ll_delete_lockfree(ll, e);
	
	LL_UNLOCK(ll);
	
}


int cf_ll_reduce( cf_ll *ll, bool forward, cf_ll_reduce_fn fn, void *udata) {
	int rv = 0;
	
	LL_LOCK(ll);
	
	cf_ll_element *cur = forward ? ll->head : ll->tail;
	
	while (cur) {
		
		if ( (rv = (*fn) (cur, udata) ) ) {
			
			if (rv == CF_LL_REDUCE_DELETE) {

				cf_ll_element *next = forward ? cur->next : cur->prev;
				// Calls the destructor on cur, can't touch it after that
				cf_ll_delete_lockfree(ll, cur);
				cur = next;
				
			}
			else
				goto Exit;
		}
		else {
			cur = forward ? cur->next : cur->prev;
		}
	}

Exit:		
	LL_UNLOCK(ll);
	return(rv);
}

int cf_ll_insert_reduce(cf_ll *ll, cf_ll_element *e, bool forward, cf_ll_reduce_fn fn, void *udata) {
	int rv = 0;
	LL_LOCK(ll);

	cf_ll_element *cur = forward ? ll->head : ll->tail;
	
	while (cur) {
		
		if ( (rv = (*fn) (cur, udata) ) ) {
			
			if (rv == CF_LL_REDUCE_INSERT) {

				if (forward)
					cf_ll_insert_before_lockfree(ll, cur, e);
				else
					cf_ll_insert_after_lockfree(ll, cur, e);
				rv = 0;
				
			}
			
			goto Exit;
			
		}
		else {
			cur = forward ? cur->next : cur->prev;
		}
	}
	
	// give chance to insert at "end"
	if ( (rv = (*fn) (cur, udata) ) ) {
		if (rv == CF_LL_REDUCE_INSERT) {
			if (forward)
				cf_ll_append_lockfree(ll, e);
			else
				cf_ll_prepend_lockfree(ll, e);
			rv = 0;
		}
	}

Exit:
	LL_UNLOCK(ll);
	return(rv);
}

uint32_t cf_ll_size(cf_ll *ll) {
	LL_LOCK(ll);
	uint32_t sz = ll->sz;
	LL_UNLOCK(ll);
	return(sz);
}	

int  cf_ll_init(cf_ll *ll, cf_ll_destructor destroy_fn, bool uselock) {
	ll->head = 0;
	ll->tail = 0;
	ll->destroy_fn = destroy_fn;
	ll->sz = 0;
	ll->uselock = uselock;
	if (uselock) {
#ifdef EXTERNAL_LOCKS
		ll->LOCK = cf_hooked_mutex_alloc();
#else
		pthread_mutex_init(&ll->LOCK, 0);
#endif 
	}
	return(0);	
}
