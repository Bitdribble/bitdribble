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

#ifndef _BITD_TIMER_LIST_H_
#define _BITD_TIMER_LIST_H_

/*****************************************************************************
 *                                INCLUDE FILES 
 *****************************************************************************/
#include "bitd/common.h"


#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

/*****************************************************************************
 *                             MANIFEST CONSTANTS
 *****************************************************************************/



/*****************************************************************************
 *                                  MACROS 
 *****************************************************************************/



/*****************************************************************************
 *                                  TYPES
 *****************************************************************************/



/*****************************************************************************
 *                            FUNCTION DEFINITIONS
 *****************************************************************************/


/* Timer handle */
typedef struct bitd_timer_s *bitd_timer;

/* Timer list handle */
typedef struct bitd_timer_list_s *bitd_timer_list;

typedef void (bitd_timer_expired_callback)(bitd_timer t, void *cookie);


/*****************************************************************************
 *                            FUNCTION DEFINITIONS
 *****************************************************************************/

/* Create/destroy a timer element */
bitd_timer bitd_timer_create(void);
void bitd_timer_destroy(bitd_timer t);

/* Create/destroy a timer list */
bitd_timer_list bitd_timer_list_create(void);
void bitd_timer_list_destroy(bitd_timer_list l);

/* Add/remove a timer from the tmo list */
void bitd_timer_list_add_msec(bitd_timer_list l, 
			      bitd_timer t, 
			      bitd_uint32 tmo_msec,
			      bitd_timer_expired_callback *expiration_callback,
			      void *expiration_cookie);
void bitd_timer_list_add_nsec(bitd_timer_list l, 
			      bitd_timer t, 
			      bitd_uint64 tmo_nsec,
			      bitd_timer_expired_callback *expiration_callback,
			      void *expiration_cookie);
void bitd_timer_list_remove(bitd_timer t);

/* Call the expiration handler on all expired timers on the list. */
void bitd_timer_list_tick(bitd_timer_list l);

/* Set a tick limit, so the tick api does not end up in an infinite loop. */
void bitd_timer_list_set_ticks_max(bitd_timer_list l, int ticks_max);
    
/* Returns the time until the next expiration, or BITD_FOREVER if no next
   expiration. It is possible for the returned timeout to be shorter
   than the next expiration, since it is returned in 32 bit msecs, and msecs
   wrap around quicker than the 64 bit nsec used to store the
   timeout internally. */
bitd_uint32 bitd_timer_list_get_timeout_msec(bitd_timer_list l);

/* Return the number of timers on list */
long bitd_timer_list_count(bitd_timer_list l);

void _bitd_assert_timer_list(bitd_timer_list l);
#ifdef NDEBUG
# define bitd_assert_timer_list(l) do {} while (0)
#else
# define bitd_assert_timer_list(l) _bitd_assert_timer_list(l)
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _UTILS_TIMER_LIST_H_ */
