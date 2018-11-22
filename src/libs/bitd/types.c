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
#include "bitd/base64.h"

#include <ctype.h>
#include <stdlib.h>
#include <limits.h>

/*****************************************************************************
 *                             MANIFEST CONSTANTS
 *****************************************************************************/



/*****************************************************************************
 *                                  MACROS 
 *****************************************************************************/
#ifndef ULLONG_MAX
# define ULLONG_MAX (~(bitd_uint64)0) /* 0xFFFFFFFFFFFFFFFF */
#endif

#ifndef LLONG_MAX
# define LLONG_MAX ((bitd_int64)(ULLONG_MAX >> 1)) /* 0x7FFFFFFFFFFFFFFF */
#endif

#ifndef LLONG_MIN
# define LLONG_MIN (~LLONG_MAX) /* 0x8000000000000000 */
#endif


/*****************************************************************************
 *                                  TYPES
 *****************************************************************************/



/*****************************************************************************
 *                           FUNCTION DECLARATION
 *****************************************************************************/


/*****************************************************************************
 *                                VARIABLES
 *****************************************************************************/

static char * s_type_names[] = {
    "void",
    "boolean",
    "int64",
    "uint64",
    "double",
    "string",
    "blob",
    "nvp"
};

/*****************************************************************************
 *                          FUNCTION IMPLEMENTATION
 *****************************************************************************/


/*
 *============================================================================
 *                        bitd_get_type_t
 *============================================================================
 * Description:     Get type_t from type name in string form
 * Parameters:    
 * Returns:  
 */
bitd_type_t bitd_get_type_t(char *type_name) {
    int i;

    bitd_assert(sizeof(s_type_names) / sizeof(s_type_names[0]) == bitd_type_max);

    if (!type_name) {
	return bitd_type_max;
    }

    for (i = 0; i < bitd_type_max; i++) {
	if (!strcmp(type_name, s_type_names[i])) {
	    return i;
	}
    }

    return bitd_type_max;
} 


/*
 *============================================================================
 *                        bitd_get_type_name
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
char *bitd_get_type_name(bitd_type_t t) {
    
    bitd_assert(sizeof(s_type_names) / sizeof(s_type_names[0]) == bitd_type_max);

    if (t >= bitd_type_max) {
	return NULL;
    }

    return s_type_names[t];
} 


/*
 *============================================================================
 *                        bitd_object_init
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
void bitd_object_init(bitd_object_t *a) {
    
    if (!a) {
	return;
    }

    memset(a, 0, sizeof(*a));
} 


/*
 *============================================================================
 *                        bitd_object_clone
 *============================================================================
 * Description:     Clone a2 into a1
 * Parameters:    
 * Returns:  
 */
void bitd_object_clone(bitd_object_t *a1, 
		     bitd_object_t *a2) {

    a1->type = a2->type;
    bitd_value_clone(&a1->v, &a2->v, a2->type);
} 


/*
 *============================================================================
 *                        bitd_object_copy
 *============================================================================
 * Description:     Flat memcopy of a2 into a1
 * Parameters:    
 * Returns:  
 */
void bitd_object_copy(bitd_object_t *a1, 
		    bitd_object_t *a2) {

    memcpy(a1, a2, sizeof(*a1));
} 


/*
 *============================================================================
 *                        bitd_object_free
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
void bitd_object_free(bitd_object_t *a) {
    
    bitd_value_free(&a->v, a->type);
} 


/*
 *============================================================================
 *                        bitd_object_compare
 *============================================================================
 * Description:     Return 0 if same values, 1 if v1 > v2
 * Parameters:    
 * Returns:  
 */
int bitd_object_compare(bitd_object_t *a1, 
		      bitd_object_t *a2) {
    int ret;
    bitd_type_t t1 = a1->type;
    bitd_type_t t2 = a2->type;


#define CMP(x, y) (x == y ? 0 : (x < y ? -1 : 1))

    ret = CMP(t1, t2);
    if (ret) {
	return ret;
    }

    return bitd_value_compare(&a1->v, &a2->v, t1);
} 


/*
 *============================================================================
 *                        bitd_object_to_string
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
char *bitd_object_to_string(bitd_object_t *a) {
    return bitd_value_to_string(&a->v, a->type);
} 


/*
 *============================================================================
 *                        bitd_typed_string_to_object
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
bitd_boolean bitd_typed_string_to_object(bitd_object_t *a, /* OUT */
					 char *value_str, 
					 bitd_type_t t) {
    bitd_boolean ret = bitd_typed_string_to_value(&a->v, value_str, t);
    
    if (ret) {
	a->type = t;
    }
    return ret;
} 


/*
 *============================================================================
 *                        bitd_string_to_object
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
void bitd_string_to_object(bitd_object_t *a, /* OUT */
			   char *value_str) {
    bitd_string_to_type_and_value(&a->type, &a->v, value_str);
} 


/*
 *============================================================================
 *                        bitd_value_init
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
void bitd_value_init(bitd_value_t *v) {
    
    if (!v) {
	return;
    }

    memset(v, 0, sizeof(*v));
} 


/*
 *============================================================================
 *                        bitd_value_clone
 *============================================================================
 * Description:     Clone v2 into v1
 * Parameters:    
 * Returns:  
 */
void bitd_value_clone(bitd_value_t *v1, bitd_value_t *v2, bitd_type_t t) {
    
    bitd_assert(t < bitd_type_max);

    if (t == bitd_type_void) {
	/* Nothing to copy */
	return;
    }

    memcpy(v1, v2, sizeof(*v1));

    switch (t) {
    case bitd_type_string:
	if (v2->value_string) {
	    v1->value_string = strdup(v2->value_string);
	}
	break;
    case bitd_type_blob:	
	v1->value_blob = bitd_blob_clone(v2->value_blob);
	break;
    case bitd_type_nvp:
	v1->value_nvp = bitd_nvp_clone(v2->value_nvp);
	break;
    default:
	break;	
    }
} 


/*
 *============================================================================
 *                        bitd_value_copy
 *============================================================================
 * Description:     Flat memcopy of v2 into v1
 * Parameters:    
 * Returns:  
 */
void bitd_value_copy(bitd_value_t *v1, bitd_value_t *v2, bitd_type_t t) {
    memcpy(v1, v2, sizeof(*v1));
}

/*
 *============================================================================
 *                        bitd_value_free
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
void bitd_value_free(bitd_value_t *v, bitd_type_t t) {
    int i;
    bitd_nvp_t nvp;
    
    switch (t) {
    case bitd_type_string:
	if (v->value_string) {
	    free(v->value_string);
	    v->value_string = NULL;	    
	}
	break;
    case bitd_type_blob:
	if (v->value_blob) {
	    free(v->value_blob);
	    v->value_blob = NULL;	    
	}
	break;
    case bitd_type_nvp:
	nvp = v->value_nvp;
	if (nvp) {
	    for (i = 0; i < nvp->n_elts; i++) {
		if (nvp->e[i].name) {
		    free(nvp->e[i].name);
		}
		/* This will recurse over nvps */
		bitd_value_free(&nvp->e[i].v, nvp->e[i].type);
	    }
	    
	    free(nvp);
	    v->value_nvp = NULL;
	}
	break;	
    default:
	break;
    }
} 


