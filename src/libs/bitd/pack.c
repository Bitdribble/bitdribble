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
#include "bitd/types.h"
#include "bitd/pack.h"
#include <math.h>

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
 *                        bitd_get_packed_size_object
 *============================================================================
 * Description:     Find size of object, once packed
 * Parameters:    
 * Returns:  
 */
int bitd_get_packed_size_object(bitd_object_t *a) {
    int ret;

    ret = bitd_get_packed_size_uint8(); /* for type */
    ret += bitd_get_packed_size_value(a->type, &a->v);
    
    return ret;
}


/*
 *============================================================================
 *                        bitd_pack_object
 *============================================================================
 * Description:     Pack an object
 * Parameters:    
 * Returns:  
 */
bitd_boolean bitd_pack_object(char *buf, int size, int *idx,
			      bitd_object_t *a) { 
    if (!a) {
	return FALSE;
    }

    /* Pack the type and value */
    if (!bitd_pack_uint8(buf, size, idx, (bitd_uint8)a->type) ||
	!bitd_pack_value(buf, size, idx, a->type, &a->v)) {
	return FALSE;
    }

    return TRUE; 
}


/*
 *============================================================================
 *                        bitd_unpack_object
 *============================================================================
 * Description:     Unpack an object
 * Parameters:    
 * Returns:  
 */
bitd_boolean bitd_unpack_object(char *buf, int size, int *idx,
				bitd_object_t *a) { 
    bitd_uint8 type;

    if (!a) {
	return FALSE;
    }

    /* Initialize the OUT parameter */
    a->type = bitd_type_void;

    if (!bitd_unpack_uint8(buf, size, idx, &type) ||
	(int)type >= (int)bitd_type_max ||
	!bitd_unpack_value(buf, size, idx, type, &a->v)) {
	return FALSE;
    }

    a->type = type;

    return TRUE; 
}


/*
 *============================================================================
 *                        bitd_get_packed_size_value
 *============================================================================
 * Description:     Find size of value, once packed
 * Parameters:    
 * Returns:  
 */
int bitd_get_packed_size_value(bitd_type_t t, bitd_value_t *v) {
    
    switch (t) {
    case bitd_type_void:
        return bitd_get_packed_size_void();
    case bitd_type_boolean:
        return bitd_get_packed_size_boolean();
    case bitd_type_int64:
    case bitd_type_uint64:
        return bitd_get_packed_size_int64();
    case bitd_type_double:
        return bitd_get_packed_size_double();
    case bitd_type_string:
        return bitd_get_packed_size_string(v->value_string);
    case bitd_type_blob:
        return bitd_get_packed_size_blob(v->value_blob);
    case bitd_type_nvp:
        return bitd_get_packed_size_nvp(v->value_nvp);
    default:
	bitd_assert(0);
	break;
    }	

    return 0;    
}


/*
 *============================================================================
 *                        bitd_pack_value
 *============================================================================
 * Description:     Pack generic values by type
 * Parameters:    
 * Returns:  
 */
bitd_boolean bitd_pack_value(char *buf, int size, int *idx, 
			     bitd_type_t t, bitd_value_t *v) {
    switch (t) {
    case bitd_type_void:
        return bitd_pack_void(buf, size, idx);
    case bitd_type_boolean:
        return bitd_pack_boolean(buf, size, idx, v->value_boolean);
    case bitd_type_int64:
        return bitd_pack_int64(buf, size, idx, v->value_int64);
    case bitd_type_uint64:
        return bitd_pack_uint64(buf, size, idx, v->value_uint64);
    case bitd_type_double:
        return bitd_pack_double(buf, size, idx, v->value_double);
    case bitd_type_string:
        return bitd_pack_string(buf, size, idx, v->value_string);
    case bitd_type_blob:
        return bitd_pack_blob(buf, size, idx, v->value_blob);
    case bitd_type_nvp:
        return bitd_pack_nvp(buf, size, idx, v->value_nvp);
    default:
	break;
    }	

    return FALSE;
} 


