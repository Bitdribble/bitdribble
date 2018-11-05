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

#include "bitd/common.h"

#if defined(BITD_HAVE_POLL_H) && defined(BITD_HAVE_POLL)
# include <poll.h>
#endif

/*****************************************************************************
 *                             MANIFEST CONSTANTS
 *****************************************************************************/



/*****************************************************************************
 *                                  MACROS 
 *****************************************************************************/
/* #define _XDEBUG */

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
 *                        bitd_sys_socket_init
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
bitd_boolean bitd_sys_socket_init(void) {
#if defined(_WIN32)
    WSADATA d; 
    return WSAStartup(MAKEWORD(2, 0), &d) ? FALSE : TRUE;
#else
    return TRUE;
#endif
} 



/*
 *============================================================================
 *                        bitd_sys_socket_deinit
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
void bitd_sys_socket_deinit(void) {
#if defined(_WIN32)
    WSACleanup();
#endif
} 


/*
 *============================================================================
 *                        bitd_select
 *============================================================================
 * Description:     Portable version of Linux select()
 * Parameters:    
 * Returns:  
 */
int bitd_select(bitd_uint32 n_sockets,
		bitd_socket_t *r, bitd_socket_t *w, bitd_socket_t *e,  
		bitd_int32 msec_tmo) {
    fd_set rfds;
    fd_set wfds;
    fd_set efds;
    struct timeval timeval_tmo;
    bitd_uint32 i;
    int ret;
    
    /* Number of socks on Windows, highest sock on Linux */
    bitd_socket_t hsock = n_sockets;

#if !defined(_WIN32)
    hsock = (bitd_socket_t)-1;
#endif
    
    if (r) {
        FD_ZERO(&rfds);
        for (i = 0 ; i < n_sockets ; i++) {
            if (r[i] != BITD_INVALID_SOCKID) {
                FD_SET(r[i], &rfds);
#if !defined(_WIN32)
                hsock = MAX(hsock, r[i] + 1);
#endif
            }
        }
    }
    
    if (w) {
        FD_ZERO(&wfds);
        for (i = 0 ; i < n_sockets ; i++) {
            if (w[i] != BITD_INVALID_SOCKID) {
                FD_SET(w[i], &wfds);
#if !defined(_WIN32)
                hsock = MAX(hsock, w[i] + 1);
#endif
            }
        }
    }
    
    if (e) {
        FD_ZERO(&efds);
        for (i = 0 ; i < n_sockets ; i++) {
            if (e[i] != BITD_INVALID_SOCKID) {
                FD_SET(e[i], &efds);
#if !defined(_WIN32)
                hsock = MAX(hsock, e[i] + 1);
#endif
            }
        }
    }
    
    if (msec_tmo != -1) {
        timeval_tmo.tv_sec = msec_tmo / 1000;
        timeval_tmo.tv_usec = (msec_tmo % 1000) * 1000;
    } else {
	/* Keep valgrind happy */
        timeval_tmo.tv_sec = 0;
        timeval_tmo.tv_usec = 0;
    }
    
#ifdef _XDEBUG
    {
        unsigned i;
        
        printf("select(");
        if (r) {
            printf("[");
            for (i = 0 ; i < n_sockets ; i++) {
                printf("%d ", r[i]);
            }
            printf("]");
        } else {
            printf("NULL");
        }
        printf(", ");
        
        if (w) {
            printf("[");
            for (i = 0 ; i < n_sockets ; i++) {
                printf("%d ", w[i]);
            }
            printf("]");
        } else {
            printf("NULL");
        }
        printf(", ");
        
        if (e) {
            printf("[");
            for (i = 0 ; i < n_sockets ; i++) {
                printf("%d ", e[i]);
                printf("]");
            }
        } else {
            printf("NULL");
        }
        printf(", ");
        
        printf("{%d}, %d)\n", hsock, msec_tmo);
    }
#endif

    ret = select(hsock,
                 r ? &rfds : NULL,
                 w ? &wfds : NULL,
                 e ? &efds : NULL,
                 msec_tmo == (bitd_int32)BITD_FOREVER ? NULL : &timeval_tmo);

    if (ret > 0) {
        if (r) {
            for (i = 0 ; i < n_sockets ; i++) {
                if (r[i] != BITD_INVALID_SOCKID && !FD_ISSET(r[i], &rfds)) {
                    r[i] = BITD_INVALID_SOCKID;
                }
            }
        }
        if (w) {
            for (i = 0 ; i < n_sockets ; i++) {
                if (w[i] != BITD_INVALID_SOCKID && !FD_ISSET(w[i], &wfds)) {
                    w[i] = BITD_INVALID_SOCKID;
                }
            }
        }
        if (e) {
            for (i = 0 ; i < n_sockets ; i++) {
                if (e[i] != BITD_INVALID_SOCKID && !FD_ISSET(e[i], &efds)) {
                    e[i] = BITD_INVALID_SOCKID;
                }
            }
        }
    }

#ifdef _XDEBUG
    printf("select(");
    
    if (ret > 0) {
	unsigned i;
      
	if (r) {
	    printf("[");
	    for (i = 0 ; i < n_sockets ; i++) {
		printf("%d ", r[i]);
	    }
	    printf("]");
	} else {
	    printf("NULL");
	}
	printf(", ");
      
	if (w) {
	    printf("[");
	    for ( i = 0 ; i < n_sockets ; i++) {
		printf("%d ", w[i]);
	    }
	    printf("]");
	} else {
	    printf("NULL");
	}
	printf(", ");
      
	if (e) {
	    printf("[");
	    for ( i = 0 ; i < n_sockets ; i++) {
		printf("%d ", e[i]);
	    }
	    printf("]");
	} else {
	    printf("NULL");
	}
    }  
    printf(") ret %d (errno %d)\n", ret, bitd_socket_errno);
#endif

    if (ret == -1) {
	return BITD_SOCKET_ERROR;
    }

    return ret;
}