/*
 *============================================================================
 *                        bitd_value_compare
 *============================================================================
 * Description:     Return 0 if same values, 1 if v1 > v2
 * Parameters:    
 * Returns:  
 */
int bitd_value_compare(bitd_value_t *v1, bitd_value_t *v2, bitd_type_t t) {
    
    bitd_assert(t < bitd_type_max);

#define CMP(x, y) (x == y ? 0 : (x < y ? -1 : 1))

    switch (t) {
    case bitd_type_void:
	return TRUE;
    case bitd_type_boolean:
	return CMP(v1->value_boolean, v2->value_boolean);
    case bitd_type_int64:
	return CMP(v1->value_int64, v2->value_int64);
    case bitd_type_uint64:
	return CMP(v1->value_uint64, v2->value_uint64);
    case bitd_type_double:
	return CMP(v1->value_double, v2->value_double);
    case bitd_type_string:
	if (!v1->value_string && !v2->value_string) {
	    return 0;
	} else if (!v1->value_string && v2->value_string) {
	    return -1;
	} else if (v1->value_string && !v2->value_string) {
	    return 1;
	} else {
	    return strcmp(v1->value_string, v2->value_string);
	}
    case bitd_type_blob:
	return bitd_blob_compare(v1->value_blob, 
				 v2->value_blob);
    case bitd_type_nvp:
	return bitd_nvp_compare(v1->value_nvp,
				v2->value_nvp);
    default:
	break;	
    }

    return 1;
} 


/*
 *============================================================================
 *                        bitd_double_approx_same
 *============================================================================
 * Description:     Doubles are approx. the same
 * Parameters:    
 * Returns:  
 */
int bitd_double_approx_same(double a, double b) {

    if ((a-b) < 0.000000001 &&
	(b-a) < 0.000000001) {
	return 0;
    } else if (a < b) {
	return -1;
    } else {
	return 1;
    }
}


/*
 *============================================================================
 *                        bitd_double_precision
 *============================================================================
 * Description:     Estimate the format precision - not an exact result
 * Parameters:    
 * Returns:  
 */
int bitd_double_precision(double a) {
    
    if (!bitd_double_approx_same(a*1000, (long long)(a*1000))) {
	return 3;
    } else if (!bitd_double_approx_same(a*1000000, (long long)(a*1000000))) {
	return 6;
    } else {
	return 9;
    }
} 



/*
 *============================================================================
 *                        bitd_value_to_string
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
char *bitd_value_to_string(bitd_value_t *v, bitd_type_t t) {
    char *buf;
    int len = 0;

    bitd_assert(t < bitd_type_max);

    if (!v) {
	return NULL;
    }

    switch (t) {
    case bitd_type_void:
	len = 1;
	break;
    case bitd_type_boolean:
	len = sizeof("FALSE");
	break;
    case bitd_type_int64:
    case bitd_type_uint64:
    case bitd_type_double:
	len = 64;
	break;
    case bitd_type_string:
	if (!v->value_string) {
	    return NULL;
	}
	len = strlen((char *)(v->value_string)) + 1;
	break;
    case bitd_type_blob:
	/* Direct return */
	return bitd_blob_to_string_base64(v->value_blob);
    case bitd_type_nvp:
	/* Direct return */
	return bitd_nvp_to_string(v->value_nvp, NULL);
    default:
	break;	
    }

    buf = malloc(len);

    switch (t) {
    case bitd_type_void:
	buf[0] = 0;
	break;
    case bitd_type_boolean:
	snprintf(buf, len, "%s", v->value_boolean ? "TRUE" : "FALSE");
	break;
    case bitd_type_int64:
	snprintf(buf, len, "%lld", v->value_int64);
	break;
    case bitd_type_uint64: 
	snprintf(buf, len, "%llu", v->value_uint64);
	break;
    case bitd_type_double:
	if (v->value_double == (long long)v->value_double) {
 	    snprintf(buf, len, "%lld.0", (long long)v->value_double);
	} else {
	    snprintf(buf, len, "%.*g", 
		     bitd_double_precision(v->value_double), v->value_double);
	}
	break;
    case bitd_type_string:
	snprintf(buf, len, "%s", v->value_string);
	break;
    case bitd_type_blob:
    case bitd_type_nvp:
	/* Fallthrough - already handled in previous switch block */
	break;
    default:
	break;	
    }

    return buf;
}


/*
 *============================================================================
 *                        bitd_string_to_boolean
 *============================================================================
 * Description:     Convert a string to a bitd_boolean_t
 * 
 * Parameters:    
 *     v [OUT] - the boolean value
 *     value_str - the input string
 * Returns:  
 *     Return TRUE if success, and the value in 'v'.
 */
static bitd_boolean bitd_string_to_boolean(bitd_value_t *v, /* OUT */
					   char *value_str) {

    if (!strcmp(value_str, "FALSE") ||
	!strcmp(value_str, "False") ||
	!strcmp(value_str, "false")) {
	if (v) {
	    v->value_boolean = FALSE;
	}
	return TRUE;
    } else if (!strcmp(value_str, "TRUE") ||
	       !strcmp(value_str, "True") ||
	       !strcmp(value_str, "true")) {
	if (v) {
	    v->value_boolean = TRUE;
	}
	return TRUE;
    }
    
    return FALSE;
}


/*
 *============================================================================
 *                        bitd_string_to_int
 *============================================================================
 * Description:     Convert a string to bitd_int64 (if number is between
 *     LLONG_MIN and LLONG_MAX) or to bitd_uint64 (if number is between
 *     LLONG_MAX+1 and ULLONG_MAX). Initial and trailing spaces in the
 *     value_str are allowed. The number may start with a '+' or a '-'
 *     character.
 * 
 * Parameters:    
 *     t [OUT] - the type, which can be bitd_int64 or bitd_uint64
 *     v [OUT] - the numeric value
 *     value_str - the input string
 * Returns:  
 *     Return TRUE if success, the bitd_int64 or bitd_uint64 type in 't', and 
 *     the value in 'v'.
 */
