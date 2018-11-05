/*****************************************************************************
 *
 * Original Author: Andrei Radulescu-Banu
 * Creation Date:   
 * Description: 
 * 
 * Copyright (C) 2018 by Andrei Radulescu-Banu.  All Rights Reserved.
 * Unauthorized reproduction, modification, distribution, transmission, 
 * republication, display or performance are strictly prohibited.
 ****************************************************************************/

/*****************************************************************************
 *                                INCLUDE FILES 
 *****************************************************************************/
#include "mmr.h"


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
 *                        mmr_event_loop
 *============================================================================
 * Description:      The module manager event loop
 * Parameters:    
 * Returns:  
 */
void mmr_event_loop(void *thread_arg) {
    struct bitd_pollfd p;
    int ret;
    bitd_uint32 tmo;
    
    mmr_log(log_level_trace, "Event loop started");

    while (!g_mmr_cb->stopping_p) {

	bitd_mutex_lock(g_mmr_cb->lock);

	/* Tick the timers */
	bitd_timer_list_tick(g_mmr_cb->timers);

	/* Get the list timer expiration */
	tmo = bitd_timer_list_get_timeout_msec(g_mmr_cb->timers);

	/* Normalize it */
	tmo = MAX(tmo, 1);
	
	bitd_mutex_unlock(g_mmr_cb->lock);

	mmr_log(log_level_trace, "Event loop, timer list count %ld, min tmo %d", bitd_timer_list_count(g_mmr_cb->timers), tmo);

	/* Wait on the stop event */
	memset(&p, 0, sizeof(p));
	p.fd = bitd_event_to_fd(g_mmr_cb->event_loop_ev);
	p.events = BITD_POLLIN;

	mmr_log(log_level_trace, "Event loop poll, tmo %u, stopping %d", tmo, g_mmr_cb->stopping_p);

	ret = bitd_poll(&p, 1, tmo);
	if (ret > 0) {
	    if (p.revents & BITD_POLLIN) {
		mmr_log(log_level_trace, "Wake up event detected");
		
		/* Clear the event */
		bitd_event_clear(g_mmr_cb->event_loop_ev);
		/* Go back to top of while loop, where we check 
		   if we're stopping */
		continue;
	    }
	}
    }

    mmr_log(log_level_trace, "Event loop stopped");
}
