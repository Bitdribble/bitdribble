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
#include "bitd/platform-socket.h"
#include "bitd/platform-inetutil.h"


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
 *                        bitd_strlcpy
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
bitd_uint32 bitd_strlcpy(char *dst, const char *src, bitd_uint32 siz) {
    char *d = dst;
    const char *s = src;
    bitd_uint32 n = siz;

    /* Copy as many bytes as will fit */
    if (n != 0) {
	while (--n != 0) {
	    if ((*d++ = *s++) == '\0')
		break;
	}
    }
    
    /* Not enough room in dst, add NUL and traverse rest of src */
    if (n == 0) {
	if (siz != 0)
	    *d = '\0';		/* NUL-terminate dst */
	while (*s++)
	    ;
    }
    
    return(s - src - 1);	/* count does not include NUL */
}


/*
 *============================================================================
 *                        bitd_inet_ntop4
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
char *bitd_inet_ntop4(const unsigned char *src, char *dst, bitd_uint32 size) {
    static const char fmt[128] = "%u.%u.%u.%u";
    char tmp[sizeof "255.255.255.255"];
    int l;
    
    l = snprintf(tmp, sizeof(tmp), fmt, src[0], src[1], src[2], src[3]); // ****
    if (l <= 0 || (bitd_uint32) l >= size) {
	return (NULL);
    }
    bitd_strlcpy(dst, tmp, size);
    return (dst);
}


/*
 *============================================================================
 *                        bitd_inet_ntop6
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
char *bitd_inet_ntop6(const unsigned char *src, char *dst, bitd_uint32 size) {
    /*
     * Note that int32_t and int16_t need only be "at least" large enough
     * to contain a value of the specified size.  On some systems, like
     * Crays, there is no such thing as an integer variable with 16 bits.
     * Keep this in mind if you think this function should have been coded
     * to use pointer overlays.  All the world's not a VAX.
     */
    char tmp[sizeof "ffff:ffff:ffff:ffff:ffff:ffff:255.255.255.255"], *tp;
    struct { int base, len; } best, cur;
#define NS_IN6ADDRSZ	16
#define NS_INT16SZ	2
    unsigned int words[NS_IN6ADDRSZ / NS_INT16SZ];
    int i;
    
    /*
     * Preprocess:
     *	Copy the input (bytewise) array into a wordwise array.
     *	Find the longest run of 0x00's in src[] for :: shorthanding.
     */
    memset(words, '\0', sizeof words);
    for (i = 0; i < NS_IN6ADDRSZ; i++)
	words[i / 2] |= (src[i] << ((1 - (i % 2)) << 3));
    best.base = -1;
    best.len = 0;
    cur.base = -1;
    cur.len = 0;
    for (i = 0; i < (NS_IN6ADDRSZ / NS_INT16SZ); i++) {
	if (words[i] == 0) {
	    if (cur.base == -1)
		cur.base = i, cur.len = 1;
	    else
		cur.len++;
	} else {
	    if (cur.base != -1) {
		if (best.base == -1 || cur.len > best.len)
		    best = cur;
		cur.base = -1;
	    }
	}
    }
    if (cur.base != -1) {
	if (best.base == -1 || cur.len > best.len)
	    best = cur;
    }
    if (best.base != -1 && best.len < 2)
	best.base = -1;
    
    /*
     * Format the result.
     */
    tp = tmp;
    for (i = 0; i < (NS_IN6ADDRSZ / NS_INT16SZ); i++) {
	/* Are we inside the best run of 0x00's? */
	if (best.base != -1 && i >= best.base &&
	    i < (best.base + best.len)) {
	    if (i == best.base)
		*tp++ = ':';
	    continue;
	}
	/* Are we following an initial run of 0x00s or any real hex? */
	if (i != 0)
	    *tp++ = ':';
	/* Is this address an encapsulated IPv4? */
	if (i == 6 && best.base == 0 && (best.len == 6 ||
					 (best.len == 7 && words[7] != 0x0001) ||
					 (best.len == 5 && words[5] == 0xffff))) {
	    if (!bitd_inet_ntop4(src+12, tp, sizeof tmp - (tp - tmp)))
		return (NULL);
	    tp += strlen(tp);
	    break;
	}
	tp += sprintf(tp, "%x", words[i]); // ****
    }
    /* Was it a trailing run of 0x00's? */
    if (best.base != -1 && (best.base + best.len) == 
	(NS_IN6ADDRSZ / NS_INT16SZ))
	*tp++ = ':';
    *tp++ = '\0';
    
    /*
     * Check for overflow, copy, and we're done.
     */
    if ((bitd_uint32)(tp - tmp) > size) {
	return (NULL);
    }
    strcpy(dst, tmp);
    return (dst);
}