static bitd_boolean bitd_string_to_int(bitd_type_t *t, /* OUT */
				       bitd_value_t *v, /* OUT */
				       char *value_str) {
    register const char *s = value_str;
    register bitd_uint64 acc;
    register int c;
    register bitd_uint64 cutoff;
    register int sign = 1, unsigned_64 = FALSE, cutlim;

    if (!value_str) {
	return FALSE;
    }

    /* Skip leading white spaces */
    do {
	c = *s++;
    } while (isspace(c));

    /* Number sign */
    if (c == '-') {
	sign = -1;
	c = *s++;
    } else if (c == '+') {
	c = *s++;
    } 

    /* Compute the cutoff value */
    if (sign == -1) {
	cutoff = -(bitd_uint64)LLONG_MIN;
    } else {
	cutoff = LLONG_MAX;
    }
    cutlim = cutoff % (bitd_uint64)10;
    cutoff /= (bitd_uint64)10;

    for (acc = 0;; c = *s++) {
	if (c >= '0' && c <= '9') {
	    c -= '0';
	} else {
	    break;
	}
	if (acc > cutoff || (acc == cutoff && c > cutlim)) {
	    /* Overflow. */
	    if (unsigned_64) {
		/* An overflowing bitd_uint64 number - give up */
		return FALSE;
	    }
	    
	    /* Instead of bitd_int64, look for bitd_uint64 type */
	    unsigned_64 = TRUE;

	    /* Only numbers > LLONG_MAX+1 and <= ULLONG_MAX are allowed 
	       as bitd_uint64 types. Negative numbers are disallowed. */
	    if (sign < 0) {
		return FALSE;
	    }
	    
	    /* Update the cutoff for bitd_uint64 */
	    cutoff = ULLONG_MAX;
	    cutlim = cutoff % (bitd_uint64)10;
	    cutoff /= (bitd_uint64)10;
	}

	/* Update the accumulator */
	acc *= 10;
	acc += c;
    }

    /* There are no more number characters. 
       Only space chars are allowed to trail */
    while (isspace(c)) {
	c = *s++;
    }
    if (!c) {
	/* End of the string */	
	if (unsigned_64) {
	    if (t) {
		*t = bitd_type_uint64;
	    }
	    if (v) {
		v->value_uint64 = acc;
	    }
	} else {
	    if (t) {
		*t = bitd_type_int64;
	    }
	    if (v) {
		v->value_int64 = sign * (bitd_int64)acc;
	    }
	}
	return TRUE;
    }
	 
    /* Non-space chars in the trailer */
    return FALSE;
} 


/*
 *============================================================================
 *                        bitd_string_to_double
 *============================================================================
 * Description:     Convert a string to a bitd_double_t
 * 
 * Parameters:    
 *     v [OUT] - the double value
 *     value_str - the input string
 * Returns:  
 *     Return TRUE if success, and the value in 'v'.
 */
static bitd_boolean bitd_string_to_double(bitd_value_t *v, /* OUT */
					  char *value_str) {

    double d = 0;
    char *next = NULL;

    d = strtod(value_str, &next);
    if (next) {
	while (isspace((int)*next)) {
	    next++;
	}
	if (*next) {
	    return FALSE;
	}
    }

    if (v) {
	v->value_double = d;
    }
    return TRUE;
}


/*
 *============================================================================
 *                        bitd_typed_string_to_value
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
bitd_boolean bitd_typed_string_to_value(bitd_value_t *v /* OUT */, 
					char *value_str,
					bitd_type_t t) {
    bitd_type_t t1;
    bitd_value_t v1;

    bitd_assert(t < bitd_type_max);

    if (!v) {
	return FALSE;
    }

    memset(v, 0, sizeof(*v));
    
    if (!value_str) {
	/* Only strings and void are allowed to be NULL */
	if (t == bitd_type_void ||
	    t == bitd_type_string) {
	    return TRUE;
	} else {
	    return FALSE;
	}
    }

    switch (t) {
    case bitd_type_boolean:
	return bitd_string_to_boolean(v, value_str);

    case bitd_type_int64:
	if (bitd_string_to_int(&t1, &v1, value_str) &&
	    t1 == bitd_type_int64) {
	    v->value_int64 = v1.value_int64;
	    return TRUE;
	}
	return FALSE;

    case bitd_type_uint64:
	if (bitd_string_to_int(&t1, &v1, value_str)) {
	    if (t1 == bitd_type_int64 &&
		v1.value_int64 >= 0) {
		v->value_uint64 = (bitd_uint64)v1.value_int64;
		return TRUE;
	    } else if (t1 == bitd_type_uint64){
		v->value_uint64 = v1.value_uint64;
		return TRUE;
	    }
	}
	return FALSE;

    case bitd_type_double:
	return bitd_string_to_double(v, value_str);

    case bitd_type_string:
	v->value_string = (bitd_string)strdup(value_str);
	return TRUE;

    case bitd_type_blob:
	return bitd_string_base64_to_blob(&v->value_blob, value_str);

    case bitd_type_nvp:
	/* Not supported */
	return FALSE;

    default:
	break;	
    }

    return TRUE;
}


/*
 *============================================================================
 *                        bitd_string_to_type_and_value
 *============================================================================
 * Description:     Get the default type of a string, and also its value
 * Parameters:   
 *     t [OUT] - returns the default type of value_str
 *     v [OUT] - returns the value of value_str associated to the detected
 *               default type
 *     value_str - the value in string format
 * Returns:  
 */
void bitd_string_to_type_and_value(bitd_type_t *t, /* OUT */
				   bitd_value_t *v, /* OUT */
				   char *value_str) {
    if (!value_str || !value_str[0]) {
	*t = bitd_type_void;
	return;
    }

    if (bitd_string_to_boolean(v, value_str)) {
	*t = bitd_type_boolean;
	return;	
    }
    
    if (bitd_string_to_int(t, v, value_str)) {
	return;
    }

    if (bitd_string_to_double(v, value_str)) {
	*t = bitd_type_double;
	return;	
    }

    *t = bitd_type_string;
    if (v) {
	v->value_string = strdup(value_str);
    }
}


/*
 *============================================================================
 *                        bitd_blob_alloc
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
bitd_blob *bitd_blob_alloc(int blob_size) {
    bitd_blob *v;

    /* Parameter check */
    if (blob_size < 0) {
	return NULL;
    }

    v = malloc(4 + blob_size);
    bitd_blob_size(v) = blob_size;

    return v;
} 


/*
 *============================================================================
 *                        bitd_blob_clone
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
bitd_blob *bitd_blob_clone(bitd_blob *nvp1) {
    bitd_blob *nvp2;

    if (!nvp1) {
	return NULL;
    }

    nvp2 = malloc(4 + bitd_blob_size(nvp1));
    bitd_blob_size(nvp2) = bitd_blob_size(nvp1);
    memcpy(bitd_blob_payload(nvp2),
	   bitd_blob_payload(nvp1),
	   bitd_blob_size(nvp1));

    return nvp2;
} 


/*
 *============================================================================
 *                        bitd_blob_compare
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
int bitd_blob_compare(bitd_blob *b1, 
		    bitd_blob *b2) {
    
    if (!b1 && !b2) {
	return 0;
    } else if (!b1 && b2) {
	return -1;
    } else if (b1 && !b2) {
	return 1;
    } else if (bitd_blob_size(b1) != bitd_blob_size(b2)) {
	return (bitd_blob_size(b1) < bitd_blob_size(b2) ? -1 : 1);
    } else {
	return memcmp(bitd_blob_payload(b1),
		      bitd_blob_payload(b2),
		      bitd_blob_size(b1));
    }
} 


/*
 *============================================================================
 *                        bitd_blob_to_string_hex
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
char *bitd_blob_to_string_hex(bitd_blob *b) {
    int len, i, idx;
    char *buf;

    len = 2 * bitd_blob_size(b) + 1;
    buf = malloc(len);

    idx = 0;
    buf[0] = 0;
    for (i = 0; i < bitd_blob_size(b); i++) {
	idx += snprintf(buf + idx, len - idx, "%02x", 
			(bitd_blob_payload(b))[i]);
    }

    return buf;
} 


/*
 *============================================================================
 *                        bitd_string_to_blob_hex
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
bitd_boolean bitd_string_to_blob_hex(bitd_blob **b /* OUT */, 
				     char *value_str) {
    int i, len;
    char c0, c1;
    bitd_int8 s8_0, s8_1;
    
    if (!b) {
	return FALSE;
    }

    /* Initialize OUT parameter */
    *b = NULL;
        
    if (!value_str) {
	return FALSE;
    }

    /* Get the blob size */
    len = strlen(value_str);

    /* Length should be even */
    if (len % 2) {
	return FALSE;
    }

    *b = malloc(len/2 + 4);
    bitd_blob_size(*b) = len/2;
	
    for (i = 0; i < len; i += 2) {
	c0 = value_str[i];
	c1 = value_str[i+1];
	
	if (c0 >= '0' && c0 <= '9') {
	    s8_0 = c0 - '0';
	} else if (c0 >= 'a' && c0 <= 'f') {
	    s8_0 = (c0 - 'a') + 10;
	} else if (c0 >= 'A' && c0 <= 'F') {
	    s8_0 = (c0 - 'A') + 10;
	} else {
	    free(*b);
	    *b = NULL;
	    return FALSE;
	}
	
	if (c1 >= '0' && c1 <= '9') {
	    s8_1 = c1 - '0';
	} else if (c1 >= 'a' && c1 <= 'f') {
	    s8_1 = (c1 - 'a') + 10;
	} else if (c1 >= 'A' && c1 <= 'F') {
	    s8_1 = (c1 - 'A') + 10;
	} else {
	    free(*b);
	    *b = NULL;
	    return FALSE;
	}
	
	bitd_blob_payload(*b)[i/2] = (char)(s8_1 + (s8_0 << 4));
    }

    return TRUE;
} 


