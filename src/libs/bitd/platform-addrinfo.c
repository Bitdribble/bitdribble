/*****************************************************************************
 *
 * Original Author: 
 * Creation Date:   
 * Description: 
 * 
 * Copyright (C) by Andrei Radulescu-Banu, 2018. All Rights Reserved.
 * Unauthorized reproduction, modification, distribution, transmission, 
 * republication, display or performance are strictly prohibited.
 ****************************************************************************/

/*****************************************************************************
 *                                INCLUDE FILES 
 *****************************************************************************/
/* Note that getaddrinfo() and freeaddrinfo() are only supported
   in WindowsXP or later. To have these work, we'd have to set 
   #define WINVER WindowsXP
   at the top of the file. In that case, though, we'd get a runtime symbol
   error for earlier Windowse. Our solution is simply to wrap getaddrinfo()
   into gethostbyname() for Windows. */
 
#include "bitd/common.h"

#if defined(BITD_HAVE_SYS_SOCKET_H)
# include <sys/socket.h>
#endif

#if defined(BITD_HAVE_NETDB_H)
# include <netdb.h>
#endif

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
 *                        bitd_getaddrinfo
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
int bitd_getaddrinfo(const char *node, const char *service,
		     const bitd_addrinfo_t hints,
		     bitd_addrinfo_t *res) {
#if defined(BITD_HAVE_GETADDRINFO)
    int ret;
    struct addrinfo hints1, *res1 = NULL;

    memset(&hints1, 0, sizeof(struct addrinfo));
    if (hints) {
	hints1.ai_family = hints->ai_family;  
	hints1.ai_socktype = hints->ai_socktype; 
	hints1.ai_flags =  hints->ai_flags;   
	hints1.ai_protocol = hints->ai_protocol;
    } else {
	hints1.ai_family = AF_UNSPEC;  
    }

    ret = getaddrinfo(node, service, &hints1, &res1);
    if (!ret) {
	if (res) {
	    *res = bitd_platform_clone_addrinfo(res1);
	}	
    }

    if (res1) {
	freeaddrinfo(res1);
    }

    return ret;
#else
    int ret = -1;
    bitd_addrinfo_t res1 = NULL, res1_prev = NULL;
    bitd_uint32 i;
    bitd_hostent h;

#ifdef _WIN32
    WSASetLastError(0);
#endif

    /* Initialize the OUT parameter */
    if (!res) {
	return -1;
    }
    *res = NULL;

    h = bitd_gethostbyname(node);
    if (h) {
	ret = 0;
	for(i = 0; h->h_addr_list[i]; i++) {
	    res1 = malloc(sizeof(*res1));
	    
	    res1->ai_flags = 0;
	    res1->ai_family = AF_INET;
	    res1->ai_socktype = hints ? hints->ai_socktype : 0;
	    res1->ai_protocol = hints? hints->ai_protocol : 0;
	    res1->ai_addrlen = sizeof(struct sockaddr_in);
	    res1->ai_addr = calloc(1, sizeof(struct sockaddr_storage));

	    /* Copy the address */
	    bitd_set_sin_family((struct sockaddr_storage *)res1->ai_addr, 
			      AF_INET);
	    memcpy(bitd_sin_addr((struct sockaddr_storage *)res1->ai_addr), 
		   h->h_addr_list[i], 
		   bitd_sin_addrlen((struct sockaddr_storage *)res1->ai_addr));

	    /* Copy the canonical name */
	    if (h->h_name) {
		res1->ai_canonname = strdup(h->h_name);
	    } else {
		res1->ai_canonname = NULL;
	    }

	    res1->ai_next = NULL;

	    /* Save the routine result on the 1st pass */
	    if (!*res) {
		*res = res1;
	    }

	    if (res1_prev) {
		res1_prev->ai_next = res1;
	    }
	
	    /* Next time in the loop, res1 is accessed as res1_prev */	
	    res1_prev = res1;
	}
    }

    bitd_hostent_free(h);

    return ret;
#endif
}