/*
 *============================================================================
 *                        bitd_unpack_value
 *============================================================================
 * Description:     Unpack generic values by type
 * Parameters:    
 * Returns:  
 */
bitd_boolean bitd_unpack_value(char *buf, int size, int *idx, 
			       bitd_type_t t, bitd_value_t *v) {

    switch (t) {
    case bitd_type_void:
        return bitd_unpack_void(buf, size, idx);
    case bitd_type_boolean:
        return bitd_unpack_boolean(buf, size, idx, &(v->value_boolean));
    case bitd_type_int64:
        return bitd_unpack_int64(buf, size, idx, &(v->value_int64));
    case bitd_type_uint64:
        return bitd_unpack_uint64(buf, size, idx, &(v->value_uint64));
    case bitd_type_double:
        return bitd_unpack_double(buf, size, idx, &(v->value_double));
    case bitd_type_string:
        return bitd_unpack_string(buf, size, idx, &(v->value_string));
    case bitd_type_blob:
        return bitd_unpack_blob(buf, size, idx, &(v->value_blob));
    case bitd_type_nvp:
        return bitd_unpack_nvp(buf, size, idx, &(v->value_nvp));
    default:
	break;
    }	
    return FALSE;
} 

int bitd_get_packed_size_string(char *value) {
    int len;

    if (!value) {
	len = 0;
    } else {
	len = strlen(value) + 1;
    }

    return bitd_get_packed_size_int32() + len;
}

int bitd_get_packed_size_blob(bitd_blob *value) {
    return bitd_get_packed_size_int32() + bitd_blob_size(value);
}

int bitd_get_packed_size_nvp(bitd_nvp_t nvp) {
    int size = 1, i; 

    if (!nvp) {
	return size;
    }

    /* Encode the number of elements */
    size += 4;

    /* Encode the packed size of each element */
    for (i = 0; i < nvp->n_elts; i++) {
	size += bitd_get_packed_size_string(nvp->e[i].name);
	size += bitd_get_packed_size_uint8(); /* for type */
	size += bitd_get_packed_size_value(nvp->e[i].type, &nvp->e[i].v);
    }

    return size;
}


bitd_boolean bitd_pack_void(char *buf, int size, int *idx) { 
  return TRUE; 
}


bitd_boolean bitd_pack_boolean(char *buf, int size, int *idx, 
			       bitd_boolean value) { 
  register int idx1 = *idx;

  if (idx1 + 1 > size) {
    return FALSE;
  }

  buf[idx1++] = value;

  *idx = idx1;

  return TRUE; 
}


bitd_boolean bitd_pack_int8(char *buf, int size, int *idx, 
			    bitd_int8 value) { 
  register int idx1 = *idx;

  if (idx1 + 1 > size) {
    return FALSE;
  }

  buf[idx1++] = value;

  *idx = idx1;

  return TRUE; 
}

bitd_boolean bitd_pack_uint8(char *buf, int size, int *idx, 
			     bitd_uint8 value) { 
  register int idx1 = *idx;

  if (idx1 + 1 > size) {
    return FALSE;
  }

  buf[idx1++] = value;

  *idx = idx1;

  return TRUE; 
}

bitd_boolean bitd_pack_int16(char *buf, int size, int *idx, 
			     bitd_int16 value) { 
  register int idx1 = *idx;

  if (idx1 + 2 > size) {
    return FALSE;
  }

  buf[idx1++] = value >> 8;
  buf[idx1++] = value & 0xff;

  *idx = idx1;

  return TRUE; 
}

bitd_boolean bitd_pack_uint16(char *buf, int size, int *idx, 
			      bitd_uint16 value) { 
  register int idx1 = *idx;

  if (idx1 + 2 > size) {
    return FALSE;
  }

  buf[idx1++] = value >> 8;
  buf[idx1++] = value & 0xff;

  *idx = idx1;

  return TRUE; 
}