/*
 *============================================================================
 *                        bitd_blob_to_string_base64
 *============================================================================
 * Description:     Convert a blob to a string in base64 format
 * Parameters:    
 * Returns:  
 */
char *bitd_blob_to_string_base64(bitd_blob *b) {
    int len;
    char *buf;

    len = bitd_base64_encode_len(bitd_blob_size(b));
    buf = malloc(len);
    bitd_base64_encode(buf, bitd_blob_payload(b), bitd_blob_size(b));
    
    return buf;
} 


/*
 *============================================================================
 *                        bitd_string_base64_to_blob
 *============================================================================
 * Description:    Convert a base64 string to blob
  * Parameters:    
 * Returns:  
 */
bitd_boolean bitd_string_base64_to_blob(bitd_blob **b /* OUT */, 
				    char *value_str) {
    int len;

    len = bitd_base64_decode_len(value_str);
    *b = malloc(len + 4);
    bitd_blob_size(*b) = bitd_base64_decode(bitd_blob_payload(*b), value_str);

    return TRUE;
}

/*
 *============================================================================
 *                        bitd_nvp_alloc
 *============================================================================
 * Description:     Allocate an nvp
 * Parameters:    
 * Returns:  
 */
bitd_nvp_t bitd_nvp_alloc(int n_elts) {
    bitd_nvp_t v;

    v = malloc(sizeof(*v) + n_elts * sizeof(bitd_nvp_element_t));
    v->n_elts = 0;
    v->n_elts_allocated = n_elts;

    return v;    
} 


/*
 *============================================================================
 *                        bitd_nvp_clone
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
bitd_nvp_t bitd_nvp_clone(bitd_nvp_t nvp1) {
    bitd_nvp_t nvp2;
    int i;

    if (!nvp1) {
	return NULL;
    }

    nvp2 = calloc(1, sizeof(*nvp2) + nvp1->n_elts * sizeof(nvp2->e));
    nvp2->n_elts = nvp1->n_elts;
    nvp2->n_elts_allocated = nvp1->n_elts; /* nvp2 is smaller! */

    memcpy(&nvp2->e[0], &nvp1->e[0], nvp2->n_elts * sizeof(nvp2->e[0]));

    /* Copy the element names, types and values. This will
       recurse on nvp sub-elements. */
    for (i = 0; i < nvp1->n_elts; i++) {
	if (nvp1->e[i].name) {
	    nvp2->e[i].name = strdup(nvp1->e[i].name);
	}
	nvp2->e[i].type = nvp1->e[i].type;
	bitd_value_clone(&nvp2->e[i].v, &nvp1->e[i].v, nvp1->e[i].type);
    }

    return nvp2;
} 


/*
 *============================================================================
 *                        bitd_nvp_free
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
void bitd_nvp_free(bitd_nvp_t v) {
    int i;

    if (!v) {
	return;
    }

    for (i = 0; i < v->n_elts; i++) {
	if (v->e[i].name) {
	    free(v->e[i].name);
	}

	/* This will recurse over nvp subelements */
	bitd_value_free(&v->e[i].v, v->e[i].type);
    }

    free(v);
} 


/*
 *============================================================================
 *                        bitd_nvp_elem_compare
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
int bitd_nvp_elem_compare(bitd_nvp_element_t *e1, bitd_nvp_element_t *e2) {
    int ret;

    if (e1->name && !e2->name) {
	return 1;
    } else if (!e1->name && e2->name) {
	return -1;
    } else if (e1->name && e2->name) {
	ret = strcmp(e1->name, e2->name);
	if (ret) {
	    return ret;
	}
    } 
    
    /* Compare element types */
    if (e1->type < e2->type) {
	return -1;
    } else if (e1->type > e2->type) {
	return 1;
    }
    
    /* Compare element values. This recurses over nvp elements. */
    return bitd_value_compare(&e1->v, &e2->v, e1->type);
} 


/*
 *============================================================================
 *                        bitd_nvp_compare
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  0 if the nvps have the same contents, 1 if nvp1 > nvp2, 
 *     and -1 otherwise
 */
int bitd_nvp_compare(bitd_nvp_t nvp1, 
		     bitd_nvp_t nvp2) {
    int i, ret;

    if (!nvp1 && !nvp2) {
	return 0;
    } else if (nvp1 && !nvp2) {
	return 1;
    } else if (!nvp1 && nvp2) {
	return -1;
    } else if (nvp1->n_elts != nvp2->n_elts) {
	return (nvp1->n_elts < nvp2->n_elts ? -1 : 1);
    } else {
	for (i = 0; i < nvp1->n_elts; i++) {
	    /* Compare elements */
	    ret = bitd_nvp_elem_compare(&nvp1->e[i], &nvp2->e[i]);
	    if (ret) {
		return ret;
 	    }
	}
    }

    /* ...We've concluded they are the same */
    return 0;
} 


/*
 *============================================================================
 *                        bitd_nvp_sort
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
void bitd_nvp_sort(bitd_nvp_t nvp) {
    
    if (!nvp) {
	return;
    }

    /* Use the quicksort standard library routine to sort the nvp */
    qsort(&nvp->e[0], nvp->n_elts, sizeof(nvp->e[0]), 
	  (int (*)(const void*, const void*))&bitd_nvp_elem_compare);
} 


