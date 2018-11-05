/*****************************************************************************
 *
 * Original Author: Andrei Radulescu-Banu
 * Creation Date:   
 * Description: 
 * 
 * Copyright 2018 by Andrei Radulescu-Banu.  All Rights Reserved.
 * Unauthorized reproduction, modification, distribution, transmission, 
 * republication, display or performance are strictly prohibited.
 ****************************************************************************/

/*****************************************************************************
 *                                INCLUDE FILES 
 *****************************************************************************/
#include "bitd/timer-list.h"

/*****************************************************************************
 *                             MANIFEST CONSTANTS
 *****************************************************************************/


/*****************************************************************************
 *                                  MACROS 
 *****************************************************************************/

#define BITD_TMO_MAX_MSEC 0x80000000


#define TIMER_LIST_HEAD(l) \
  ((bitd_timer)(((char *)&l->head) - offsetof(struct bitd_timer_s, next)))


#define bitd_printf if(0) printf

/*****************************************************************************
 *                                  TYPES
 *****************************************************************************/

/* Timer object */
struct bitd_timer_s {
    struct bitd_timer_list_s *l;         /* The timer list (possibly NULL) */
    bitd_uint64 tmo_nsec;                /* Expiration timeout */
    bitd_timer_expired_callback *expiration_callback;
    void *expiration_cookie;             /* Callback cookie */
    bitd_timer next, prev;               /* Timer list */
};

/* Timer list object */
struct bitd_timer_list_s {
    bitd_timer head, tail; /* Timer list */
    long count;          /* Count of elements in the list */
    bitd_mutex lock;
    int ticks_max;       /* Max ticks per pass. No limit if set to zero */
};


/*****************************************************************************
 *                           FUNCTION DECLARATION
 *****************************************************************************/



/*****************************************************************************
 *                                VARIABLES
 *****************************************************************************/



/*****************************************************************************
 *                          FUNCTION IMPLEMENTATION
 *****************************************************************************/




/*
 *============================================================================
 *                        bitd_timer_create
 *============================================================================
 * Description:     Create a timer
 * Parameters:    
 * Returns:  
 */
bitd_timer bitd_timer_create(void) {
    bitd_timer t;
    
    t = calloc(1, sizeof(*t));
    return t;
} 



/*
 *============================================================================
 *                        bitd_timer_destroy
 *============================================================================
 * Description:     Destroy a timer. If enqueued on a timer list, the timer is
 *    removed from the list.
 * Parameters:    
 *     t - the timer handle
 * Returns:  
 */
void bitd_timer_destroy(bitd_timer t) {

    if (t) {
	/* Take it off the list - if it's on a list */
	bitd_timer_list_remove(t);

	/* Free the timer */
        free(t);
    }
} 



/*
 *============================================================================
 *                        bitd_timer_list_create
 *============================================================================
 * Description:     Create a timer list.
 * Parameters:    
 * Returns:  
 *     The timer list handle
 */
bitd_timer_list bitd_timer_list_create(void) {
    bitd_timer_list l;
    
    l = calloc(1, sizeof(*l));
    if (l) {
        l->head = TIMER_LIST_HEAD(l);
        l->tail = TIMER_LIST_HEAD(l);
	l->lock = bitd_mutex_create();
    }
    return l;
} 



/*
 *============================================================================
 *                        bitd_timer_list_destroy
 *============================================================================
 * Description:     Destroy a timer list, removing all timers on the list.
 * Parameters:    
 *     l - the timer list
 * Returns:  
 */
void bitd_timer_list_destroy(bitd_timer_list l) {
    bitd_timer t;

    if (l) {
	bitd_mutex_lock(l->lock);

        /* Remove the list timers, if any */
        while ((t = l->head) != TIMER_LIST_HEAD(l)) {
            bitd_timer_destroy(t);
        }

	bitd_mutex_unlock(l->lock);
	bitd_mutex_destroy(l->lock);

        free(l);
    }
} 



