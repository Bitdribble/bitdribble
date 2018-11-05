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

#ifndef _BITD_PLATFORM_NETDB_H_
#define _BITD_PLATFORM_NETDB_H_

/*****************************************************************************
 *                                INCLUDE FILES 
 *****************************************************************************/

#include "bitd/platform-types.h"


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

/* Portable hostent definition */
typedef struct bitd_hostent_s {
    char    *h_name;        /* official name of host */
    char    **h_aliases;    /* alias list */
    int     h_addrtype;     /* host address type */
    int     h_length;       /* length of address */
    char    **h_addr_list;  /* list of addresses */
} *bitd_hostent;

#define h_addr  h_addr_list[0]  /* for backward compatibility */

/* Forward declaration of the platform-specific type */
struct hostent;

/* Portable addrinfo definition */
typedef struct bitd_addrinfo_s {
    int ai_flags;
    int ai_family;
    int ai_socktype;
    int ai_protocol;
    int ai_addrlen;
    struct sockaddr *ai_addr;
    char *ai_canonname;             /* official name of host */
    struct bitd_addrinfo_s *ai_next;
} *bitd_addrinfo_t;

/* Forward declaration of the platform-specific type */
struct addrinfo;

/*****************************************************************************
 *                            FUNCTION DEFINITIONS
 *****************************************************************************/

/* Portable gethostbyname/addr APIs */
bitd_hostent bitd_gethostbyname(char const *name);
bitd_hostent bitd_gethostbyaddr(bitd_uint32 addr);

/* Use this API to clone hostent structures */
bitd_hostent bitd_hostent_clone(bitd_hostent h);
bitd_hostent bitd_platform_hostent_clone(struct hostent *h);

/* Use this API to free the returned structure from gethostbyname/addr */
void bitd_hostent_free(bitd_hostent h);

/* Portable getaddrinfo APIs */
int bitd_getaddrinfo(const char *node, const char *service,
		     const bitd_addrinfo_t hints,
		     bitd_addrinfo_t *res);
bitd_addrinfo_t bitd_clone_addrinfo(bitd_addrinfo_t res);
bitd_addrinfo_t bitd_platform_clone_addrinfo(struct addrinfo *res);
void bitd_freeaddrinfo(bitd_addrinfo_t res);
const char *bitd_gai_strerror(int errcode);

int bitd_getnameinfo(const struct sockaddr *sa, int salen,
		     char *host, int hostlen,
		     char *serv, int servlen, int flags);

struct sockaddr_storage;

/* Resolve hostport in host:port or [host]:port format into numeric address */
int bitd_resolve_hostport(struct sockaddr_storage *sa, int salen,
			  char *hostport);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _BITD_PLATFORM_NETDB_H_ */