/*
 *============================================================================
 *                        bitd_nvp_add_elem
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
bitd_boolean bitd_nvp_add_elem(bitd_nvp_t *vp, 
			   char *name, bitd_value_t *v, bitd_type_t type) {
    int i;

    /* Parameter check */
    if (!vp) {
	return FALSE;
    }

    if (!*vp) {
	*vp = bitd_nvp_alloc(4);
    } else if ((*vp)->n_elts == (*vp)->n_elts_allocated) {
	*vp = realloc(*vp, sizeof(**vp) + 2*((*vp)->n_elts)*sizeof(bitd_nvp_element_t));
	(*vp)->n_elts_allocated = 2*((*vp)->n_elts);
    }

    i = (*vp)->n_elts;
    (*vp)->e[i].name = name ? strdup(name) : NULL;
    bitd_value_clone(&((*vp)->e[i].v), v, type);
    (*vp)->e[i].type = type;

    /* Update the nvp element count */
    (*vp)->n_elts = i+1;

    return TRUE;
} 


/*
 *============================================================================
 *                        bitd_nvp_delete_elem
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
void bitd_nvp_delete_elem(bitd_nvp_t nvp, 
			  char *elem_name) {
    int i;

    /* Parameter check */
    if (!nvp || !elem_name) {
	return;
    }

    i = 0;
    while (i < nvp->n_elts) {
	if (nvp->e[i].name && !strcmp(nvp->e[i].name, elem_name)) {
	    bitd_nvp_delete_elem_by_idx(nvp, i);
	    /* Keep the index unchanged - we'll process what was i+1 as i */
	} else {
	    /* Go to next element */
	    i++;
	}
    }
} 


/*
 *============================================================================
 *                        bitd_nvp_delete_elem_by_idx
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
void bitd_nvp_delete_elem_by_idx(bitd_nvp_t nvp, int idx) {
    int i;

    /* Parameter check */
    if (!nvp || idx < 0 || idx >= nvp->n_elts) {
	return;
    }
    
    if (nvp->e[idx].name) {
	free(nvp->e[idx].name);
    }
    bitd_value_free(&nvp->e[idx].v, nvp->e[idx].type);
    
    /* Slide elements down by one */
    for (i = idx+1; i < nvp->n_elts; i++) {
	nvp->e[i-1] = nvp->e[i];
    }

    /* Adjust the count */
    nvp->n_elts--;    
} 


/*
 *============================================================================
 *                        bitd_nvp_lookup_elem
 *============================================================================
 * Description:     Look up an nvp element by name
 * Parameters:    
 *     nvp - the passed-in nvp
 *     elem_name - the element name we're looking for
 *     idx [OUT] - used for returning the element index, if found
 * Returns:  TRUE if found. The element index is set in *idx.
 */
bitd_boolean bitd_nvp_lookup_elem(bitd_nvp_t nvp, 
				  char *elem_name,
				  int *idx) {
    int i;

    if (idx) {
	/* Initialize the OUT parameter */
	*idx = 0;
    }

    /* More parameter check */
    if (!nvp || !elem_name) {
	return FALSE;
    }

    for (i = 0; i < nvp->n_elts; i++) {
	if (nvp->e[i].name && !strcmp(nvp->e[i].name, elem_name)) {
	    if (idx) {
		/* Save the index in the the OUT parameter */
		*idx = i;
	    }
	    return TRUE;
	}
    }

    return FALSE;
} 


/*
 *============================================================================
 *                        bitd_nvp_trim
 *============================================================================
 * Description:     Trim an nvp to the given list of elements
 * Parameters:    
 *     nvp - the passed-in nvp
 *     elem_names - the array of element names we're looking for
 *     n_elem_names - how many names in the array
 * Returns:  
 *     New nvp containing only the listed elements
 */
bitd_nvp_t bitd_nvp_trim(bitd_nvp_t nvp, 
			 char **elem_names,
			 int n_elem_names) {
    bitd_nvp_t trimmed_nvp;
    int i, j;

    if (!nvp || !n_elem_names || !elem_names) {
	return NULL;
    }

    trimmed_nvp = calloc(1, sizeof(*trimmed_nvp) + nvp->n_elts * sizeof(trimmed_nvp->e));
    trimmed_nvp->n_elts_allocated = nvp->n_elts; /* trimmed nvp is smaller! */

    /* Copy the element names, types and values. This will
       recurse on nvp sub-elements. */
    for (i = 0; i < nvp->n_elts; i++) {
	if (nvp->e[i].name) {
	    for (j = 0; j < n_elem_names; j++) {
		if (elem_names[j] && !strcmp(elem_names[j], nvp->e[i].name)) {
		    /* Element name match - copy element */
		    trimmed_nvp->e[trimmed_nvp->n_elts].name = strdup(nvp->e[i].name);
		    trimmed_nvp->e[trimmed_nvp->n_elts].type = nvp->e[i].type;
		    bitd_value_clone(&trimmed_nvp->e[trimmed_nvp->n_elts].v, 
				   &nvp->e[i].v, 
				   nvp->e[i].type);
		    trimmed_nvp->n_elts++;
		}
	    }
	}
    }

    return trimmed_nvp;
}


/*
 *============================================================================
 *                        bitd_nvp_trim_bytype
 *============================================================================
 * Description:     Trim an nvp to the given list of elements while
 *     also specifying the type
 * Parameters:    
 *     nvp - the passed-in nvp
 *     elem_names - the array of element names we're looking for
 *     n_elem_names - how many names in the array
 *     type - the looked-for element type
 * Returns:  
 *     New nvp containing only the listed elements
 */
bitd_nvp_t bitd_nvp_trim_bytype(bitd_nvp_t nvp, 
				char **elem_names,
				int n_elem_names,
				bitd_type_t type) {
    bitd_nvp_t trimmed_nvp;
    int i, j;

    if (!nvp || !n_elem_names || !elem_names) {
	return NULL;
    }

    trimmed_nvp = calloc(1, sizeof(*trimmed_nvp) + nvp->n_elts * sizeof(trimmed_nvp->e));
    trimmed_nvp->n_elts_allocated = nvp->n_elts; /* trimmed nvp is smaller! */

    /* Copy the element names, types and values. This will
       recurse on nvp sub-elements. */
    for (i = 0; i < nvp->n_elts; i++) {
	if (nvp->e[i].name && nvp->e[i].type == type) {
	    for (j = 0; j < n_elem_names; j++) {
		if (elem_names[j] && 
		    !strcmp(elem_names[j], nvp->e[i].name)) {
		    /* Element name match - copy element */
		    trimmed_nvp->e[trimmed_nvp->n_elts].name = strdup(nvp->e[i].name);
		    trimmed_nvp->e[trimmed_nvp->n_elts].type = nvp->e[i].type;
		    bitd_value_clone(&trimmed_nvp->e[trimmed_nvp->n_elts].v, 
				   &nvp->e[i].v, 
				   nvp->e[i].type);
		    trimmed_nvp->n_elts++;
		}
	    }
	}
    }

    return trimmed_nvp;
}



/*
 *============================================================================
 *                        bitd_nvp_chunk
 *============================================================================
 * Description:     Break an nvp into nvp chunks by element name - i.e.,
 *     create a new nvp whose elements are nvps consisting of all elements
 *     in the nvp parameters with the same element name
 * Parameters:    
 * Returns:  
 */