bitd_boolean bitd_pack_int32(char *buf, int size, int *idx, 
			     bitd_int32 value) { 
  register int idx1 = *idx;

  if (idx1 + 4 > size) {
    return FALSE;
  }

  buf[idx1++] = value >> 24;
  buf[idx1++] = (value >> 16) & 0xff;
  buf[idx1++] = (value >> 8) & 0xff;
  buf[idx1++] = value & 0xff;

  *idx = idx1;

  return TRUE; 
}

bitd_boolean bitd_pack_uint32(char *buf, int size, int *idx, 
			      bitd_uint32 value) { 
  register int idx1 = *idx;

  if (idx1 + 4 > size) {
    return FALSE;
  }

  buf[idx1++] = value >> 24;
  buf[idx1++] = (value >> 16) & 0xff;
  buf[idx1++] = (value >> 8) & 0xff;
  buf[idx1++] = value & 0xff;

  *idx = idx1;

  return TRUE; 
}

bitd_boolean bitd_pack_int64(char *buf, int size, int *idx, 
			     bitd_int64 value) { 
  return bitd_pack_int32(buf, size, idx, value >> 32) && 
    bitd_pack_uint32(buf, size, idx, value & 0xffffffff);
}

bitd_boolean bitd_pack_uint64(char *buf, int size, int *idx, 
			      bitd_uint64 value) { 
  return bitd_pack_uint32(buf, size, idx, value >> 32) && 
    bitd_pack_uint32(buf, size, idx, value & 0xffffffff);
}

bitd_boolean bitd_pack_double(char *buf, int size, int *idx, 
			      bitd_double value) { 
  bitd_int32 exp;
  bitd_int64 frac;

  double xf = fabs(frexp(value, &exp)) - 0.5;

  if (xf < 0.0) {
   frac = 0;
  } else {
    frac = 1 + (bitd_int64)(xf * 2.0 * 0xffffffff);

    if (value < 0.0) {
      frac = -frac;
    }
  }

  return bitd_pack_int32(buf, size, idx, exp) && 
    bitd_pack_int64(buf, size, idx, frac);
}

bitd_boolean bitd_pack_string(char *buf, int size, int *idx, 
			      char *value) { 
  int len;
  register int idx1 = *idx;

  len = value ? (strlen(value) + 1) : 0;

  if (idx1 + 4 + len > size) {
    return FALSE;
  }

  if (!bitd_pack_uint32(buf, size, idx, len)) {
      return FALSE;
  }

  idx1 += 4;

  memcpy(buf + idx1, value, len);
  idx1 += len;

  *idx = idx1;

  return TRUE; 
}

bitd_boolean bitd_pack_blob(char *buf, int size, int *idx,
			bitd_blob *value) { 
  int len = bitd_blob_size(value);
  register int idx1 = *idx;

  if (idx1 + len + 4 > size) {
    return FALSE;
  }

  if (!bitd_pack_uint32(buf, size, idx, len)) {
    return FALSE;
  }

  idx1 += 4;

  memcpy(buf + idx1, bitd_blob_payload(value), len);
  idx1 += len;

  *idx = idx1;

  return TRUE; 
}

bitd_boolean bitd_pack_nvp(char *buf, int size, int *idx,
			   bitd_nvp_t nvp) { 
    int i;
    
    if (!nvp) {
	/* Encode the NULL nvp as a boolean TRUE */
	return bitd_pack_boolean(buf, size, idx, TRUE);
    } 
    
    /* Encode the non-NULL nvp as a boolean FALSE */
    if (!bitd_pack_boolean(buf, size, idx, FALSE)) {
	return FALSE;
    }
    
    /* Pack the element count */
    if (!bitd_pack_uint32(buf, size, idx, nvp->n_elts)) {
	return FALSE;
    }

    for (i = 0; i < nvp->n_elts; i++) {
	/* Pack the element name, type and value */
	if (!bitd_pack_string(buf, size, idx, nvp->e[i].name) ||
	    !bitd_pack_uint8(buf, size, idx, (bitd_uint8)nvp->e[i].type) ||
	    !bitd_pack_value(buf, size, idx, nvp->e[i].type, &nvp->e[i].v)) {
	    return FALSE;
	}
    }

    return TRUE; 
}


