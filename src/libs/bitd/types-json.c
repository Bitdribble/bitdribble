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
#include "bitd/format.h"

#include <jansson.h>


/*****************************************************************************
 *                             MANIFEST CONSTANTS
 *****************************************************************************/



/*****************************************************************************
 *                                  MACROS 
 *****************************************************************************/
#define dbg_printf if (0) printf



/*****************************************************************************
 *                                  TYPES
 *****************************************************************************/



/*****************************************************************************
 *                           FUNCTION DECLARATION
 *****************************************************************************/
static char *escape_to_json(char *s);



/*****************************************************************************
 *                                VARIABLES
 *****************************************************************************/



/*****************************************************************************
 *                          FUNCTION IMPLEMENTATION
 *****************************************************************************/


/*
 *============================================================================
 *                        escape_to_json
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
char *escape_to_json(char *s) {
    char *s1;
    int len, idx, len1, idx1;

    if (!s) {
	return NULL;
    }

    len = strlen(s) + 1;
    len1 = 2*len;

    s1 = malloc(len1);

    for (idx = 0, idx1 = 0; s[idx]; idx++) {
	if (s[idx] == '\b') {
	    s1[idx1++] = '\\'; 
	    s1[idx1++] = 'b'; 
	} else if (s[idx] == '\f') {
	    s1[idx1++] = '\\'; 
	    s1[idx1++] = 'f'; 
	} else if (s[idx] == '\n') {
	    s1[idx1++] = '\\'; 
	    s1[idx1++] = 'n'; 
	} else if (s[idx] == '\r') {
	    s1[idx1++] = '\\'; 
	    s1[idx1++] = 'r'; 
	} else if (s[idx] == '\t') {
	    s1[idx1++] = '\\'; 
	    s1[idx1++] = 't'; 
	} else if (s[idx] == '"') {
	    s1[idx1++] = '\\'; 
	    s1[idx1++] = '"'; 
	} else if (s[idx] == '\\') {
	    s1[idx1++] = '\\'; 
	    s1[idx1++] = '\\'; 
	} else {
	    s1[idx1++] = s[idx]; 
	}
    }

    /* NULL termination */
    s1[idx1] = 0;

    return s1;
} 


/*
 *============================================================================
 *                        bitd_object_to_json
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
char *bitd_object_to_json(bitd_object_t *a,
			  bitd_boolean full_json,
			  bitd_boolean compressed_json) {
    char *buf = NULL, *buf1;
    int size = 0, idx = 0;
    
    bitd_assert(a);

    if (a->type != bitd_type_nvp) {
	return NULL;
    }

    buf1 = bitd_object_to_json_element(a, 0, full_json, compressed_json);

    /* Buffer auto-allocated inside snprintf_w_realloc() */
    snprintf_w_realloc(&buf, &size, &idx,
		       "%s%s", buf1, 
		       compressed_json ? "" : "\n");
    free(buf1);

    return buf;
}