bitd_nvp_t bitd_nvp_chunk(bitd_nvp_t nvp) {
    bitd_nvp_t chunked_nvp = NULL;
    bitd_nvp_t trimmed_nvp;
    int i, j;
    char *elem_name;
    bitd_boolean found;

    if (!nvp) {
	return NULL;
    }

    /* Allocate an empty chunked nvp */
    chunked_nvp = bitd_nvp_alloc(1);

    for (i = 0; i < nvp->n_elts; i++) {
	elem_name = nvp->e[i].name;

	/* Has a chunk already been created for elem_name? */
	found = FALSE;
	if (chunked_nvp) {
	    for (j = 0; j < chunked_nvp->n_elts; j++) {		
		if ((!elem_name && !chunked_nvp->e[j].name) ||
		    (elem_name && chunked_nvp->e[j].name &&
		     !strcmp(elem_name, chunked_nvp->e[j].name))) {
		    found = TRUE;
		    break;
		}
	    }
	}
	if (found) {
	    /* A chunk was already created - go to next element */
	    continue;
	}

	trimmed_nvp = bitd_nvp_trim(nvp, &elem_name, 1);
	
	if (chunked_nvp->n_elts == chunked_nvp->n_elts_allocated) {
	    chunked_nvp = 
		realloc(chunked_nvp, 
			sizeof(*chunked_nvp) + 
			2*(chunked_nvp->n_elts)*sizeof(bitd_nvp_element_t));
	    chunked_nvp->n_elts_allocated = 2*(chunked_nvp->n_elts);
	}
	
	/* Append the chunk */
	j = chunked_nvp->n_elts;
	chunked_nvp->e[j].name = elem_name ? strdup(elem_name) : NULL;
	chunked_nvp->e[j].v.value_nvp = trimmed_nvp;
	chunked_nvp->e[j].type = bitd_type_nvp;

	/* Update the nvp element count */
	chunked_nvp->n_elts = j+1;
    }

    return chunked_nvp;
} 


/*
 *============================================================================
 *                        bitd_nvp_unchunk
 *============================================================================
 * Description:     Reassembed an nvp from nvp chunks
 * Parameters:    
 * Returns:  
 */
bitd_nvp_t bitd_nvp_unchunk(bitd_nvp_t chunked_nvp) {
    bitd_nvp_t nvp = NULL, cnvp;
    int i, j;

    if (!chunked_nvp) {
	return NULL;
    }
    
    for (i = 0; i < chunked_nvp->n_elts; i++) {
	/* Chunks are only of nvp type */
	if (chunked_nvp->e[i].type != bitd_type_nvp) {
	    continue;
	}

	cnvp = chunked_nvp->e[i].v.value_nvp;
	if (!cnvp) {
	    /* Empty chunk */
	    continue;
	}
	
	for (j = 0; j < cnvp->n_elts; j++) {
	    bitd_nvp_add_elem(&nvp, 
			    cnvp->e[j].name, 
			    &cnvp->e[j].v, 
			    cnvp->e[j].type);
	}
    }

    return nvp;
} 


/*
 *============================================================================
 *                        bitd_nvp_merge
 *============================================================================
 * Description:     Merge the delta of (new_nvp, base_nvp) on top of nvp
 * Parameters:    
 *     old_nvp  - the current nvp
 *     new_nvp  - the new nvp
 *     base_nvp - the base nvp
 * Returns:  
 *     The newly-allocated nvp
 */
bitd_nvp_t bitd_nvp_merge(bitd_nvp_t old_nvp, bitd_nvp_t new_nvp, 
			  bitd_nvp_t base_nvp) {
    bitd_nvp_t chunked_old_nvp, chunked_new_nvp, chunked_base_nvp;
    bitd_nvp_t merged_nvp = NULL, chunked_merged_nvp = NULL;
    int i_old, i_new, i_base;
    bitd_boolean keep_old, keep_new;
    char *elem_name;

    /* Create the chunked nvps, to make merging easier */
    chunked_old_nvp = bitd_nvp_chunk(old_nvp);
    chunked_new_nvp = bitd_nvp_chunk(new_nvp);
    chunked_base_nvp = bitd_nvp_chunk(base_nvp);

    if (chunked_old_nvp) {
	for (i_old = 0; i_old < chunked_old_nvp->n_elts; i_old++) {
	    elem_name = chunked_old_nvp->e[i_old].name;
	    
	    keep_old = FALSE;
	    keep_new = FALSE;

	    if (bitd_nvp_lookup_elem(chunked_new_nvp, elem_name, &i_new)) {
		/* In the new - but has the new changed from the base? */
		if (bitd_nvp_lookup_elem(chunked_base_nvp, elem_name, &i_base) && 
		    !bitd_nvp_compare(chunked_new_nvp->e[i_new].v.value_nvp,
				    chunked_base_nvp->e[i_base].v.value_nvp)) {
		    keep_old = TRUE;
		} else {
		    /* Else, keep the new */
		    keep_new = TRUE;
		}
	    } else {
		/* Not in the new - but did it exist in the base? */
		if (!bitd_nvp_lookup_elem(chunked_base_nvp, elem_name, &i_base)) {
		    keep_old = TRUE;
		}
	    }

	    /* If this chunk has changed from the base, keep it */
	    if (keep_old) {
		/* Insert the old chunk in the chunked merged nvp */
		bitd_nvp_add_elem(&chunked_merged_nvp, 
				elem_name, 
				&chunked_old_nvp->e[i_old].v, 
				chunked_old_nvp->e[i_old].type);	    
	    }
	    
	    if (keep_new) {
		/* Insert the new chunk in the chunked merged nvp */
		bitd_nvp_add_elem(&chunked_merged_nvp, 
				elem_name, 
				&chunked_new_nvp->e[i_new].v, 
				chunked_new_nvp->e[i_new].type);	    
	    }
	}
    }

    
    /* All elements in nvp have been merged. Now merge the new_nvp
       elements not in nvp. as long as they are not in base_nvp */
    if (chunked_new_nvp) {
	for (i_new = 0; i_new < chunked_new_nvp->n_elts; i_new++) {
	    elem_name = chunked_new_nvp->e[i_new].name;
	    
	    /* If this chunk has changed from the base, keep it */
	    if (!bitd_nvp_lookup_elem(chunked_old_nvp, elem_name, NULL) &&
		!bitd_nvp_lookup_elem(chunked_base_nvp, elem_name, NULL)) {
		
		/* Insert the chunk in the chunked merged env */
		bitd_nvp_add_elem(&chunked_merged_nvp, 
				elem_name, 
				&chunked_new_nvp->e[i_new].v, 
				chunked_new_nvp->e[i_new].type);	    
	    }
	}
    }

    /* Un-chunk the merged nvp */
    merged_nvp = bitd_nvp_unchunk(chunked_merged_nvp);

    /* Release allocated memory */
    bitd_nvp_free(chunked_old_nvp);
    bitd_nvp_free(chunked_new_nvp);
    bitd_nvp_free(chunked_base_nvp);
    bitd_nvp_free(chunked_merged_nvp);

    return merged_nvp;
} 


/*
 *============================================================================
 *                        bitd_nvp_name_len_max
 *============================================================================
 * Description:     Compute the largest name length in the nvp
 * Parameters:    
 *     nvp - the nvp vector
 * Returns:  
 */
