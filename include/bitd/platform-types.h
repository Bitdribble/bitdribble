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

#ifndef _BITD_PLATFORM_TYPES_H_
#define _BITD_PLATFORM_TYPES_H_

/*****************************************************************************
 *                                INCLUDE FILES 
 *****************************************************************************/

#include "bitd/config.h"

#ifdef _WIN32
/* Use Microsoft socket libraries */
# define __USE_W32_SOCKETS
# include <winsock2.h>
# include <ws2tcpip.h>
# include <windows.h>
#endif

#include <stdio.h> 
#include <stddef.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <string.h> 
#include <sys/types.h>
#include <sys/stat.h>
#include <limits.h> 
#include <assert.h>
#include <errno.h>

#ifndef TRUE
# define TRUE 1
#endif

#ifndef FALSE
# define FALSE 0
#endif

#ifndef NULL
# define NULL (void *)0
#endif

#ifndef MIN
# define MIN(x,y) ((x) < (y) ? (x) : (y))
#endif

#ifndef MAX
# define MAX(x,y) ((x) > (y) ? (x) : (y))
#endif

#ifdef NDEBUG
# define bitd_assert if (0) assert
#else
# define bitd_assert _bitd_assert
#endif
#define _bitd_assert assert

/* The basic types */
typedef signed char bitd_int8;
typedef unsigned char bitd_uint8;
typedef short bitd_int16;
typedef unsigned short bitd_uint16;
typedef int bitd_int32;
typedef unsigned int bitd_uint32;
typedef long long bitd_int64;
typedef unsigned long long bitd_uint64;
typedef double bitd_double;

/* The boolean type */
typedef bitd_uint8 bitd_boolean;

/* The string type */
typedef char *bitd_string;

/* The blob type */
typedef struct {
  bitd_uint32 nbytes;
} bitd_blob;

#define bitd_blob_size(b) ((b)->nbytes)
#define bitd_blob_payload(b) ((char *)(((bitd_blob *)b)+1))

/* Forward declaration */
struct bitd_nvp_s;

/* Enumeration of types */
typedef enum {
    bitd_type_void,
    bitd_type_boolean,
    bitd_type_int64,
    bitd_type_uint64,
    bitd_type_double,
    bitd_type_string,
    bitd_type_blob,
    bitd_type_nvp,
    bitd_type_max
} bitd_type_t;

/* Untyped value */
typedef union {
    bitd_boolean value_boolean;
    bitd_int64 value_int64;
    bitd_uint64 value_uint64;
    bitd_double value_double;
    bitd_string value_string;
    bitd_blob *value_blob;
    struct bitd_nvp_s *value_nvp;
} bitd_value_t;

/* A name-value-pair element - or 'nvp element' */
typedef struct {
    char *name; 
    bitd_value_t v;
    bitd_type_t type;
} bitd_nvp_element_t;

/* A name-value-pair array - or 'nvp' */
typedef struct bitd_nvp_s {
    int n_elts;
    int n_elts_allocated;
    bitd_nvp_element_t e[1]; /* Array of named objects */
} *bitd_nvp_t;

/* An object is a typed value */
typedef struct {
    bitd_value_t v;
    bitd_type_t type;
} bitd_object_t;

#endif /* _BITD_PLATFORM_TYPES_H_ */
