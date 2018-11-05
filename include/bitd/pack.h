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

#ifndef _BITD_PACK_H_
#define _BITD_PACK_H_

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

/* Pack objects */
bitd_boolean bitd_pack_object(char *buf, int size, int *idx, 
			      bitd_object_t *a);
bitd_boolean bitd_unpack_object(char *buf, int size, int *idx, 
				bitd_object_t *a);
int bitd_get_packed_size_object(bitd_object_t *a);

/* Pack generic values by type */
bitd_boolean bitd_pack_value(char *buf, int size, int *idx, 
			     bitd_type_t t, bitd_value_t *v);
bitd_boolean bitd_unpack_value(char *buf, int size, int *idx, 
			       bitd_type_t t, bitd_value_t *v);
int bitd_get_packed_size_value(bitd_type_t t, bitd_value_t *v);

/* Pack specific values by type */
bitd_boolean bitd_pack_void(char *buf, int size, int *idx);
bitd_boolean bitd_pack_boolean(char *buf, int size, int *idx, bitd_boolean value);
bitd_boolean bitd_pack_int8(char *buf, int size, int *idx, bitd_int8 value);
bitd_boolean bitd_pack_uint8(char *buf, int size, int *idx, bitd_uint8 value);
bitd_boolean bitd_pack_int16(char *buf, int size, int *idx, bitd_int16 value);
bitd_boolean bitd_pack_uint16(char *buf, int size, int *idx, bitd_uint16 value);
bitd_boolean bitd_pack_int32(char *buf, int size, int *idx, bitd_int32 value);
bitd_boolean bitd_pack_uint32(char *buf, int size, int *idx, bitd_uint32 value);
bitd_boolean bitd_pack_int64(char *buf, int size, int *idx, bitd_int64 value);
bitd_boolean bitd_pack_uint64(char *buf, int size, int *idx, bitd_uint64 value);
bitd_boolean bitd_pack_double(char *buf, int size, int *idx, bitd_double value);
bitd_boolean bitd_pack_string(char *buf, int size, int *idx, char *value);
bitd_boolean bitd_pack_blob(char *buf, int size, int *idx, 
			    bitd_blob *value);
bitd_boolean bitd_pack_nvp(char *buf, int size, int *idx, 
			   bitd_nvp_t value);

/* Unpack specific values by type */
bitd_boolean bitd_unpack_void(char *buf, int size, 
			      int *idx);
bitd_boolean bitd_unpack_boolean(char *buf, int size, int *idx, 
				 bitd_boolean *value);
bitd_boolean bitd_unpack_int8(char *buf, int size, int *idx, 
			      bitd_int8 *value);
bitd_boolean bitd_unpack_uint8(char *buf, int size, int *idx, 
			       bitd_uint8 *value);
bitd_boolean bitd_unpack_int16(char *buf, int size, int *idx, 
			       bitd_int16 *value);
bitd_boolean bitd_unpack_uint16(char *buf, int size, int *idx, 
				bitd_uint16 *value);
bitd_boolean bitd_unpack_int32(char *buf, int size, int *idx, 
			       bitd_int32 *value);
bitd_boolean bitd_unpack_uint32(char *buf, int size, int *idx, 
				bitd_uint32 *value);
bitd_boolean bitd_unpack_int64(char *buf, int size, int *idx, 
			       bitd_int64 *value);
bitd_boolean bitd_unpack_uint64(char *buf, int size, int *idx, 
				bitd_uint64 *value);
bitd_boolean bitd_unpack_double(char *buf, int size, int *idx, 
				bitd_double *value);
bitd_boolean bitd_unpack_string(char *buf, int size, int *idx, 
				char **value);
bitd_boolean bitd_unpack_blob(char *buf, int size, int *idx, 
			      bitd_blob **value);
bitd_boolean bitd_unpack_nvp(char *buf, int size, int *idx, 
			     bitd_nvp_t *value);

/* Size of packed values by type */
#define bitd_get_packed_size_void() 0
#define bitd_get_packed_size_boolean() 1
#define bitd_get_packed_size_int8() 1
#define bitd_get_packed_size_uint8() 1
#define bitd_get_packed_size_int16() 2
#define bitd_get_packed_size_uint16() 2
#define bitd_get_packed_size_int32() 4
#define bitd_get_packed_size_uint32() 4
#define bitd_get_packed_size_int64() 8
#define bitd_get_packed_size_uint64() 8
#define bitd_get_packed_size_double() 12
int bitd_get_packed_size_string(char *value);
int bitd_get_packed_size_blob(bitd_blob *value);
int bitd_get_packed_size_nvp(bitd_nvp_t value);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _BITD_PACK_H_ */