int bitd_nvp_name_len_max(bitd_nvp_t nvp) {
    int i, namelen_max;
    char *c;

    /* Find the max element name length */
    namelen_max = 0;
    for (i = 0 ; i < nvp->n_elts; i++) {
	c = nvp->e[i].name;
	if (c) {
	    namelen_max = MAX(namelen_max, strlen(c));
	}
    }
    
    return namelen_max;
} 


/*
 *============================================================================
 *                        bitd_nvp_to_string_size
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
int bitd_nvp_to_string_size(bitd_nvp_t v, int prefix_len) {
    int i, size = 0, namelen_max;
    char *c;

    if (!v) {
	/* Account for the NULL termination */
	return prefix_len + 1;
    }

    /* Find the max element name length */
    namelen_max = bitd_nvp_name_len_max(v);

    for (i = 0; i < v->n_elts; i++) {
	/* Estimate size of prefix, element name, colon, space pad, newline */
	size += (prefix_len + namelen_max + 3);

	/* Estimate size of value */
	switch (v->e[i].type) {
	case bitd_type_boolean:
	    size += sizeof("FALSE");
	    break;
	case bitd_type_int64:
	case bitd_type_uint64:
	    size += sizeof("-9223372036854775807");
	    break;
	case bitd_type_double:
	    size += sizeof("-0.98999999999999999");
	    break;
	case bitd_type_string:
	    /* Normalize the value */
	    c = v->e[i].v.value_string;
	    if (!c) {
		c = "";
	    }
	    size += (strlen(c) + 1);
	    break;
	case bitd_type_blob:
	    size += 2*bitd_blob_size(v->e[i].v.value_blob);
	    break;
	case bitd_type_nvp:
	    /* Recurse over the nvp. Reserve 2 extra spaces for prefix. */
	    if (v->e[i].v.value_nvp) {
		size += bitd_nvp_to_string_size(v->e[i].v.value_nvp, 
					      prefix_len + 2);
	    }
	    break;
	default:
	    break;	
	}
    }

    /* Account for the NULL termination */
    size += 1;
 
    return size;
} 


/*
 *============================================================================
 *                        bitd_nvp_to_string
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
char *bitd_nvp_to_string(bitd_nvp_t v, char *prefix) {
    char *buf = NULL, *c;
    int size, idx = 0, i, j, namelen, namelen_max;

    /* Normalize the prefix */
    if (!prefix) {
	prefix = "";
    }

    /* Estimate upper bound for buffer size. Add one to have space to check
       that idx < size, at the end. */
    size = bitd_nvp_to_string_size(v, strlen(prefix)) + 1;

    /* Allocate the buffer */
    buf = malloc(size+1);

    /* Initial NULL termination */
    buf[0] = 0;
    buf[size] = 0;

    if (!v) {
	return buf;
    }

    /* Find the max element name length */
    namelen_max = bitd_nvp_name_len_max(v);

    for (i = 0 ; i < v->n_elts; i++) {
	/* Normalize the element name */
	c = v->e[i].name;
	if (!c) {
	    c = "";
	}

	namelen = strlen(c);

	/* Print prefix, name, colon separator */
        idx += snprintf(buf + idx, size - idx, 
                        "%s%s:", 
                        prefix, c);
	
	/* Pad with spaces */
	for (j = 0; j < (namelen_max - namelen); j++) {
	    buf[idx++] = ' ';
	}

	/* One more trailing space */
	buf[idx++] = ' ';

	switch (v->e[i].type) {
	case bitd_type_void:
	case bitd_type_boolean:
	case bitd_type_int64:
	case bitd_type_uint64:
	case bitd_type_double:
	case bitd_type_string:
	case bitd_type_blob:
	    c = bitd_value_to_string(&v->e[i].v, v->e[i].type);
	    idx += snprintf(buf + idx, size - idx, "%s\n", c ? c : "");
	    free(c);	    
	    break;
	case bitd_type_nvp:
	    /* Recurse over the nvp. */
	    if (!v->e[i].v.value_nvp) {
		idx += snprintf(buf + idx, size - idx, "\n");
	    } else {
                char *prefix1 = malloc(strlen(prefix) + 3);
                sprintf(prefix1, "%s  ", prefix);

		c = bitd_nvp_to_string(v->e[i].v.value_nvp, prefix1);
		idx += snprintf(buf + idx, size - idx, "\n");
		idx += snprintf(buf + idx, size - idx, "%s", c);
		free(c);
		free(prefix1);
	    }
	    break;
	default:
	    break;	
	}
    }

    /* Sanity check */
    bitd_assert(idx < size);

    return buf;
}


/*
 *============================================================================
 *                        bitd_object_node_map
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
void bitd_object_node_map(object_node_t *node, 
			  object_node_map_func_t *f, 
			  void *cookie) {
    int i;
    bitd_nvp_t nvp;

    if (!node) {
	return;
    }

    if (node->a.type != bitd_type_nvp) {
	f(node, cookie);
    } else {
	nvp = node->a.v.value_nvp;
	if (!nvp) {
	    f(node, cookie);	    
	} else {
	    for (i = 0; i < nvp->n_elts; i++) {
		object_node_t nvp_node;
		bitd_nvp_element_t *e = &nvp->e[i];
		bitd_value_t *v = &e->v;
		char *full_name;
				
		/* Bear trap - remove me! */
		memset(&nvp_node, 0xab, sizeof(nvp_node));

		/* Copy and normalize the name */
		nvp_node.name = e->name;
		if (!nvp_node.name) {
		    nvp_node.name = "";
		}

		/* Normalize the node full name */
		full_name = node->full_name;
		if (!full_name) {
		    full_name = "";
		}
		
		/* Create the nvp_node full name */
		nvp_node.full_name = malloc(strlen(full_name) + 
					    strlen(nvp_node.name) + 2);
		if (full_name[0]) {
		    sprintf(nvp_node.full_name, "%s%c%s", 
			    full_name, 
			    node->sep, 
			    nvp_node.name);
		} else {
		    sprintf(nvp_node.full_name, "%s", 
			    nvp_node.name);
		}
		
		nvp_node.sep = node->sep;
		nvp_node.depth = node->depth + 1;
		nvp_node.a.type = e->type;
		nvp_node.a.v = *v;

		bitd_object_node_map(&nvp_node, f, cookie);
		
		/* Release memory */
		free(nvp_node.full_name);
	    }
	}
    }
} 


/*
 *============================================================================
 *                        bitd_object_map
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
void bitd_object_map(char *name, char sep, int depth, bitd_object_t *a, 
		     object_map_func_t *f, void *cookie) {

    /* Normalize the name */
    if (!name) {
	name = "";
    }

    if (a) {
	bitd_value_map(name, sep, depth, &a->v, a->type, f, cookie);
    }
} 


