/*****************************************************************************
 *
 * Original Author: 
 * Creation Date:   
 * Description: 
 *
 ****************************************************************************/

/*****************************************************************************
 *                                INCLUDE FILES 
 *****************************************************************************/
#include "bitd/timer-thread.h"
#include "bitd/timer-list.h"


/*****************************************************************************
 *                             MANIFEST CONSTANTS
 *****************************************************************************/



/*****************************************************************************
 *                                  MACROS 
 *****************************************************************************/
#define tth_printf if (0) printf


/*****************************************************************************
 *                                  TYPES
 *****************************************************************************/
struct timer_thread_s {
    bitd_mutex lock;
    bitd_timer_list timers;
    bitd_boolean stopping_p;            /* The timer thread is stopping */
    bitd_thread event_loop_th;          /* The timer thread */
    bitd_event event_loop_ev;           /* Wakes up the event loop */
};

struct tth_timer_s {
    bitd_timer timer;
    void *cookie;
    tth_timer_expired_callback_t *callback;
    bitd_int64 tmo_nsec;
    bitd_int64 next_expiration_nsec;
    bitd_boolean periodic_p;
};

/*****************************************************************************
 *                           FUNCTION DECLARATION
 *****************************************************************************/



/*****************************************************************************
 *                                VARIABLES
 *****************************************************************************/
/* The global timer thread control block pointer */
static struct timer_thread_s *g_tth;



/*****************************************************************************
 *                          FUNCTION IMPLEMENTATION
 *****************************************************************************/


/*
 *============================================================================
 *                        tth_timer_expired
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
static void tth_timer_expired(bitd_timer timer, void *cookie) {
    tth_timer_t t = (tth_timer_t)cookie;
    bitd_uint64 current_time, tmo_nsec;

    if (t->periodic_p) {
	/* Update the next expiration */
	t->next_expiration_nsec += t->tmo_nsec;
    }

    /* We are inside the mutex. Call the callback. */
    t->callback(t->cookie);

    if (t->periodic_p) {
	current_time = bitd_get_time_nsec();
	if (current_time >= t->next_expiration_nsec) {
	    /* Next expiration should have already happened */
	    tmo_nsec = 1;
	    t->next_expiration_nsec = current_time;
	} else {
	    tmo_nsec = t->next_expiration_nsec - current_time;
	}

	/* Re-add the timer to the timer list */
	bitd_timer_list_add_nsec(g_tth->timers,
			       t->timer,
			       tmo_nsec,
			       tth_timer_expired,
			       t);
    }
} 


/*
 *============================================================================
 *                        tth_event_loop
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
void tth_event_loop(void *thread_arg) {
    struct bitd_pollfd p;
    int ret;
    bitd_uint32 tmo;
    
    tth_printf("Event loop started\n");

    while (!g_tth->stopping_p) {

	bitd_mutex_lock(g_tth->lock);

	/* Tick the timers */
	bitd_timer_list_tick(g_tth->timers);

	/* Get the list timer expiration */
	tmo = bitd_timer_list_get_timeout_msec(g_tth->timers);

	/* Normalize it */
	tmo = MAX(tmo, 1);
	
	bitd_mutex_unlock(g_tth->lock);

	/* Wait on the stop event */
	memset(&p, 0, sizeof(p));
	p.fd = bitd_event_to_fd(g_tth->event_loop_ev);
	p.events = BITD_POLLIN;

	tth_printf("Event loop poll, tmo %u, stopping %d\n", 
		   tmo, g_tth->stopping_p);

	ret = bitd_poll(&p, 1, tmo);
	if (ret > 0) {
	    if (p.revents & BITD_POLLIN) {
		tth_printf("Wake up event detected\n");
		
		/* Clear the event */
		bitd_event_clear(g_tth->event_loop_ev);
		/* Go back to top of while loop, where we check 
		   if we're stopping */
		continue;
	    }
	}
    }

    tth_printf("Event loop stopped\n");
} 


/*
 *============================================================================
 *                        tth_init
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
bitd_boolean tth_init(void) {

    if (!g_tth) {
	g_tth = calloc(1, sizeof(*g_tth));

	g_tth->lock = bitd_mutex_create();
	g_tth->timers = bitd_timer_list_create();
	bitd_timer_list_set_ticks_max(g_tth->timers, 250);
	g_tth->event_loop_ev = bitd_event_create(BITD_EVENT_FLAG_POLL);

	/* Start the event loop thread */
	g_tth->event_loop_th = bitd_create_thread("timer-thread-event-loop",
						tth_event_loop,
						BITD_DEFAULT_PRIORITY,
						BITD_DEFAULT_STACK_SIZE,
						NULL);
    }

    return TRUE;
} 


/*
 *============================================================================
 *                        tth_deinit
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
void tth_deinit(void) {

    if (g_tth) {
	/* Stop the event loop */
	g_tth->stopping_p = TRUE;
	bitd_event_set(g_tth->event_loop_ev);
	
	/* Wait for event loop to exit */
	bitd_join_thread(g_tth->event_loop_th);

	bitd_event_destroy(g_tth->event_loop_ev);
	bitd_timer_list_destroy(g_tth->timers);
	bitd_mutex_destroy(g_tth->lock);

	free(g_tth);
	g_tth = NULL;
    }
} 


/*
 *============================================================================
 *                        tth_timer_set_msec
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
tth_timer_t tth_timer_set_msec(bitd_uint32 tmo_msec,
			       tth_timer_expired_callback_t *callback,
			       void *cookie,
			       bitd_boolean periodic_p) {
    return tth_timer_set_nsec(tmo_msec * 1000000ULL,
			      callback,
			      cookie,
			      periodic_p);
}


/*
 *============================================================================
 *                        tth_timer_set_nsec
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
tth_timer_t tth_timer_set_nsec(bitd_uint64 tmo_nsec,
			       tth_timer_expired_callback_t *callback,
			       void *cookie,
			       bitd_boolean periodic_p) {
    bitd_boolean wake_up_event_loop_p = FALSE;
    bitd_uint32 tmo_list;
    tth_timer_t t = calloc(1, sizeof(*t));

    /* Create the underlying timer */
    t->timer = bitd_timer_create();

    /* Save the cookie and the callback */
    t->callback = callback;
    t->cookie = cookie;

    t->tmo_nsec = tmo_nsec;
    t->periodic_p = periodic_p;

    if (periodic_p) {
	t->next_expiration_nsec = bitd_get_time_nsec() + tmo_nsec;
    }

    /* We should wake up the event loop if the timer is shorter than
       all other timers */
    tmo_list = bitd_timer_list_get_timeout_msec(g_tth->timers);
    if (tmo_nsec / 1000000ULL < tmo_list) {
	wake_up_event_loop_p = TRUE;
    }

    /* Add the timer to the timer list */
    bitd_timer_list_add_nsec(g_tth->timers,
			   t->timer,
			   tmo_nsec,
			   tth_timer_expired,
			   t);

    if (wake_up_event_loop_p) {
	bitd_event_set(g_tth->event_loop_ev);
    }
    
    return t;
}


/*
 *============================================================================
 *                        tth_timer_destroy
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
void tth_timer_destroy(tth_timer_t t) {

    if (!t) {
	return;
    }

    bitd_mutex_lock(g_tth->lock);

    /* Destroy the timer object, and free the handle */
    bitd_timer_destroy(t->timer);
    free(t);

    bitd_mutex_unlock(g_tth->lock);
}
