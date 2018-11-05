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
#define lcl_printf if(0) printf


#define SOCK_NOERROR(s) \
    { \
      int ret = (s); \
      if (ret < 0) { \
        lcl_printf("Unexpected return code %d, (%s %d) %s:%d\n", \
                   ret, strerror(bitd_socket_errno), bitd_socket_errno, __FILE__, __LINE__); \
        goto end; \
      } \
    }




/*****************************************************************************
 *                                  TYPES
 *****************************************************************************/

/* The poll event control block */
struct bitd_poll_event_s {
    bitd_mutex m;
    bitd_socket_t s;
    bitd_boolean event_set;
};




/*****************************************************************************
 *                           FUNCTION DECLARATION
 *****************************************************************************/
static bitd_boolean sock_recv(bitd_poll_event e);



/*****************************************************************************
 *                                VARIABLES
 *****************************************************************************/



/*****************************************************************************
 *                          FUNCTION IMPLEMENTATION
 *****************************************************************************/

/*
 *============================================================================
 *                        sock_receive
 *============================================================================
 * Description:     Flush the socket rx queues
 * Parameters:    
 * Returns:         FALSE if no data was received
 */
bitd_boolean sock_recv(bitd_poll_event e) {
    bitd_boolean ret = FALSE;
    char c;

    /* Go to non-blocking mode */
    bitd_set_blocking(e->s, FALSE);
       
    while (bitd_recv(e->s, &c, sizeof(c), 0) >= 0) {
        ret = TRUE;
    }
    
    /* Go back to blocking mode */
    bitd_set_blocking(e->s, TRUE);
        
    return ret;
} 


/*
 *============================================================================
 *                        bitd_poll_event_to_fd
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
bitd_socket_t bitd_poll_event_to_fd(bitd_poll_event e) {
    return e ? e->s : 0;
} 

/*
 *============================================================================
 *                        bitd_poll_event_create
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
bitd_poll_event bitd_poll_event_create(void) {
    bitd_poll_event e;
    int ret = 0;
    struct sockaddr_in laddr;
    bitd_socklen_t addrlen;

    e = (bitd_poll_event)calloc(1, sizeof(*e));
    if (e) {
        /* Create its mutex */
        e->m = bitd_mutex_create();
        if (!e->m) {
            ret = -1;
            goto end;
        }

        /* Create the socket, bind it to the loopback address
           and connect it to itself */
        SOCK_NOERROR(e->s = bitd_socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP));

        laddr.sin_family = AF_INET;
        laddr.sin_port = 0;
        laddr.sin_addr.s_addr = htonl(0x7f000001);
        SOCK_NOERROR(bitd_bind(e->s, &laddr, sizeof(laddr)));

        addrlen = sizeof(laddr);
        SOCK_NOERROR(bitd_getsockname(e->s, 
                                    (struct sockaddr *)&laddr, &addrlen));

        SOCK_NOERROR(bitd_connect(e->s, &laddr, sizeof(laddr)));
    } else {
        ret = -1;
    }

end:
    if (ret < 0) {
        /* Release whatever we've allocated */
        bitd_poll_event_destroy(e);
        e = NULL;
    }

    return e;
} 

/*
 *============================================================================
 *                        bitd_event_destroy
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
void bitd_poll_event_destroy(bitd_poll_event e) {
    if (e) {
        bitd_close(e->s);
        if (e->m) {
            bitd_mutex_destroy(e->m);
        }
        free(e);
    }
} 



/*
 *============================================================================
 *                        bitd_poll_event_set
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
void bitd_poll_event_set(bitd_poll_event e) {
    char c;

    /* 
       The event is set if and only if ev->event_set is TRUE.
       
       While the event is set, only one UDP packet has been sent, and
       it has already been read out of the socket or is still
       lingering in the IP stack. 
    */

    if (e) {
        bitd_mutex_lock(e->m);

        if (!e->event_set) {
            e->event_set = TRUE;
            c = 0;
            bitd_send(e->s, &c, sizeof(c), 0);
        }        

        bitd_mutex_unlock(e->m);
    }
} 


/*
 *============================================================================
 *                        bitd_poll_event_clear
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
void bitd_poll_event_clear(bitd_poll_event e) {

    if (e) {
        bitd_mutex_lock(e->m);

        if (e->event_set) {
            if (!sock_recv(e)) {
                bitd_poll_event_wait(e, BITD_FOREVER);
            }

            /* Clear the event flag only after all data has been
               dequeued from the socket */
            e->event_set = FALSE;
        }
        
        bitd_mutex_unlock(e->m);
    }
} 


/*
 *============================================================================
 *                        bitd_poll_event_wait
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
bitd_boolean bitd_poll_event_wait(bitd_poll_event e, bitd_uint32 tmo) {
    bitd_boolean ret = FALSE;
    bitd_uint32 start_time;
    bitd_uint32 current_time;
    bitd_socket_t rfd;
    int n;

    if (e) {        
        start_time = bitd_get_time_msec();
        
        for (;;) {
            rfd = e->s;
            n = bitd_select(1, &rfd, NULL, NULL, tmo);
            
            bitd_mutex_lock(e->m);            
            if (n == 1) {           
                /* Socket has data */
                if (e->event_set) {
                    sock_recv(e);
                    e->event_set = FALSE;
                    bitd_mutex_unlock(e->m);
                    ret = TRUE;
                    break;
                }
            }
            bitd_mutex_unlock(e->m);
            
            if (tmo != BITD_FOREVER) {
                /* Compute the remaining time */
                current_time = bitd_get_time_msec();
                tmo -= (current_time - start_time);

                if ((int)tmo <= 0) {
                    break;
                }
            }
        }
    }

    return ret;
} 


