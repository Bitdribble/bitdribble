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

#ifndef _BITD_HASH_H_
#define _BITD_HASH_H_

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



/*****************************************************************************
 *                            FUNCTION DEFINITIONS
 *****************************************************************************/

typedef struct bitd_hash_s *bitd_hash;
typedef struct bitd_hash_key_s *bitd_hash_key;
typedef struct bitd_hash_value_s *bitd_hash_value;

/* Hashing function prototype */
typedef bitd_uint32 (bitd_hash_func_t)(bitd_hash_key k);

/* Key comparison function prototype, returning 0 on match */
typedef bitd_int32 (bitd_hash_compare_t)(bitd_hash_key k1, bitd_hash_key k2);

/* Free hash element prototype */
typedef void (bitd_hash_free_t)(bitd_hash_key k, bitd_hash_value v);

/* Hash mapping function prototype */
typedef void (bitd_hash_map_t)(bitd_hash_key k, bitd_hash_value v, void *cookie);

/* Create a hash, with a given hashing function */
bitd_hash bitd_hash_create(bitd_hash_func_t *f, 
                       bitd_hash_compare_t *cmp,
                       bitd_hash_free_t *g);

/* Remove all elements from the hash, and destroy the hash */
void bitd_hash_destroy(bitd_hash h);

/* Add a hash element. May fail in case of a key collision. */
bitd_boolean bitd_hash_add(bitd_hash h, bitd_hash_key k, bitd_hash_value v);

/* Remove a hash element. Return TRUE if hash element was actually removed. */
bitd_boolean bitd_hash_remove(bitd_hash h, bitd_hash_key k);

/* Call the passed-in map function for all elements in the hash.
   The mapping function may NOT remove elements from the hash. */
void bitd_hash_map(bitd_hash h, bitd_hash_map_t *m, void *cookie);

/* Some predefined comparison routines */
bitd_hash_func_t bitd_hash_func_int32;        /* For numeric key types */
bitd_hash_compare_t bitd_hash_compare_int32;
bitd_hash_func_t bitd_hash_func_string;       /* For string key types */
bitd_hash_compare_t bitd_hash_compare_string;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _BITD_HASH_H_ */
