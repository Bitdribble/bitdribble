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
void bitd_json_elem_to_nvp(bitd_nvp_t *nvp, char *jkey, json_t *jelem, 
			   bitd_boolean is_root);


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
 * Description:     Convert object to json buffer
 * Parameters:    
 *     a - pointer to the object to be converted
 *     full_json - append _!!<type> to the label names to determine type
 *     single_line_json - print the json buffer on a single line
 * Returns:  
 *     Heap-allocated buffer containing the json buffer
 */
char *bitd_object_to_json(bitd_object_t *a,
			  bitd_boolean full_json,
			  bitd_boolean single_line_json) {
    char *buf = NULL, *buf1;
    int size = 0, idx = 0;
    
    bitd_assert(a);

    if (a->type != bitd_type_nvp) {
	return NULL;
    }

    buf1 = bitd_object_to_json_element(a, 0, full_json, single_line_json);

    /* Buffer auto-allocated inside snprintf_w_realloc() */
    snprintf_w_realloc(&buf, &size, &idx,
		       "%s%s", buf1, 
		       single_line_json ? "" : "\n");
    free(buf1);

    return buf;
}


/*
 *============================================================================
 *                        bitd_object_to_json_element
 *============================================================================
 * Description:     Convert object to an element of json. Nvps are converted
 *     to actual json buffers, and all other types are converted to json
 *     values that can be reused to format a json buffer.
 * Parameters:    
 *     a - pointer to the object to be converted
 *     indentation - how many spaces to indent the buffer, in case the object 
 *         is of nvp type. For non-nvp-type objects, the output is not indented.
 *     full_json - append _!!<type> to the label names to determine type
 *     single_line_json - print the json buffer on a single line
 * Returns:  
 *     Heap-allocated buffer containing the json buffer
 */