/*
 *============================================================================
 *                        bitd_platform_clone_addrinfo
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
bitd_addrinfo_t bitd_platform_clone_addrinfo(struct addrinfo *res) {
    bitd_addrinfo_t res_new = NULL, res1, res1_prev = NULL;

    while (res) {
	res1 = malloc(sizeof(*res1));

	res1->ai_flags = res->ai_flags;
	res1->ai_family = res->ai_family;
	res1->ai_socktype = res->ai_socktype;
	res1->ai_protocol = res->ai_protocol;
	res1->ai_addrlen = res->ai_addrlen;

	if (res->ai_addr) {
	    res1->ai_addr = malloc(sizeof(struct sockaddr_storage));
	    memcpy(res1->ai_addr, res->ai_addr, 
		   bitd_sockaddrlen((struct sockaddr_storage *)res->ai_addr));
	} else {
	    res1->ai_addr = NULL;
	}
	if (res->ai_canonname) {
	    res1->ai_canonname = strdup(res->ai_canonname);
	} else {
	    res1->ai_canonname = NULL;
	}

	res1->ai_next = NULL;

	/* Save the routine result on the 1st pass */
	if (!res_new) {
	    res_new = res1;
	}

	if (res1_prev) {
	    res1_prev->ai_next = res1;
	}
	
	/* Next time in the loop, res1 is accessed as res1_prev */	
	res1_prev = res1;

	res = res->ai_next;
    }

    return res_new;
} 


/*
 *============================================================================
 *                        bitd_clone_addrinfo
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
bitd_addrinfo_t bitd_clone_addrinfo(bitd_addrinfo_t res) {
    bitd_addrinfo_t res_new = NULL, res1, res1_prev = NULL;

    while (res) {
	res1 = malloc(sizeof(*res));

	res1->ai_flags = res->ai_flags;
	res1->ai_socktype = res->ai_socktype;
	res1->ai_protocol = res->ai_protocol;
	res1->ai_addrlen = res->ai_addrlen;

	if (res->ai_addr) {
	    res1->ai_addr = malloc(sizeof(struct sockaddr_storage));
	    memcpy(res1->ai_addr, res->ai_addr, 
		   bitd_sockaddrlen((struct sockaddr_storage *)res->ai_addr));
	} else {
	    res1->ai_addr = NULL;
	}
	if (res->ai_canonname) {
	    res1->ai_canonname = strdup(res->ai_canonname);
	} else {
	    res1->ai_canonname = NULL;
	}

	res1->ai_next = NULL;

	/* Save the routine result on the 1st pass */
	if (!res_new) {
	    res_new = res1;
	}

	if (res1_prev) {
	    res1_prev->ai_next = res1;
	}
	
	/* Next time in the loop, res1 is accessed as res1_prev */	
	res1_prev = res1;

	res = res->ai_next;
    }

    return res_new;
} 


/*
 *============================================================================
 *                        bitd_freeaddrinfo
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
void bitd_freeaddrinfo(bitd_addrinfo_t res) {
    bitd_addrinfo_t res_next;

    while (res) {
	res_next = res->ai_next;
	if (res->ai_addr) {
	    free(res->ai_addr);
	}
	if (res->ai_canonname) {
	    free(res->ai_canonname);
	}
	free(res);
	res = res_next;
    }
} 


/*
 *============================================================================
 *                        bitd_gai_strerror
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
const char *bitd_gai_strerror(int errcode) {
#if defined(BITD_HAVE_GAI_STRERROR)
    return gai_strerror(errcode);
#elif defined(_WIN32)
    if (errcode == -1) {
	DWORD dwError;

	dwError = WSAGetLastError();
	if (dwError) {
            if (dwError == WSAHOST_NOT_FOUND) {
                return "host not found";
            } else if (dwError == WSANO_DATA) {
                return "no data record found";
            } else {
                return "resolver error";
	    }
	}
    }
    return NULL;
#else
# error "gai_strerror() or WSAGetLastError() not defined"
    bitd_assert(0);
    return NULL;
#endif
}

/*
 *============================================================================
 *                        bitd_getnameinfo
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
int bitd_getnameinfo(const struct sockaddr *sa, int salen,
		     char *host, int hostlen,
		     char *serv, int servlen, int flags) {
#if defined(BITD_HAVE_GETNAMEINFO)
    return getnameinfo(sa, salen, host, hostlen, serv, servlen, flags);
#else
    int ret = -1;
    bitd_hostent h;
    bitd_uint16 af;
    bitd_uint32 addr;

    /* Initialize OUT parameters */
    if (host && hostlen) {
	host[0] = 0;
    }
    if (serv && servlen) {
	serv[0] = 0;
    }

#ifdef _WIN32
    WSASetLastError(0);
