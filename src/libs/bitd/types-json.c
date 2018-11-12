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
static char *escape_to_xml(char *s);



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
			  bitd_boolean full_xml) {
    char *buf = NULL, *buf1;
    int size = 0, idx = 0;
    char *object_name = NULL;
    
    bitd_assert(a);

    if (!object_name || !object_name[0]) {
	object_name = "_";
    }

    /* This makes object_name heap allocated */
    object_name = escape_to_json(object_name);

    buf1 = bitd_object_to_xml_element(a, object_name, 0, full_xml);

    /* Buffer auto-allocated inside snprintf_w_realloc() */
    snprintf_w_realloc(&buf, &size, &idx,
		       "%s", buf1);
    free(buf1);
    

    free(object_name);

    /* NULL termination for buffer */
    if (idx == size) {
	buf = realloc(buf, size+1);
    }
    buf[idx] = 0;

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
				  bitd_boolean full_xml) {
    char *buf = NULL;
    int size = 0, idx = 0, i;
    char *prefix;
    char *value_str = NULL;
    bitd_type_t t;
    bitd_boolean full_string_xml;
    char *object_name = NULL;

    bitd_assert(a);
    bitd_assert(indentation >= 0);

    if (!object_name) {
	object_name = "_";
    }

    /* Escape the object name for xml. This will allocate it on the heap. */
    object_name = escape_to_json(object_name);

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

    if (a->type != bitd_type_void &&
	a->type != bitd_type_nvp) {
	/* Get the value in string form */
	value_str = bitd_value_to_string(&a->v, a->type);
    }
    
    /* Buffer auto-allocated inside snprintf_w_realloc() */
    switch (a->type) {
	case bitd_type_void:
	    if (full_xml) {
		snprintf_w_realloc(&buf, &size, &idx,
				   "%s<%s type='void'/>\n", 
				   prefix, object_name);
	    } else {
		snprintf_w_realloc(&buf, &size, &idx,
				   "%s<%s/>\n", 
				   prefix, object_name);
	    }
	    break;
	case bitd_type_boolean:
	    if (full_xml) {
		snprintf_w_realloc(&buf, &size, &idx,
				   "%s<%s type='boolean'>%s</%s>\n", 
				   prefix, object_name, 
				   value_str, object_name);
	    } else {
		snprintf_w_realloc(&buf, &size, &idx,
				   "%s<%s>%s</%s>\n", 
				   prefix, object_name, value_str, object_name);
	    }
	    break;
	case bitd_type_int64:
	case bitd_type_uint64:
	case bitd_type_double:
	    /* Skip string - handled below */
	case bitd_type_blob:
	    if (full_xml || a->type == bitd_type_blob) {
		snprintf_w_realloc(&buf, &size, &idx,
				   "%s<%s type='%s'>%s</%s>\n", 
				   prefix, object_name, 
				   bitd_get_type_name(a->type),
				   value_str, object_name);
	    } else {
		snprintf_w_realloc(&buf, &size, &idx,
				   "%s<%s>%s</%s>\n", 
				   prefix, object_name, 
				   value_str, object_name);
	    }
	    break;
	case bitd_type_string:
	    /* We should print the type in full if full_xml is set, but also
	       if the string could be interpreted as a void, boolean, int64,
	       uint64 or double */
	    full_string_xml = full_xml;
	    bitd_string_to_type_and_value(&t, NULL, value_str);
	    if (t != bitd_type_string) {
		full_string_xml = TRUE;
	    }

	    /* Escape the value string for xml */
	    {
		char *c = value_str;
		value_str = escape_to_json(c);
		free(c);
	    }

	    if (full_string_xml) {
		snprintf_w_realloc(&buf, &size, &idx,
				   "%s<%s type='%s'", 
				   prefix, object_name, 
				   bitd_get_type_name(a->type));
	    } else {
		snprintf_w_realloc(&buf, &size, &idx,
				   "%s<%s", 
				   prefix, object_name);
	    }
	    if (value_str && value_str[0]) {
		snprintf_w_realloc(&buf, &size, &idx,
				   ">%s</%s>\n", 
				   value_str, object_name);
	    } else {
		snprintf_w_realloc(&buf, &size, &idx,
				   "/>\n");
	    }
	    break;
	case bitd_type_nvp:
	    snprintf_w_realloc(&buf, &size, &idx,
			       "%s<%s", 
			       prefix, object_name);

	    if (full_xml || !a->v.value_nvp) {
		snprintf_w_realloc(&buf, &size, &idx,
				   " type='%s'", 
				   bitd_get_type_name(a->type));
	    }
	    /* Recurse over the nvp. */
	    if (!a->v.value_nvp) {
		snprintf_w_realloc(&buf, &size, &idx,
				   "/>\n");
	    } else {
		int n_elts = a->v.value_nvp->n_elts;
		snprintf_w_realloc(&buf, &size, &idx, ">\n");
		for (i = 0; i < n_elts; i++) {		    
		    bitd_object_t a1;

		    a1.v = a->v.value_nvp->e[i].v;
		    a1.type = a->v.value_nvp->e[i].type;

		    if (value_str) {
			free(value_str);
		    }
		    value_str = bitd_object_to_xml_element(&a1, 
							 a->v.value_nvp->e[i].name,
							 indentation + 2,
							 full_xml);
		    snprintf_w_realloc(&buf, &size, &idx,
				       "%s", value_str);
		    free(value_str);
		    value_str = NULL;
		}
		snprintf_w_realloc(&buf, &size, &idx,
				   "%s</%s>\n", 
				   prefix, object_name);
	    }
	    break;
	default:
	    break;	
	
    }

    /* NULL termination for buffer */
    if (idx == size) {
	buf = realloc(buf, size+1);
    }
    buf[idx] = 0;

    free(prefix);
    free(object_name);
    if (value_str) {
	free(value_str);
    }

    return buf;
}


