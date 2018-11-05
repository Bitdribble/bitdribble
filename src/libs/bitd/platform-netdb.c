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
#include "bitd/platform-netdb.h"
#include "bitd/platform-socket.h"

#if defined(BITD_HAVE_NETDB_H)
# include "netdb.h"
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
 *                        bitd_gethostbyname
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
bitd_hostent bitd_gethostbyname(char const *name) {
#if defined(BITD_HAVE_GETHOSTBYNAME_R)
    char *buf = NULL;
    int buflen = 0;
    int herr;
    int res;
    struct hostent hbuf, *hent;
    bitd_hostent h;
    
    for (;;) {
        if (buf) {
            free(buf);
        }
        buflen += 1024;
        buf = malloc(buflen);
        if (!buf) {
            return NULL;
        }

        res = gethostbyname_r(name, &hbuf, buf, buflen, &hent, &herr);
        if (!res || (errno != ERANGE)) {
            break;
        }        
    }
      
    if (!res) {
        h = bitd_platform_hostent_clone(hent);
    } else {
        h = NULL;
    }

    free(buf);
    return h;

#elif defined(BITD_HAVE_GETHOSTBYNAME)
    struct hostent *h;
    struct bitd_hostent_s h1;
    
    h = gethostbyname(name);
    if (!h) {
      return NULL;
    }

    /* Copy to bitd_hostent_s before cloning, to not get tripped
       by different size of h_addrtype and h_length */
    h1.h_name = (char *)h->h_name;
    h1.h_aliases = h->h_aliases;
    h1.h_addrtype = h->h_addrtype;
    h1.h_length = h->h_length;
    h1.h_addr_list = h->h_addr_list;

    return bitd_hostent_clone(&h1);
#else
# error "gethostbyname() or gethostbyname_r() not defined"
#endif
} 


/*
 *============================================================================
 *                        bitd_gethostbyaddr
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
bitd_hostent bitd_gethostbyaddr(bitd_uint32 addr) {
#if defined(BITD_HAVE_GETHOSTBYADDR_R)
    char *buf = NULL;
    int buflen = 0;
    int herr;
    int res;
    struct hostent hbuf, *hent;
    bitd_hostent h;
    
    for (;;) {
        if (buf) {
            free(buf);
        }
        buflen += 1024;
        buf = malloc(buflen);
        if (!buf) {
            return NULL;
        }
        
        res = gethostbyaddr_r((const char *)&addr, sizeof(addr), AF_INET,
                              &hbuf, buf, buflen, &hent, &herr);
        if (!res || (errno != ERANGE)) {
            break;
        }        
    }
            
    if (!res) {
        h = bitd_hostent_clone((bitd_hostent)hent);
    } else {
        h = NULL;
    }

    free(buf);
    return h;    
#elif defined(BITD_HAVE_GETHOSTBYADDR)
    struct hostent *h;
    struct bitd_hostent_s h1;
    
    h = gethostbyaddr((char const *)&addr, sizeof(addr), AF_INET);
    if (!h) {
      return NULL;
    }

    /* Copy to bitd_hostent_s before cloning, to not get tripped
       by different size of h_addrtype and h_length */
    h1.h_name = (char *)h->h_name;
    h1.h_aliases = h->h_aliases;
    h1.h_addrtype = h->h_addrtype;
    h1.h_length = h->h_length;
    h1.h_addr_list = h->h_addr_list;

    return bitd_hostent_clone(&h1);
#else
# error "gethostbyaddr() or gethostbyaddr_r() not defined"
#endif
} 