#endif

    af = bitd_sin_family((struct sockaddr_storage *)sa);
    if (af != 0 && af != AF_INET) {
	/* Address family not supported */
	return -1;
    }

    addr = ((struct sockaddr_in *)sa)->sin_addr.s_addr;
    h = bitd_gethostbyaddr(addr);
    if (h) {
	if (h->h_name) {
	    int len1 = strlen(h->h_name) + 1;
	    int len = hostlen - 1;

	    if (len > len1) {
		len = len1;
	    }

	    memcpy(host, h->h_name, len);
	    if (len < len1) {
		/* Partial memcpy - set NULL termination */
		host[len+1] = 0;
	    }

	    ret = 0;
	}

	/* Release resources */
	bitd_hostent_free(h);	
    }

    return ret;
#endif
} 


/*
 *============================================================================
 *                        bitd_resolve_hostport
 *============================================================================
 * Description:     Resolve hostport in host:port or [host]:port format 
 *     into numeric address, using getaddrinfo()
 * Parameters:    
 *     sa [OUT] - the resolved address
 *     salen - the size of the allocate sa structure
 *     hostport - the input address & port in host:port or [host]:port format
 * Returns:  
 *     0 on success, non-zero on error. Use bitd_gai_strerror() to get a string
 *         explanation of the return code error.
 */
int bitd_resolve_hostport(struct sockaddr_storage *sa, int salen,
			  char *hostport) {
    int ret = -1;
    char *hostname = NULL;
    char *c, *c1, *c2;
    int port = 0;
    struct sockaddr_storage sock_addr;
    bitd_boolean numeric = FALSE;
    
    /* Parameter check */
    if (!sa || !salen || !hostport) {
	return -1;
    }
    
    memset(&sock_addr, 0, sizeof(sock_addr));

    /* Make a copy of the hostport */
    hostport = strdup(hostport);

    /* Look for square bracket enclosure */
    c = strchr(hostport, '[');
    c1 = strchr(hostport, ']');
    if (c && c1 && c1 > c) {
	*c1++ = 0;
	hostname = strdup(c+1);
	
	if (*c1 == ':') {
	    port = atoi(c1+1);
	}
    } else {
	/* No brackets - is this a numeric ip6 address? */
	bitd_set_sin_family(&sock_addr, AF_INET6);
	if (inet_pton(AF_INET6, hostport, bitd_sin_addr(&sock_addr))) {
	    /* Yes it is */
	    hostname = hostport;
	    hostport = NULL;
	    /* Remember this as a numeric address */
	    numeric = TRUE;
	    ret = 0;
	} else {
	    /* Clear the address family */
	    bitd_set_sin_family(&sock_addr, 0);
	    /* Look for ':port' */
	    c2 = strchr(hostport, ':');
	    if (c2) {
		*c2++ = 0;
		hostname = hostport;
		hostport = NULL;
		port = atoi(c2);
	    } else {
		hostname = hostport;
		hostport = NULL;
	    }
	}
    }
    
    /* At this point, we have the hostname and the port. Find out if the host
       is numeric or needs to be resolved */
    if (!numeric) {
	bitd_set_sin_family(&sock_addr, AF_INET6);
    
	if (bitd_inet_pton(AF_INET6, hostname, 
			 bitd_sin_addr(&sock_addr)) > 0) {
	    numeric = TRUE;		
	    ret = 0;
	} else {
	    bitd_set_sin_family(&sock_addr, AF_INET);
	    if (bitd_inet_pton(AF_INET, hostname, 
			     bitd_sin_addr(&sock_addr)) > 0) {
		numeric = TRUE;
		ret = 0;
	    }
	}
    }
    
    if (!numeric) {
	/* Clear the address damily */
	bitd_set_sin_family(&sock_addr, 0);

	/* Perform DNS resolution */
	bitd_addrinfo_t a = NULL;

	ret = bitd_getaddrinfo(hostname, NULL, NULL, &a);
	if (!ret) {
	    if (a) {
		bitd_set_sin_family(&sock_addr, a->ai_family);	
		memcpy(bitd_sin_addr(&sock_addr), 
		       bitd_sin_addr((struct sockaddr_storage *)a->ai_addr),
		       a->ai_addrlen);
		ret = 0;
	    }
	}
	bitd_freeaddrinfo(a);
    }

    if (!ret) {
	/* Copy the port */
	*bitd_sin_port(&sock_addr) = htons(port);

	/* Copy the address */
	if (salen < bitd_sockaddrlen(&sock_addr)) {
	    /* Passed-in address is too short */
	    ret = -1;
	} else {
	    salen = bitd_sockaddrlen(&sock_addr);

	    memcpy(sa, &sock_addr, salen);
	}
    }
    
    if (hostport) {
	free(hostport);
    }
    if (hostname) {
	free(hostname);
    }

    return ret;
} 
