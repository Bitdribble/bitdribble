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

#ifndef _BITD_TSTAMP_H_
#define _BITD_TSTAMP_H_

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
typedef struct ts_socket_s *ts_socket_t; /* The timestamp-capable socket */

/* Flags */
#define TS_FLAG_KERNEL  0x0001 /* Kernel timestamp */
#define TS_FLAG_SW      0x0002 /* Software timestamp */
#define TS_FLAG_DROPPED 0x0004 /* Dropped timestamp */

#define TS_FLAG_ENQ     0x0010 /* Enqueue timestamp */
#define TS_FLAG_SND     0x0020 /* Send timestamp */
#define TS_FLAG_ACK     0x0040 /* Ack timestamp (tcp only) */


/* Modeled on Linux kernel timestamps, but implemented for software
   timestamps also */
typedef struct {
    int flags;         /* TS_FLAG_XXX */
    bitd_uint32 seq;     /* Packet count for datagrams, byte count for streams */
    bitd_uint64 tstamp;  /* The timestamp in ns */
} ts_timestamp_t;

/*****************************************************************************
 *                            FUNCTION DEFINITIONS
 *****************************************************************************/

ts_socket_t ts_socket(int domain, int type, int protocol);
void ts_close(ts_socket_t s);

/* Get the socket id, to poll for socket data */
int ts_get_sock_fd(ts_socket_t s);

/* All socket operations */
#define ts_bind(s,addr,size) \
    bind(ts_get_sock_fd(s),(struct sockaddr * )addr,size)
#define ts_listen(s,backlog) \
    listen(ts_get_sock_fd(s),backlog)
#define ts_getsockname(s,addr,addrlen) \
    getsockname(ts_get_sock_fd(s),(struct sockaddr * )addr,addrlen)
#define ts_getpeername(s,addr,addrlen) \
    getpeername(ts_get_sock_fd(s),(struct sockaddr * )addr,addrlen)
#define ts_shutdown(sk,flag) shutdown(ts_get_sock_fd(s),flag)
    
int ts_connect(ts_socket_t s,
	       struct sockaddr *addr, bitd_socklen_t addrlen);

ts_socket_t ts_accept(ts_socket_t s, 
		      struct sockaddr *addr, bitd_socklen_t *addrlen);

/* If return code is >= 0, the timestamp can be retrieved by 
   ts_get_tstamp_tx() when ts_get_tstamp_fd() is readable or has
   an exception event. */
int ts_send(ts_socket_t s, const void *msg, int len, int flags);
int ts_sendto(ts_socket_t s, const void *msg, int len, int flags,
	      const struct sockaddr *to, int tolen);

/* If return code is >= 0, the timestamp can be retrieved 
   from the ts argument. */
int ts_recv(ts_socket_t sock, void *buf, int len, int flags,
	    ts_timestamp_t *ts);
int ts_recvfrom(ts_socket_t sock, void *buf, int len, int flags,
		struct sockaddr *from, int *fromlen,
		ts_timestamp_t *ts);

/* Get the timestamp file id, to poll for tx timestamps. On some platforms
   this is same as the sock_fd. */
int ts_get_tstamp_fd(ts_socket_t s);

/* Get a transmit timestamp. Returns -1 on failure. */
int ts_get_tstamp_tx(ts_socket_t s, ts_timestamp_t *ts);

/*
 * Get and set socket options. The SOL_TS_SOCKET level is specific 
 * to ts_sockets.
 */
int ts_getsockopt(ts_socket_t s, int level, int optname, void *optval,
		  bitd_socklen_t *optlen);
int ts_setsockopt(ts_socket_t s, int level, int optname, void *optval,
		  bitd_socklen_t optlen);

/* ts_socket options */
#define SOL_TS_SOCKET 0xcddc /* Avoid collision with system socket opts */
#define SO_TS_FLAGS 0x1 /* Get/set TS_FLAG_ENQ/SND/ACK */


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _BITD_TSTAMP_H_ */