/*
 *============================================================================
 *                        bitd_timer_list_add_msec
 *============================================================================
 * Description:     Add an expiration timer, in msecs
 * Parameters:    
 *     l - the timer list handle
 *     t - the timer handle
 *     tmo_msec - the expiration time, in msecs
 *     expiration_callback - the function pointer called at expiration
 *     expiration_cookie - argument passed to the above
 * Returns:  
 */
void bitd_timer_list_add_msec(bitd_timer_list l, 
			      bitd_timer t, 
			      bitd_uint32 tmo_msec,
			      bitd_timer_expired_callback *expiration_callback,
			      void *expiration_cookie) {

    bitd_timer_list_add_nsec(l, t,
			   tmo_msec * 1000000ULL,
			   expiration_callback, expiration_cookie);
} 


/*
 *============================================================================
 *                        bitd_timer_list_add_nsec
 *============================================================================
 * Description:     Add a timer to the list
 * Parameters:    
 *     l - the timer list handle
 *     t - the timer handle
 *     tmo_nsec - the expiration time, in nanosecs
 *     expiration_callback - the function pointer called at expiration
 *     expiration_cookie - argument passed to the above
 * Returns:  
 */
void bitd_timer_list_add_nsec(bitd_timer_list l, 
			      bitd_timer t, 
			      bitd_uint64 tmo_nsec,
			      bitd_timer_expired_callback *expiration_callback,
			      void *expiration_cookie) {
    bitd_timer t1;
    bitd_uint64 current_time = bitd_get_time_nsec();

    bitd_printf("%s() tmo_nsec %llu\n", __FUNCTION__, tmo_nsec);
    
    bitd_mutex_lock(l->lock);

    /* Ensure timer is not on any list */
    bitd_timer_list_remove(t);
    
    /* Set the timeout value */
    t->tmo_nsec = current_time + tmo_nsec;

    bitd_printf("%s() t->tmo_nsec %llu\n", __FUNCTION__, t->tmo_nsec);
    
    /* Save callback and cookie */
    t->expiration_callback = expiration_callback;
    t->expiration_cookie = expiration_cookie;
 
    /* Add to the active list */
    if (l->head == TIMER_LIST_HEAD(l) || 
        l->head->tmo_nsec > t->tmo_nsec) {
        /* Add to head */
        t->next = l->head;
        t->prev = TIMER_LIST_HEAD(l);        
    } else {
        /* Add to the list of timeouts. Start search with the tail to 
           minimize search time. */
        t1 = l->tail;
        if (t1->tmo_nsec <= t->tmo_nsec) {
            /* Add to tail */
            t->prev = t1;
            t->next = t1->next;
        } else {            
            while (t1 != TIMER_LIST_HEAD(l) && 
                   t1->tmo_nsec > t->tmo_nsec) {
                t1 = t1->prev;
            }
            /* Insert timer right after t1 */
            t->prev = t1;
            t->next = t1->next;
        }
    }

    /* Chain to the list */
    t->prev->next = t;
    t->next->prev = t;    
    
    /* Update the list count */
    l->count++;

    /* Save the list in the timer structure */
    t->l = l;
    
    bitd_mutex_unlock(l->lock);
} 


/*
 *============================================================================
 *                        bitd_timer_list_remove
 *============================================================================
 * Description:     
 * Parameters:    
 *     l - the timer list
 * Returns:  
 */
void bitd_timer_list_remove(bitd_timer t) {

    bitd_timer_list l;

    if (!t) {
	return;
    }

    l = t->l;
    if (!l) {
	return;
    }

    bitd_mutex_lock(l->lock);    

    /* If timer is on a list, it should have a next and prev pointer */
    bitd_assert(t->next && t->prev);
    
    if (t->next)
    {
        /* Remove from doubly linked list */
        t->next->prev = t->prev;
        t->prev->next = t->next;

	/* Update the list count */
	l->count--;

	/* Timer is off the list */
	t->next = NULL;
	t->prev = NULL;
    }

    t->l = NULL;

    bitd_mutex_unlock(l->lock);    
} 



/*
 *============================================================================
 *                        bitd_timer_list_tick
 *============================================================================
 * Description:     
 * Parameters:    
 *     l - the timer list
 * Returns:  
 */