/*
 *============================================================================
 *                        bitd_inet_ntop
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
const char *bitd_inet_ntop(int af, const void *src, char *dst, bitd_uint32 size) {
    switch (af) {
    case AF_INET:
	return (bitd_inet_ntop4( (unsigned char*)src, (char*)dst, size)); // ****
    case AF_INET6:
	return (char*)(bitd_inet_ntop6( (unsigned char*)src, (char*)dst, size)); // ****
    default:
        return 0;
    }
}


/*
 *============================================================================
 *                        bitd_inet_pton4
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
int bitd_inet_pton4(const char *src, unsigned char *dst) {
    static const char digits[] = "0123456789";
    int saw_digit, octets, ch;
#define NS_INADDRSZ	4
    unsigned char tmp[NS_INADDRSZ], *tp;
    
    saw_digit = 0;
    octets = 0;
    *(tp = tmp) = 0;
    while ((ch = *src++) != '\0') {
	const char *pch;
	
	if ((pch = strchr(digits, ch)) != NULL) {
	    unsigned int new = *tp * 10 + (pch - digits);
	    
	    if (saw_digit && *tp == 0)
		return (0);
	    if (new > 255)
		return (0);
	    *tp = new;
	    if (!saw_digit) {
		if (++octets > 4)
		    return (0);
		saw_digit = 1;
	    }
	} else if (ch == '.' && saw_digit) {
	    if (octets == 4)
		return (0);
	    *++tp = 0;
	    saw_digit = 0;
	} else
	    return (0);
    }
    if (octets < 4)
	return (0);
    memcpy(dst, tmp, NS_INADDRSZ);
    return (1);
}


/*
 *============================================================================
 *                        bitd_inet_pton6
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
int bitd_inet_pton6(const char *src, unsigned char *dst) {
    static const char xdigits_l[] = "0123456789abcdef",
	xdigits_u[] = "0123456789ABCDEF";
#define NS_IN6ADDRSZ	16
#define NS_INT16SZ	2
    unsigned char tmp[NS_IN6ADDRSZ], *tp, *endp, *colonp;
    const char *xdigits, *curtok;
    int ch, seen_xdigits;
    unsigned int val;
    
    memset((tp = tmp), '\0', NS_IN6ADDRSZ);
    endp = tp + NS_IN6ADDRSZ;
    colonp = NULL;
    /* Leading :: requires some special handling. */
    if (*src == ':')
	if (*++src != ':')
	    return (0);
    curtok = src;
    seen_xdigits = 0;
    val = 0;
    while ((ch = *src++) != '\0') {
	const char *pch;
	
	if ((pch = strchr((xdigits = xdigits_l), ch)) == NULL)
	    pch = strchr((xdigits = xdigits_u), ch);
	if (pch != NULL) {
	    val <<= 4;
	    val |= (pch - xdigits);
	    if (++seen_xdigits > 4)
		return (0);
	    continue;
	}
	if (ch == ':') {
	    curtok = src;
	    if (!seen_xdigits) {
		if (colonp)
		    return (0);
		colonp = tp;
		continue;
	    } else if (*src == '\0') {
		return (0);
	    }
	    if (tp + NS_INT16SZ > endp)
		return (0);
	    *tp++ = (unsigned char) (val >> 8) & 0xff;
	    *tp++ = (unsigned char) val & 0xff;
	    seen_xdigits = 0;
	    val = 0;
	    continue;
	}
	if (ch == '.' && ((tp + NS_INADDRSZ) <= endp) &&
	    bitd_inet_pton4(curtok, tp) > 0) {
	    tp += NS_INADDRSZ;
	    seen_xdigits = 0;
	    break;	/*%< '\\0' was seen by inet_pton4(). */
		}
	return (0);
    }
    if (seen_xdigits) {
	if (tp + NS_INT16SZ > endp)
	    return (0);
	*tp++ = (unsigned char) (val >> 8) & 0xff;
	*tp++ = (unsigned char) val & 0xff;
    }
    if (colonp != NULL) {
	/*
	 * Since some memmove()'s erroneously fail to handle
	 * overlapping regions, we'll do the shift by hand.
	 */
	const int n = tp - colonp;
	int i;
	
	if (tp == endp)
	    return (0);
	for (i = 1; i <= n; i++) {
	    endp[- i] = colonp[n - i];
	    colonp[n - i] = 0;
	}
	tp = endp;
    }
    if (tp != endp)
	return (0);
    memcpy(dst, tmp, NS_IN6ADDRSZ);
    return (1);
}