bitd_boolean bitd_unpack_void(char *buf, int size, int *idx) { 
    return TRUE; 
}


bitd_boolean bitd_unpack_boolean(char *buf, int size, int *idx, 
				 bitd_boolean *value) { 
  register int idx1 = *idx;
  register bitd_boolean a;

  if (idx1 + 1 > size) {
    return FALSE;
  }
  
  a = buf[idx1++];

  *value = a;
  *idx = idx1;

  return TRUE; 
}


bitd_boolean bitd_unpack_int8(char *buf, int size, int *idx, 
			      bitd_int8 *value) { 
  register int idx1 = *idx;
  register bitd_int8 a;

  if (idx1 + 1 > size) {
    return FALSE;
  }
  
  a = buf[idx1++];

  *value = a;
  *idx = idx1;

  return TRUE; 
}

bitd_boolean bitd_unpack_uint8(char *buf, int size, int *idx, 
			       bitd_uint8 *value) { 
  register int idx1 = *idx;
  register bitd_uint8 a;

  if (idx1 + 1 > size) {
    return FALSE;
  }
  
  a = buf[idx1++];

  *value = a;
  *idx = idx1;

  return TRUE; 
}

bitd_boolean bitd_unpack_int16(char *buf, int size, int *idx, 
			       bitd_int16 *value) { 
  register int idx1 = *idx;
  register bitd_int16 a;

  if (idx1 + 2 > size) {
    return FALSE;
  }
  
  a = (unsigned char)buf[idx1++];
  a <<= 8;
  a |= (unsigned char)buf[idx1++];

  *value = a;
  *idx = idx1;

  return TRUE; 
}

bitd_boolean bitd_unpack_uint16(char *buf, int size, int *idx, 
				bitd_uint16 *value) { 
  register int idx1 = *idx;
  register bitd_uint16 a;

  if (idx1 + 2 > size) {
    return FALSE;
  }
  
  a = (unsigned char)buf[idx1++];
  a <<= 8;
  a |= (unsigned char)buf[idx1++];

  *value = a;
  *idx = idx1;

  return TRUE; 
}

bitd_boolean bitd_unpack_int32(char *buf, int size, int *idx, 
			       bitd_int32 *value) { 
  register int idx1 = *idx;
  register bitd_int32 a;

  if (idx1 + 4 > size) {
    return FALSE;
  }
  
  a = (unsigned char)buf[idx1++];
  a <<= 8;
  a |= (unsigned char)buf[idx1++];
  a <<= 8;
  a |= (unsigned char)buf[idx1++];
  a <<= 8;
  a |= (unsigned char)buf[idx1++];

  *value = a;
  *idx = idx1;

  return TRUE; 
}

bitd_boolean bitd_unpack_uint32(char *buf, int size, int *idx, 
				bitd_uint32 *value) { 
  register int idx1 = *idx;
  register bitd_uint32 a;

  if (idx1 + 4 > size) {
    return FALSE;
  }
  
  a = (unsigned char)buf[idx1++];
  a <<= 8;
  a |= (unsigned char)buf[idx1++];
  a <<= 8;
  a |= (unsigned char)buf[idx1++];
  a <<= 8;
  a |= (unsigned char)buf[idx1++];

  *value = a;
  *idx = idx1;

  return TRUE; 
}

bitd_boolean bitd_unpack_int64(char *buf, int size, int *idx, 
			       bitd_int64 *value) { 
  bitd_int32 value_hi;
  bitd_uint32 value_lo;

  if (!bitd_unpack_int32(buf, size, idx, &value_hi) ||
      !bitd_unpack_uint32(buf, size, idx, &value_lo)) {
    return FALSE;
  }

  *value = (bitd_int64)value_lo + (((bitd_int64)value_hi) << 32);

  return TRUE; 
}

