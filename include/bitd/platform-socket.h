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

#ifndef _BITD_PLATFORM_SOCKET_H_
#define _BITD_PLATFORM_SOCKET_H_

/*****************************************************************************
 *                                INCLUDE FILES 
 *****************************************************************************/

#include "bitd/platform-types.h"

#ifdef BITD_HAVE_SYS_TIME_H
# include <sys/time.h> /* for struct timeval on linux */
#endif

#ifdef BITD_HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#endif

#ifdef BITD_HAVE_NETINET_IN_H
# include <netinet/in.h>
#endif

#ifdef BITD_HAVE_ARPA_INET_H
# include <arpa/inet.h>
#endif

#ifdef BITD_HAVE_NETDB_H
# include <netdb.h>
#endif

#ifdef BITD_HAVE_FCNTL_H
# include <fcntl.h>
#endif

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

/*
 * Windows platform definitions
 */
#if defined(_WIN32)

typedef SOCKET bitd_socket_t;
typedef int    bitd_socklen_t;

#define bitd_close closesocket

#define bitd_socket_errno WSAGetLastError()
#define BITD_EAGAIN EAGAIN
#define BITD_EWOULDBLOCK WSAEWOULDBLOCK
#define BITD_EINPROGRESS WSAEINPROGRESS
#define BITD_EADDRINUSE EADDRINUSE
#define BITD_ECONNREFUSED WSAECONNREFUSED

#define BITD_INVALID_SOCKID 0

#endif /* _WIN32 */

/*
 * Unix and OSX platform definitions
 */
#if defined(__unix__) || (defined(__APPLE__) && defined(__MACH__))

typedef int bitd_socket_t;
typedef socklen_t bitd_socklen_t;

#define bitd_close close

#define bitd_socket_errno errno
#define BITD_EAGAIN EAGAIN
#define BITD_EWOULDBLOCK EWOULDBLOCK
#define BITD_EINPROGRESS EINPROGRESS
#define BITD_EADDRINUSE EADDRINUSE
#define BITD_ECONNREFUSED ECONNREFUSED

#define BITD_INVALID_SOCKID -1
#endif /* __unix__ */

/*
 * Platform-independent definitions
 */
#define BITD_SOCKET_ERROR -1

bitd_boolean bitd_sys_socket_init(void);
void bitd_sys_socket_deinit(void);

#define bitd_socket socket
#define bitd_bind(socket,address,size) bind(socket,(struct sockaddr *)address,size)
#define bitd_connect(socket,addr,addr_len) connect(socket,(struct sockaddr *)addr, addr_len)
#define bitd_accept(socket,addr,addr_len) accept(socket,(struct sockaddr *)addr, addr_len)
#define bitd_listen listen
#define bitd_send send
#define bitd_recv recv
#define bitd_sendto(s,b,n,f,addr,addr_len) sendto(s,b,n,f,(struct sockaddr *)addr,addr_len)
#define bitd_recvfrom(s,b,n,f,addr,addr_len) recvfrom(s,b,n,f,(struct sockaddr *)addr,addr_len)
#define bitd_setsockopt setsockopt
#define bitd_getsockopt getsockopt
#define bitd_getsockname(s,addr,addr_len) getsockname(s,(struct sockaddr *)addr,addr_len)

extern int bitd_select(bitd_uint32 n_sockets,
		       bitd_socket_t *r, bitd_socket_t *w, bitd_socket_t *e, 
		       bitd_int32 msec_timeout);
    
struct bitd_pollfd {
    bitd_socket_t fd;  /* file descriptor */
    short events;    /* requested events */
    short revents;   /* returned events */
};

#define BITD_POLLIN  0x0001  /* read data available */
#define BITD_POLLOUT 0x0004  /* writing will not block */
#define BITD_POLLERR 0x0008  /* error condition */
#define BITD_POLLNVAL 0x0020 /* invalid file descriptor */

extern int bitd_poll(struct bitd_pollfd *pfds, bitd_uint32 nfds, int tmo);

extern int bitd_set_blocking(bitd_socket_t s, bitd_boolean is_blocking);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _BITD_PLATFORM_SOCKET_H_ */