char *bitd_object_to_json_element(bitd_object_t *a,
				  int indentation, /* How much to indent */
				  bitd_boolean full_json,
				  bitd_boolean single_line_json) {
    char *buf = NULL;
    int size = 0, idx = 0, i;
    char *prefix;
    char *value_str = NULL;

    bitd_assert(indentation >= 0);

    if (single_line_json) {
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

	if ((a->type == bitd_type_uint64 && a->v.value_uint64 > LONG_MAX) ||
	    a->type == bitd_type_string || 
	    a->type == bitd_type_blob) {
	    /* Jansson can't parse numbers larger than LONG_MAX. The solution
	       is to write them as strings, appending _!!uint64 to the label,
	       which makes us parse it back as uint64 instead of string. */
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
			       single_line_json ? "" : "\n");
	    for (i = 0; i < n_elts; i++) {		    
		bitd_object_t a1;
		
		a1.v = a->v.value_nvp->e[i].v;
		a1.type = a->v.value_nvp->e[i].type;
		
		value_str = bitd_object_to_json_element(&a1, 
							indentation + 2,
							full_json,
							single_line_json);
		if (single_line_json) {
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
				       single_line_json ? "" : "\n");
		} else {
		    snprintf_w_realloc(&buf, &size, &idx, "%s",
				       single_line_json ? "" : "\n");
		}
	    } 
	    snprintf_w_realloc(&buf, &size, &idx, "%s]", prefix);
	} else {
	    /* A json block */	    
	    snprintf_w_realloc(&buf, &size, &idx, "{%s",
			       single_line_json ? "" : "\n");
	    for (i = 0; i < n_elts; i++) {		    
		bitd_object_t a1;
		
		a1.v = a->v.value_nvp->e[i].v;
		a1.type = a->v.value_nvp->e[i].type;
		
		value_str = escape_to_json(a->v.value_nvp->e[i].name);
		if (!value_str) {
		    value_str = strdup("");
		}

		if (full_json || 
		    (a1.type == bitd_type_uint64 && a1.v.value_uint64 > LONG_MAX) ||
		    a1.type == bitd_type_blob) {
		    /* Jansson can't parse numbers larger than LONG_MAX. 
		       The solution is to write them as strings, appending 
		       _!!uint64 to the label, which makes us parse it back
		       as uint64 instead of string. */
		    snprintf_w_realloc(&buf, &size, &idx,
				       "%s%s\"%s_!!%s\":%s", 
				       prefix, 
				       single_line_json ? "" : "  ",
				       value_str, 
				       bitd_get_type_name(a1.type),
				       single_line_json ? "" : " ");
		} else {
		    snprintf_w_realloc(&buf, &size, &idx,
				       "%s%s\"%s\":%s", 
				       prefix, 
				       single_line_json ? "" : "  ",
				       value_str,
				       single_line_json ? "" : " ");
		}

		free(value_str);

		value_str = bitd_object_to_json_element(&a1, 
							indentation + 2,
							full_json,
							single_line_json);
		snprintf_w_realloc(&buf, &size, &idx,
				   "%s", value_str);
		if (value_str) {
		    free(value_str);
		}
		value_str = NULL;
		if (i < n_elts - 1) {
		    snprintf_w_realloc(&buf, &size, &idx, ",%s",
				       single_line_json ? "" : "\n");
		} else {
		    snprintf_w_realloc(&buf, &size, &idx, "%s",
				       single_line_json ? "" : "\n");
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


/*
 *============================================================================
 *                        bitd_nvp_to_json
 *============================================================================
 * Description:     Convert nvp to json buffer
 * Parameters:    
 *     nvp - the nvp to be converted
 *     full_json - append _!!<type> to the label names to determine type
 *     single_line_json - print the json buffer on a single line
 * Returns:  
 *     Heap-allocated buffer containing the json buffer
 */
char *bitd_nvp_to_json(bitd_nvp_t nvp,
		       bitd_boolean full_json,
		       bitd_boolean single_line_json) {
    bitd_object_t a;

    a.type = bitd_type_nvp;
    a.v.value_nvp = nvp;

    return bitd_object_to_json(&a, full_json, single_line_json);
}


/*
 *============================================================================
 *                        bitd_json_to_object
 *============================================================================
 * Description:     Parse json buffer into object
 * Parameters:    
 *     a [OUT] - pointer to the resulting object
 *     json - the json buffer
 *     json_nbytes - size of the json buffer
 *     err_buf [IN/OUT] - the parse error message, if any
 *     err_len - the length of the passed-in err_buf
 * Returns:  
 *     TRUE on successful parse
 */
bitd_boolean bitd_json_to_object(bitd_object_t *a, 
				 char *json, int json_nbytes,
				 char *err_buf,
				 int err_len) {
    bitd_boolean ret;
    bitd_nvp_t nvp = NULL;

    ret = bitd_json_to_nvp(&nvp, json, json_nbytes, err_buf, err_len);
    if (ret) {
	if (a) {
	    a->type = bitd_type_nvp;
	    a->v.value_nvp = nvp;
	    nvp = NULL;
	}
    }

    bitd_nvp_free(nvp);
    return ret;
}


/*
 *============================================================================
 *                        bitd_json_to_nvp
 *============================================================================
 * Description:     Parse json buffer into nvp
 * Parameters:    
 *     nvp [OUT] - pointer to the resulting nvp
 *     json - the json buffer
 *     json_nbytes - size of the json buffer
 *     err_buf [IN/OUT] - the parse error message, if any
 *     err_len - the length of the passed-in err_buf
 * Returns:  
 *     TRUE on successful parse
 */
bitd_boolean bitd_json_to_nvp(bitd_nvp_t *nvp, 
			      char *json, int json_nbytes,
			      char *err_buf,
			      int err_len) {
    json_t *jroot = NULL;
    json_error_t jerror;

    /* Initialize OUT parameters */
    if (nvp) {
	*nvp = NULL;
    }
    if (err_buf) {
	err_buf[0] = 0;
    }

    /* Parse the json */
    jroot = json_loadb(json, json_nbytes, 0, &jerror);
    if (!jroot) {
	if (err_buf && err_len) {
	    snprintf(err_buf, err_len - 1, 
		     "Json parse error at line %d: %s\n", 
		     jerror.line, jerror.text);
	    err_buf[err_len] = 0;
	}
        return FALSE;
    }

    /* Convert the root element */
    bitd_json_elem_to_nvp(nvp, NULL, jroot, TRUE);

    /* Release the json object */
    json_decref(jroot);

    return TRUE;
}



/*
 *============================================================================
 *                        bitd_json_elem_to_nvp
 *============================================================================
 * Description:     Convert json element to nvp. The nvp may have preallocated
 *    elements, in which case the new element is added at the end.
 * Parameters:    
 *     nvp [IN/OUT] - This nvp will be updated with the converted json element
 *     elem - The json element
 * Returns:  
 */
void bitd_json_elem_to_nvp(bitd_nvp_t *nvp, char *jkey, json_t *jelem, 
			   bitd_boolean is_root) {
    bitd_value_t v;
    bitd_type_t type;
    int jtype;    
    char *jkey1;
    json_t *jvalue1;
    int jindex1;

    jtype = json_typeof(jelem);

    switch (jtype) {
    case JSON_NULL:
	type = bitd_type_void;
	break;
    case JSON_TRUE:
	v.value_boolean = TRUE;
	type = bitd_type_boolean;
	break;
    case JSON_FALSE:
	v.value_boolean = FALSE;
	type = bitd_type_boolean;
	break;
    case JSON_INTEGER:
	v.value_int64 = json_integer_value(jelem);
	type = bitd_type_int64;
	break;
    case JSON_REAL:
	v.value_double = json_real_value(jelem);
	type = bitd_type_double;
	break;
    case JSON_STRING:
	v.value_string = (char *)json_string_value(jelem);
	type = bitd_type_string;
	break;
    case JSON_OBJECT:
	if (is_root) {
	    json_object_foreach(jelem, jkey1, jvalue1) {
		bitd_json_elem_to_nvp(nvp, jkey1, jvalue1, FALSE);
	    }
	    return;
	} else {
	    v.value_nvp = bitd_nvp_alloc(4);
	    type = bitd_type_nvp;
	    json_object_foreach(jelem, jkey1, jvalue1) {
		bitd_json_elem_to_nvp(&v.value_nvp, jkey1, jvalue1, FALSE);
	    }
	    /* Fallthrough to add v.value_nvp to the nvp */
	}
	break;
    case JSON_ARRAY:
	v.value_nvp = bitd_nvp_alloc(4);
	type = bitd_type_nvp;
	json_array_foreach(jelem, jindex1, jvalue1) {
	    bitd_json_elem_to_nvp(&v.value_nvp, NULL, jvalue1, FALSE);
	}
	/* Fallthrough to add v.value_nvp to the nvp */
	break;
    default:
	type = bitd_type_void;
    }
    
    /* Add the object to the higher nvp */
    bitd_nvp_add_elem(nvp, 
		      jkey, 
		      &v, 
		      type);
    
    if (type == bitd_type_nvp) {
	bitd_nvp_free(v.value_nvp);
    }
}