/*
 *============================================================================
 *                        bitd_inet_pton
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
int bitd_inet_pton(int af, const char *src, void *dst) {
    switch (af) {
    case AF_INET:
	return bitd_inet_pton4(src, dst);
    case AF_INET6:
	return bitd_inet_pton6(src, dst);
    default:
	return (-1);
    }
}


/*
 *============================================================================
 *                        bitd_sin_family
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
bitd_uint16 bitd_sin_family(struct sockaddr_storage *a) {
    return ((struct sockaddr_in *)a)->sin_family;
} 


/*
 *============================================================================
 *                        bitd_set_sin_family
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
void bitd_set_sin_family(struct sockaddr_storage *a, bitd_uint16 sin_family) {
    ((struct sockaddr_in *)a)->sin_family = (bitd_uint8)sin_family;
#if defined(BITD_HAVE_STRUCT_SOCKADDR_IN_SIN_LEN)
    ((struct sockaddr_in *)a)->sin_len = bitd_sockaddrlen(a);
#endif
} 


/*
 *============================================================================
 *                        bitd_sin_addr
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
bitd_uint8 *bitd_sin_addr(struct sockaddr_storage *a) {
    if (bitd_sin_family(a) == AF_INET) {
        return (bitd_uint8 *)&(((struct sockaddr_in *)a)->sin_addr);
    } else if (bitd_sin_family(a) == AF_INET6) {
        return (bitd_uint8 *)&(((struct sockaddr_in6 *)a)->sin6_addr);
    } else {
        return NULL;
    }
} 


/*
 *============================================================================
 *                        bitd_sin_addrlen
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
bitd_uint32 bitd_sin_addrlen(struct sockaddr_storage *a) {
    if (bitd_sin_family(a) == AF_INET) {
        return 4;
    } else if (bitd_sin_family(a) == AF_INET6) {
        return 16;
    } else {
        return 0;
    }
} 


/*
 *============================================================================
 *                        bitd_sin_port
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
bitd_uint16 *bitd_sin_port(struct sockaddr_storage *a) {
    if (bitd_sin_family(a) == AF_INET) {
        return (bitd_uint16 *)&(((struct sockaddr_in *)a)->sin_port);
    } else if (bitd_sin_family(a) == AF_INET6) {
        return (bitd_uint16 *)&(((struct sockaddr_in6 *)a)->sin6_port);
    } else {
        return NULL;
    }
} 


/*
 *============================================================================
 *                        bitd_sockaddrlen
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
bitd_uint32 bitd_sockaddrlen(struct sockaddr_storage *a) {
    if (bitd_sin_family(a) == AF_INET) {
        return sizeof(struct sockaddr_in);
    } else if (bitd_sin_family(a) == AF_INET6) {
        return sizeof(struct sockaddr_in6);
    } else {
        return 0;
    }
} 