#if defined(BITD_HAVE_POLL_H) && defined(BITD_HAVE_POLL)
/*
 *============================================================================
 *                        bitd_poll
 *============================================================================
 * Description:  Wrapper for the POSIX version of poll() - available on some
 *     platforms but not all. The poll() API is executed very frequently,
 *     and it should be optimized as much as possible.
 * Parameters:
 * Returns:
 *     Number of descriptors with events, or 0 on timeout 
 */
int bitd_poll(struct bitd_pollfd *pfds, bitd_uint32 nfds, int msec_tmo) {
    return poll((struct pollfd *)pfds, (nfds_t)nfds, msec_tmo);
}
#endif

#if !defined(BITD_HAVE_POLL_H) || !defined(BITD_HAVE_POLL)
/*
 *============================================================================
 *                        bitd_poll
 *============================================================================
 * Description:  Portable version of Linux poll() on top of select().
 *     This is needed when the platform does not support POSIX poll().
 * Parameters:
 * Returns:
 *     Number of descriptors with events, or 0 on timeout 
 */
int bitd_poll(struct bitd_pollfd *pfds, bitd_uint32 nfds, int msec_tmo) {
    bitd_socket_t *r = NULL;
    bitd_socket_t *w = NULL;
    bitd_socket_t *e = NULL;
    bitd_uint32 ret, i;
    bitd_socket_t sr[3], sw[3], se[3];

    /* Normalize the msec_tmo input - convert negative values to BITD_FOREVER */
    if (msec_tmo < 0) {
	msec_tmo = BITD_FOREVER;
    }

    if (!pfds || !nfds) {
        bitd_sleep(msec_tmo);
        return 0;
    }

    if (nfds <= 3) {
	/* Stack allocated nfds, to avoid too many malloc()s  */
	r = &sr[0];
	w = &sw[0];
	e = &se[0];
    } else {
	/* Heap allocated nfds */
	r = malloc(nfds * sizeof(*r));
	w = malloc(nfds * sizeof(*w));
	e = malloc(nfds * sizeof(*e));
    }

    for (i = 0; i < nfds; i++) {
        if (pfds[i].events & BITD_POLLIN) {
            r[i] = pfds[i].fd;
        } else {
            r[i] = BITD_INVALID_SOCKID;
        }
        if (pfds[i].events & BITD_POLLOUT) {
            w[i] = pfds[i].fd;
        } else {
            w[i] = BITD_INVALID_SOCKID;
        }
        if (pfds[i].events & BITD_POLLERR) {
            e[i] = pfds[i].fd;
        } else {
            e[i] = BITD_INVALID_SOCKID;
        }
    }

#ifdef _XDEBUG
    printf("poll(");
    for (i = 0; i < nfds; i++) {
	printf("[%d", pfds[i].fd);
	if (pfds[i].events & BITD_POLLIN) {
	    printf(" in");
	}
	if (pfds[i].events & BITD_POLLOUT) {
	    printf(" out");
	}
	if (pfds[i].events & BITD_POLLERR) {
	    printf(" err");
	}
	printf("] ");
    }
    printf(", %d)\n", msec_tmo);
#endif

    /* Call select() to do the actual job */
    ret = bitd_select(nfds, r, w, e, msec_tmo);
    for (i = 0; i < nfds; i++) {
        pfds[i].revents = 0;
        if (r[i] != BITD_INVALID_SOCKID) {
            pfds[i].revents |= BITD_POLLIN;
        }
        if (w[i] != BITD_INVALID_SOCKID) {
            pfds[i].revents |= BITD_POLLOUT;
        }
        if (e[i] != BITD_INVALID_SOCKID) {
            pfds[i].revents |= BITD_POLLERR;
        }
    }

#ifdef _XDEBUG
    printf("poll(");
    for (i = 0; i < nfds; i++) {
	printf("[%d", pfds[i].fd);
	if (pfds[i].revents & BITD_POLLIN) {
	    printf(" in");
	}
	if (pfds[i].revents & BITD_POLLOUT) {
	    printf(" out");
	}
	if (pfds[i].revents & BITD_POLLERR) {
	    printf(" err");
	}
	printf("] ");
    }
    printf(") ret %d (errno %d)\n", ret, bitd_socket_errno);
#endif

    if (nfds > 3) {
	free(r);
	free(w);
	free(e);
    }

    return ret;
}
#endif


/*
 *============================================================================
 *                        bitd_set_blocking
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
int bitd_set_blocking(bitd_socket_t s, bitd_boolean is_blocking) {
#if defined(_WIN32)
    u_long flags;

    if (is_blocking) {
        flags = 0;
    } else {
        flags = 1;
    }

    return ioctlsocket(s, FIONBIO, &flags);
#else
    int flags;

    flags = fcntl(s, F_GETFL);
    if (flags == -1) {
        return BITD_SOCKET_ERROR;
    }

    if (is_blocking) {
        flags &= ~(O_NONBLOCK);
    } else {
        flags |= O_NONBLOCK;
    }

    return fcntl(s, F_SETFL, flags);
#endif
} 