bitd_boolean bitd_unpack_uint64(char *buf, int size, int *idx, 
				bitd_uint64 *value) { 
  bitd_uint32 value_hi;
  bitd_uint32 value_lo;

  if (!bitd_unpack_uint32(buf, size, idx, &value_hi) ||
      !bitd_unpack_uint32(buf, size, idx, &value_lo)) {
    return FALSE;
  }

  *value = (bitd_int64)value_lo + (((bitd_int64)value_hi) << 32);

  return TRUE; 
}

bitd_boolean bitd_unpack_double(char *buf, int size, int *idx, 
				bitd_double *value) { 
  bitd_int32 exp;
  bitd_int64 frac;
  double xf, x;

  if (!bitd_unpack_int32(buf, size, idx, &exp) || 
      !bitd_unpack_int64(buf, size, idx, &frac)) {
    return FALSE;
  }

  if (!frac) {
    *value = 0.0;
    return TRUE;
  }

  xf = ((double)(llabs(frac) - 1) / (0xffffffff)) / 2.0;

  x = ldexp(xf + 0.5, exp);

  if (frac < 0) {
    x = -x;
  }

  *value = x;

  return TRUE; 
}

/* Will heap-allocate the *value */
bitd_boolean bitd_unpack_string(char *buf, int size, int *idx, 
				char **value) { 
  bitd_uint32 len;

  if (!value) {
    return FALSE;
  }

  if (!bitd_unpack_uint32(buf, size, idx, &len)) {
    return FALSE;
  }

  if (!len) {
      return TRUE;
  }

  *value = malloc(len);
  memcpy(*value, buf + *idx, len);

  (*idx) += len;

  return TRUE; 
}


/* Will heap-allocate the *value */
bitd_boolean bitd_unpack_blob(char *buf, int size, int *idx,
			      bitd_blob **value) { 
  bitd_uint32 len;

  if (!value) {
    return FALSE;
  }

  if (!bitd_unpack_uint32(buf, size, idx, &len)) {
    return FALSE;
  }

  *value = malloc(len + 4);

  bitd_blob_size(*value) = len;
  memcpy(bitd_blob_payload(*value), buf + *idx, len);

  (*idx) += len;

  return TRUE; 
}


bitd_boolean bitd_unpack_nvp(char *buf, int size, int *idx,
			     bitd_nvp_t *nvp) { 
    bitd_uint32 i, n_elts;
    bitd_boolean is_empty;
    char *name;
    bitd_uint8 type;
    bitd_value_t v;

    if (!nvp) {
	return FALSE;
    }

    /* Initialize the OUT parameter */
    *nvp = NULL;

    /* Is the nvp empty? */
    if (!bitd_unpack_boolean(buf, size, idx, &is_empty)) {
	return FALSE;
    }
    
    if (is_empty) {
	/* We're done - it's an empty nvp */
	return TRUE;
    }

    /* Get the element count */
    if (!bitd_unpack_uint32(buf, size, idx, &n_elts)) {
	return FALSE;
    }
    
    /* Get each element */
    for (i = 0; i < n_elts; i++) {
	name = NULL;
	type = 0;
	memset(&v, 0, sizeof(v));
	
	if (!bitd_unpack_string(buf, size, idx, &name) ||
	    !bitd_unpack_uint8(buf, size, idx, &type) ||
	    (int)type >= (int)bitd_type_max ||
	    !bitd_unpack_value(buf, size, idx, type, &v)) {

	    /* Free the nvp */
	    bitd_nvp_free(*nvp);
	    *nvp = NULL;

	    /* Free the name, value */
	    if (name) {
		free(name);
	    }
	    if (type && (int)type < (int)bitd_type_max) {
		bitd_value_free(&v, type);
	    }
	    
	    return FALSE;
	}

	/* Add the element to the nvp */
	bitd_nvp_add_elem(nvp, name, &v, type);

	/* Release memory */
	if (name) {
	    free(name);
	}
	bitd_value_free(&v, type);
    }

    return TRUE; 
}