/*
 *============================================================================
 *                        bitd_object_to_json_element
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
char *bitd_object_to_json_element(bitd_object_t *a,
				  int indentation, /* How much to indent */
				  bitd_boolean full_json,
				  bitd_boolean compressed_json) {
    char *buf = NULL;
    int size = 0, idx = 0, i;
    char *prefix;
    char *value_str = NULL;

    bitd_assert(indentation >= 0);

    if (compressed_json) {
	/* Cancel the indentation*/
	indentation = 0;
    }

    /* Allocate and format the prefix */
    prefix = malloc(indentation + 1);
    {
	register char *c = prefix;
	register int i;

	for (i = 0; i < indentation; i++) {
	    *c++ = ' ';
	}
	*c = 0;
    }

    if (a->type == bitd_type_void) {
	snprintf_w_realloc(&buf, &size, &idx, "null");
    } else if (a->type == bitd_type_boolean) {
	snprintf_w_realloc(&buf, &size, &idx, "%s", 
			   a->v.value_boolean ? "true" : "false");
    } else if (a->type == bitd_type_string &&
	       !a->v.value_string) {
	snprintf_w_realloc(&buf, &size, &idx, "null");
    } else if (a->type != bitd_type_nvp) {
	/* Get the value in string form */
	value_str = bitd_value_to_string(&a->v, a->type);

	if (a->type == bitd_type_string || 
	    a->type == bitd_type_blob) {
	    snprintf_w_realloc(&buf, &size, &idx, "\"%s\"", value_str);
	} else {
	    snprintf_w_realloc(&buf, &size, &idx, "%s", value_str);
	}
	free(value_str);
	value_str = NULL;
    } else if (!a->v.value_nvp || !a->v.value_nvp->n_elts) {
	/* The empty NVP case */
	snprintf_w_realloc(&buf, &size, &idx, "{}");
    } else {
	/* The non-empty NVP case */
	int n_elts = a->v.value_nvp->n_elts;
	bitd_boolean is_block = FALSE;
	char *name;
	
	/* Block or sequence? */
	for (i = 0; i < n_elts; i++) {
	    name = a->v.value_nvp->e[i].name;
	    if (name && name[0]) {
		is_block = TRUE;
		break;
	    }
	}

	if (!is_block) {
	    /* A json sequence */
	    snprintf_w_realloc(&buf, &size, &idx, "[%s",
			       compressed_json ? "" : "\n");
	    for (i = 0; i < n_elts; i++) {		    
		bitd_object_t a1;
		
		a1.v = a->v.value_nvp->e[i].v;
		a1.type = a->v.value_nvp->e[i].type;
		
		value_str = bitd_object_to_json_element(&a1, 
							indentation + 2,
							full_json,
							compressed_json);
		if (compressed_json) {
		    snprintf_w_realloc(&buf, &size, &idx,
				       "%s", value_str);
		} else {
		    snprintf_w_realloc(&buf, &size, &idx,
				       "%s  %s", prefix, value_str);
		}
		if (value_str) {
		    free(value_str);
		}
		value_str = NULL;
		if (i < n_elts - 1) {
		    snprintf_w_realloc(&buf, &size, &idx, ",%s",
				       compressed_json ? "" : "\n");
		} else {
		    snprintf_w_realloc(&buf, &size, &idx, "%s",
				       compressed_json ? "" : "\n");
		}
	    } 
	    snprintf_w_realloc(&buf, &size, &idx, "%s]", prefix);
	} else {
	    /* A json block */	    
	    snprintf_w_realloc(&buf, &size, &idx, "{%s",
			       compressed_json ? "" : "\n");
	    for (i = 0; i < n_elts; i++) {		    
		bitd_object_t a1;
		
		a1.v = a->v.value_nvp->e[i].v;
		a1.type = a->v.value_nvp->e[i].type;
		
		value_str = escape_to_json(a->v.value_nvp->e[i].name);
		if (!value_str) {
		    value_str = strdup("");
		}

		if (full_json || a1.type == bitd_type_blob) {
		    snprintf_w_realloc(&buf, &size, &idx,
				       "%s%s\"%s_!!%s\":%s", 
				       prefix, 
				       compressed_json ? "" : "  ",
				       value_str, 
				       bitd_get_type_name(a1.type),
				       compressed_json ? "" : " ");
		} else {
		    snprintf_w_realloc(&buf, &size, &idx,
				       "%s%s\"%s\":%s", 
				       prefix, 
				       compressed_json ? "" : "  ",
				       value_str,
				       compressed_json ? "" : " ");
		}

		free(value_str);

		value_str = bitd_object_to_json_element(&a1, 
							indentation + 2,
							full_json,
							compressed_json);
		snprintf_w_realloc(&buf, &size, &idx,
				   "%s", value_str);
		if (value_str) {
		    free(value_str);
		}
		value_str = NULL;
		if (i < n_elts - 1) {
		    snprintf_w_realloc(&buf, &size, &idx, ",%s",
				       compressed_json ? "" : "\n");
		} else {
		    snprintf_w_realloc(&buf, &size, &idx, "%s",
				       compressed_json ? "" : "\n");
		}
	    }
	    snprintf_w_realloc(&buf, &size, &idx, "%s}", prefix);
	}
    }

    /* NULL termination for buffer */
    if (idx == size) {
	buf = realloc(buf, size+1);
    }
    buf[idx] = 0;

    free(prefix);
    if (value_str) {
	free(value_str);
    }

    return buf;
}


