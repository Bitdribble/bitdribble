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

#ifndef _BITD_PLATFORM_INETUTIL_H_
#define _BITD_PLATFORM_INETUTIL_H_

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



/*****************************************************************************
 *                            FUNCTION DEFINITIONS
 *****************************************************************************/

#ifndef BITD_HAVE_INET_PTON
# define inet_pton bitd_inet_pton
#endif

#ifndef BITD_HAVE_INET_NTOP
# define inet_ntop bitd_inet_ntop
#endif

int bitd_inet_pton(int af, const char *src, void *dst);
const char *bitd_inet_ntop(int af, const void *src,
			 char *dst, bitd_uint32 size);

struct sockaddr_storage;
bitd_uint16 bitd_sin_family(struct sockaddr_storage *a);
void bitd_set_sin_family(struct sockaddr_storage *a, bitd_uint16 sin_family);
bitd_uint8 *bitd_sin_addr(struct sockaddr_storage *a);
bitd_uint32 bitd_sin_addrlen(struct sockaddr_storage *a);
bitd_uint16 *bitd_sin_port(struct sockaddr_storage *a);
bitd_uint32 bitd_sockaddrlen(struct sockaddr_storage *a);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _BITD_PLATFORM_INETUTIL_H_ */
