/*****************************************************************************
 *
 * Original Author: Andrei Radulescu-Banu
 * Creation Date:   
 * Description:
 *
 * Copyright (C) 2018 Andrei Radulescu-Banu.  All Rights Reserved.
 * Unauthorized reproduction, modification, distribution, transmission,
 * republication, display or performance are strictly prohibited.
 ****************************************************************************/

/*****************************************************************************
 *                                INCLUDE FILES
 *****************************************************************************/
#include "bitd/common.h"
#include "bitd/platform-poll-event.h"



/*****************************************************************************
 *                             MANIFEST CONSTANTS
 *****************************************************************************/



/*****************************************************************************
 *                                  MACROS
 *****************************************************************************/



/*****************************************************************************
 *                                  TYPES
 *****************************************************************************/

/* Event API prototypes */
typedef void *event_t;
typedef event_t (event_create_t)(void);
typedef void (event_destroy_t)(event_t);
typedef void (event_set_t)(event_t);
typedef void (event_clear_t)(event_t);
typedef bitd_boolean (event_wait_t) (event_t, bitd_uint32);
typedef bitd_socket_t (event_to_fd_t)(event_t);

/* The event control block */
struct bitd_event_s {
    event_t event;
    event_create_t *event_create;
    event_destroy_t *event_destroy;
    event_set_t *event_set;
    event_clear_t *event_clear;
    event_wait_t *event_wait;
    event_to_fd_t *event_to_fd;
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
 *                        bitd_event_create
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
bitd_event bitd_event_create(bitd_uint32 flags) {
    bitd_event ev = calloc(1, sizeof(*ev));

    if (flags & BITD_EVENT_FLAG_POLL) {
        ev->event_create = (event_create_t *)&bitd_poll_event_create;
        ev->event_destroy = (event_destroy_t *)&bitd_poll_event_destroy;
        ev->event_set = (event_set_t *)&bitd_poll_event_set;
        ev->event_clear = (event_clear_t *)&bitd_poll_event_clear;
        ev->event_wait = (event_wait_t *)&bitd_poll_event_wait;
        ev->event_to_fd = (event_to_fd_t *)&bitd_poll_event_to_fd;
    } else {
        ev->event_create = (event_create_t *)&bitd_arch_event_create;
        ev->event_destroy = (event_destroy_t *)&bitd_arch_event_destroy;
        ev->event_set = (event_set_t *)&bitd_arch_event_set;
        ev->event_clear = (event_clear_t *)&bitd_arch_event_clear;
        ev->event_wait = (event_wait_t *)&bitd_arch_event_wait;
        /* Skillping ev->event_to_fd */
    }

    ev->event = ev->event_create();
    if (!ev->event) {
        bitd_event_destroy(ev);
        return NULL;
    }

    return ev;
} 

/*
 *============================================================================
 *                        bitd_event_destroy
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
void bitd_event_destroy(bitd_event ev) {
    if (ev) {
        if (ev->event) {
            ev->event_destroy(ev->event);
        }
        free(ev);
    }
} 


/*
 *============================================================================
 *                        bitd_event_set
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
void bitd_event_set(bitd_event ev) {
    ev->event_set(ev->event);
} 



/*
 *============================================================================
 *                        bitd_event_clear
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
void bitd_event_clear(bitd_event ev) {
    ev->event_clear(ev->event);
} 



/*
 *============================================================================
 *                        bitd_event_wait
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
bitd_boolean bitd_event_wait(bitd_event ev, bitd_uint32 timeout) {
    return ev->event_wait(ev->event, timeout);
} 


/*
 *============================================================================
 *                        bitd_event_to_fd
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
bitd_socket_t bitd_event_to_fd(bitd_event ev) {
    return ev->event_to_fd ? ev->event_to_fd(ev->event) : BITD_INVALID_SOCKID;
} 