void bitd_timer_list_tick(bitd_timer_list l) {
    bitd_timer t;
    bitd_uint64 current_time;
    int ticks = 0;
    
    /* Get current time */
    current_time = bitd_get_time_nsec();

    bitd_printf("%s() current_time %llu\n", __FUNCTION__, current_time);

    bitd_mutex_lock(l->lock);    

    while((t = l->head) != TIMER_LIST_HEAD(l) && 
          current_time >= t->tmo_nsec) {
        
	bitd_printf("%s() t->tmo_nsec  %llu\n", __FUNCTION__, t->tmo_nsec);

        /* Remove timer from the list */
        bitd_timer_list_remove(t);
        
        /* Call the expiration callback */
        if (t->expiration_callback) {
	    bitd_mutex_unlock(l->lock);    
            t->expiration_callback(t, t->expiration_cookie);
	    bitd_mutex_lock(l->lock);

	    ticks++;
	    if (l->ticks_max && ticks >= l->ticks_max) {
		break;
	    }
        }
    }

    bitd_mutex_unlock(l->lock);    
} 


/*
 *============================================================================
 *                        bitd_timer_list_get_timeout_msec
 *============================================================================
 * Description:     Returns the time until the next expiration, or BITD_FOREVER 
       if no next expiration. It is possible for the returned timeout to be
       shorter than the next expiration, since it is returned in 32 bit msecs, 
       and msecs wrap around quicker than the 64 bit nsec used to store the
       timeout internally.
 * Parameters:    
 *     l - the timer list
 * Returns:  
 *    The next expiration time, in msecs, ot BITD_FOREVER if no next expiration.
 */
bitd_uint32 bitd_timer_list_get_timeout_msec(bitd_timer_list l) {
    bitd_uint32 ret = BITD_FOREVER;
    bitd_timer t;
    bitd_uint64 current_time;
    bitd_uint64 tmo_nsec;
    bitd_uint32 tmo_msec;

    /* Compute the current time */
    current_time = bitd_get_time_nsec();

    bitd_printf("%s() current_time %llu\n", __FUNCTION__, current_time);

    bitd_mutex_lock(l->lock);    

    /* Get the list head */
    t = l->head;

    /* Return the next expiration */
    if (t != TIMER_LIST_HEAD(l)) {
	bitd_assert(t->next && t->prev);

        if (current_time >= t->tmo_nsec) {
	    bitd_printf("%s() current_time %llu >= t->tmo_nsec %llu ret 0\n",
		      __FUNCTION__, current_time, t->tmo_nsec);
            ret = 0;
        } else {
            tmo_nsec = t->tmo_nsec - current_time;

	    /* Normalize the tmo_nsec so it does not wrap around on conversion
	       to millisecs */
	    tmo_nsec = MIN(tmo_nsec, BITD_TMO_MAX_MSEC * 1000000ULL);
	    tmo_msec = (tmo_nsec / 1000000ULL) + 1;

	    bitd_printf("%s() tmo_nsec %llu tmo_msec %u\n",
		      __FUNCTION__, tmo_nsec, tmo_msec);

	    ret = tmo_msec;
        }
    }

    bitd_mutex_unlock(l->lock);

    return ret;
} 


/*
 *============================================================================
 *                        bitd_timer_list_set_ticks_max
 *============================================================================
 * Description:     Set the max number of ticks we can process per 
 *     call to bitd_timer_list_tick()
 * Parameters:    
 *     l - the timer list
 * Returns:  
 */
void bitd_timer_list_set_ticks_max(bitd_timer_list l, int ticks_max) {
    l->ticks_max = ticks_max;
} 


/*
 *============================================================================
 *                        bitd_timer_list_count
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
long bitd_timer_list_count(bitd_timer_list l) {
    return l ? l->count : 0;
} 


/*
 *============================================================================
 *                        _bitd_assert_timer_list
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
void _bitd_assert_timer_list(bitd_timer_list l) {
    bitd_timer t;
    int i = 0;

    _bitd_assert(l);

    for (t = l->head; t != TIMER_LIST_HEAD(l); t = t->next) {
	_bitd_assert(t->l == l);
	i++;
	_bitd_assert(i <= l->count);
    }
} 