/*
 *============================================================================
 *                        bitd_nvp_map
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
void bitd_nvp_map(char *name, char sep, int depth, bitd_nvp_t nvp, 
		  object_map_func_t *f, void *cookie) {
    bitd_value_t v;
    int i;

    /* Normalize the name */
    if (!name) {
	name = "";
    }

    if (!nvp) {
	v.value_nvp = NULL;

	f(cookie, name, name, depth, &v, bitd_type_nvp);
    } else {
	for (i = 0; i < nvp->n_elts; i++) {
	    bitd_nvp_element_t *e = &nvp->e[i];
	    bitd_value_t *v = &e->v;
	    bitd_type_t type = e->type;
	    char *elem_name = e->name;
	    char *full_name;

	    /* Normalize the name */
	    if (!elem_name) {
		elem_name = "";
	    }

	    full_name = malloc(strlen(name) + strlen(elem_name) + 2);
	    sprintf(full_name, "%s%c%s", name, sep, elem_name);

	    if (type != bitd_type_nvp ||
		(type == bitd_type_nvp && !v->value_nvp)) {
		f(cookie, full_name, elem_name, depth + 1, v, type);
	    } else {
		bitd_nvp_map(full_name, sep, depth + 1, v->value_nvp, f, cookie);
	    }
	    
	    free(full_name);
	}
    }
} 


/*
 *============================================================================
 *                        bitd_value_map
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
void bitd_value_map(char *name, char sep, int depth, 
		    bitd_value_t *v, bitd_type_t type, 
		    object_map_func_t *f, void *cookie) {

    /* Normalize the name */
    if (!name) {
	name = "";
    }

    if (type != bitd_type_nvp ||
	(type == bitd_type_nvp && !v->value_nvp)) {
	f(cookie, name, name, depth, v, type);
    } else {
	bitd_nvp_map(name, sep, depth, v->value_nvp, f, cookie);
    }
} 


/*
 *============================================================================
 *                        bitd_buffer_to_object
 *============================================================================
 * Description:  Convert buffer to object, according to buffer_type parameter
 * Parameters:    
 *     a [OUT]    - the resulting object
 *     object_name [OUT] - the resulting object name, for buffers in xml format
 *     buf        - the input buffer
 *     buf_nbytes - length of buf
 *     buffer_type - determines conversion format
 * Returns:  
 */
void bitd_buffer_to_object(bitd_object_t *a, 
			   char **object_name,
			   char *buf, int buf_nbytes,
			   bitd_buffer_type_t buffer_type) {
    bitd_boolean ret;
    register char *c;
    register int i;

    /* Parameter check */
    if (!a) {
	bitd_assert(0);
	return;
    }

    /* Initialize the OUT parameters */
    bitd_object_init(a);
    if (object_name) {
	*object_name = NULL;
    }

    /* Parameter check */
    if (!buf || !buf_nbytes) {
	return;
    }

    if (buffer_type == bitd_buffer_type_auto) {
	/* Detect the buffer type */
	ret = bitd_json_to_object(a, buf, buf_nbytes, NULL, 0);
	if (ret) {
	    /* Detected json - object is already converted */
	    return;
	}
	ret = bitd_xml_to_object(a, object_name, buf, buf_nbytes, NULL, 0);
	if (ret) {
	    /* Detected xml - object is already converted */
	    return;
	}
	ret = bitd_yaml_to_object(a, buf, buf_nbytes, NULL, NULL, 0);
	if (ret) {
	    /* Detected yaml - object is already converted */
	    return;
	}

	/* Does the buffer contain any zero character? */
	for (i = 0, c = buf; *c; i++, c++) {
	    if (!c) {
		break;
	    }
	}
	
	if (i == buf_nbytes) {
	    buffer_type = bitd_buffer_type_string;
	} else {
	    buffer_type = bitd_buffer_type_blob;
	}
    }

    switch (buffer_type) {
    case bitd_buffer_type_string:
	/* Convert to string */
	a->type = bitd_type_string;
	a->v.value_string = malloc(buf_nbytes + 1);
	memcpy(a->v.value_string, buf, buf_nbytes);
	a->v.value_string[buf_nbytes] = 0;
	break;
    case bitd_buffer_type_blob:
	/* Convert to blob */
	a->type = bitd_type_blob;
	a->v.value_blob = malloc(sizeof(bitd_blob) + buf_nbytes);
	bitd_blob_size(a->v.value_blob) = buf_nbytes;
	memcpy(bitd_blob_payload(a->v.value_blob), buf, buf_nbytes);
	break;
    case bitd_buffer_type_json:
	bitd_json_to_object(a, buf, buf_nbytes, NULL, 0);
	break;
    case bitd_buffer_type_xml:
	bitd_xml_to_object(a, object_name, buf, buf_nbytes, NULL, 0);
	break;
    case bitd_buffer_type_yaml:
	bitd_yaml_to_object(a, buf, buf_nbytes, NULL, NULL, 0);
	break;
    default:
	bitd_assert(0);
    }
} 


/*
 *============================================================================
 *                        bitd_buffer_to_object
 *============================================================================
 * Description:  Print object into buffer, according to buffer_type parameter
 * Parameters:    
 *     buf [OUT]  - the output buffer
 *     buf_nbytes [OUT] - length of buf
 *     a - the input object
 *     object_name - the input object name, for buffers output in xml format
 *     buffer_type - determines conversion format
 * Returns:  
 */
void bitd_object_to_buffer(char **buf, int *buf_nbytes,
			   bitd_object_t *a, 
			   char *object_name,
			   bitd_buffer_type_t buffer_type) {

    /* Initialize OUT parameters */
    if (buf) {
	*buf = NULL;
    }
    if (buf_nbytes) {
	*buf_nbytes = 0;
    }

    /* OUT parameter check */
    if (!buf || !buf_nbytes) {
	return;
    }

    /* Parameter check */
    if (!a || a->type == bitd_type_void) {
	return;
    }
    
    if (buffer_type == bitd_buffer_type_xml) {
	*buf = bitd_object_to_xml(a, object_name, FALSE);
	*buf_nbytes = strlen(*buf);
	return;
    } 

    if (buffer_type == bitd_buffer_type_json) {
	*buf = bitd_object_to_json(a, FALSE, FALSE);
	*buf_nbytes = strlen(*buf);
	return;
    } 

    /* Convert the auto type */
    if (buffer_type == bitd_buffer_type_auto) {
	if (a->type == bitd_type_nvp) {
	    buffer_type = bitd_buffer_type_yaml;
	} else if (a->type == bitd_type_blob) {
	    buffer_type = bitd_buffer_type_blob;
	} else {
	    buffer_type = bitd_buffer_type_string;
	}
    }

    /* Convert nvp objects as yaml */
    if (buffer_type == bitd_buffer_type_string ||
	buffer_type == bitd_buffer_type_blob) {
	if (a->type == bitd_type_nvp) {
	    /* Convert buffer type to yaml */
	    buffer_type = bitd_buffer_type_yaml;
	}
    }

    if (buffer_type == bitd_buffer_type_yaml) {
	*buf = bitd_object_to_yaml(a, FALSE, FALSE);
	*buf_nbytes = strlen(*buf);
    } else if (a->type == bitd_type_blob) {
	*buf_nbytes = bitd_blob_size(a->v.value_blob);
	*buf = malloc(*buf_nbytes);
	memcpy(*buf, 
	       bitd_blob_payload(a->v.value_blob), 
	       *buf_nbytes);
    } else {
	*buf = bitd_object_to_string(a);
	*buf_nbytes = strlen(*buf);
    }
}
