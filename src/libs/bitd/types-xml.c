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

#include <ctype.h>
#include <expat.h>


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
struct xml_user_data {
    struct xml_user_data *next; /* User data for child block */
    struct xml_user_data *prev; /* User data for enclosing block */
    int depth;                  /* Always 3rd */
    struct bitd_xml_stream_s *s;  /* Points back to stream. Always 4th. */
    /* From this point down, struct members are valid only if depth > 0 */
    char *name;
    char *value_str;
    int value_len;
    bitd_object_t object; /* The object being parsed at this level */
    bitd_boolean object_type_set_p;
};

/* Nvp xml stream object */
struct bitd_xml_stream_s {
    struct xml_user_data *head; /* Head and tail of user data list */
    struct xml_user_data *tail;
    int depth;                  /* Always 3rd */
    struct bitd_xml_stream_s *s;  /* Points back to stream. Always 4th. */
    char *object_name;
    bitd_object_t object;         /* The object representing the entire xml */
    XML_Parser p;
    char *error_str;
};

#define USER_DATA_HEAD(s) \
    ((struct xml_user_data *)&(s)->head)


/*****************************************************************************
 *                           FUNCTION DECLARATION
 *****************************************************************************/
static char *escape_to_xml(char *s);
static char *xml_error(char *fmt, ...);
static void free_user_data(struct xml_user_data *u);

/*****************************************************************************
 *                                VARIABLES
 *****************************************************************************/



/*****************************************************************************
 *                          FUNCTION IMPLEMENTATION
 *****************************************************************************/


/*
 *============================================================================
 *                        escape_to_xml
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
char *escape_to_xml(char *s) {
    char *s1;
    int len, idx, len1, idx1;

    if (!s) {
	return NULL;
    }

    len = strlen(s);
    len1 = 6*len;

    s1 = malloc(len1);

    for (idx = 0, idx1 = 0; s[idx]; idx++) {
	if (s[idx] == '<') {
	    s1[idx1++] = '&'; 
	    s1[idx1++] = 'l'; 
	    s1[idx1++] = 't'; 
	    s1[idx1++] = ';'; 
	} else if (s[idx] == '>') {
	    s1[idx1++] = '&'; 
	    s1[idx1++] = 'g'; 
	    s1[idx1++] = 't'; 
	    s1[idx1++] = ';'; 
	} else if (s[idx] == '"') {
	    s1[idx1++] = '&'; 
	    s1[idx1++] = 'q'; 
	    s1[idx1++] = 'u'; 
	    s1[idx1++] = 'o'; 
	    s1[idx1++] = 't'; 
	    s1[idx1++] = ';'; 
	} else if (s[idx] == '\'') {
	    s1[idx1++] = '&'; 
	    s1[idx1++] = 'a'; 
	    s1[idx1++] = 'p'; 
	    s1[idx1++] = 'o'; 
	    s1[idx1++] = 's'; 
	    s1[idx1++] = ';'; 
	} else if (s[idx] == '&') {
	    s1[idx1++] = '&'; 
	    s1[idx1++] = 'a'; 
	    s1[idx1++] = 'm'; 
	    s1[idx1++] = 'p'; 
	    s1[idx1++] = ';'; 
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
 *                        xml_error
 *============================================================================
 * Description:     Allocate an error message on the heap, and return
 *           the pointer.
 * Parameters:    
 * Returns:  The error message. Must be freed by caller.
 */
char *xml_error(char *fmt, ...) {
    char *buf;
    int size = 1024, idx;
    va_list args;
    
    va_start(args, fmt);

    buf = malloc(size);
    idx = vsnprintf(buf, size - 1, fmt, args);
    
    /* Null termination */
    buf[idx] = 0;

    va_end(args);

    return buf;
} 


/*
 *============================================================================
 *                        bitd_object_to_xml
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
char *bitd_object_to_xml(bitd_object_t *a,
			 char *object_name,
			 bitd_boolean full_xml) {
    char *buf = NULL, *buf1;
    int size = 0, idx = 0;
    
    bitd_assert(a);

    if (!object_name || !object_name[0]) {
	object_name = "_";
    }

    /* This makes object_name heap allocated */
    object_name = escape_to_xml(object_name);

    /* Buffer auto-allocated inside snprintf_w_realloc() */
    snprintf_w_realloc(&buf, &size, &idx,
		       "<?xml version='1.0'?>\n");

    buf1 = bitd_object_to_xml_element(a, object_name, 0, full_xml);
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
 *                        bitd_object_to_xml_element
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
char *bitd_object_to_xml_element(bitd_object_t *a,
				 char *object_name,
				 int indentation, /* How much to indent */
				 bitd_boolean full_xml) {
    char *buf = NULL;
    int size = 0, idx = 0, i;
    char *prefix;
    char *value_str = NULL;
    bitd_type_t t;
    bitd_boolean full_string_xml;

    bitd_assert(a);
    bitd_assert(indentation >= 0);

    if (!object_name) {
	object_name = "_";
    }

    /* Escape the object name for xml. This will allocate it on the heap. */
    object_name = escape_to_xml(object_name);

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
		value_str = escape_to_xml(c);
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


/*
 *============================================================================
 *                        bitd_nvp_to_xml
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
char *bitd_nvp_to_xml(bitd_nvp_t nvp, 
		    char *nvp_name,
		    bitd_boolean full_xml) {
    bitd_object_t a;

    a.type = bitd_type_nvp;
    a.v.value_nvp = nvp;

    return bitd_object_to_xml(&a, nvp_name, full_xml);
}


/*
 *============================================================================
 *                        bitd_nvp_to_xml_elem
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
char *bitd_nvp_to_xml_elem(bitd_nvp_t nvp, 
			   char *nvp_name,
			   int indentation, /* How much to indent */
			   bitd_boolean full_xml) {
    bitd_object_t a;

    a.type = bitd_type_nvp;
    a.v.value_nvp = nvp;
    
    return bitd_object_to_xml_element(&a, nvp_name, indentation, full_xml);
}


/*
 *============================================================================
 *                        free_user_data
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
void free_user_data(struct xml_user_data *u) {

    if (u) {
	bitd_object_free(&u->object);
	if (u->name) {
	    free(u->name);
	}
	if (u->value_str) {
	    free(u->value_str);
	}
	free(u);
    }
} 


/*
 *============================================================================
 *                        xml_start
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
static void xml_start(void *data, const XML_Char *el, 
		      const XML_Char **attr) {
    struct xml_user_data *u, *u_prev;
    char const *name = NULL;
    char const *type = NULL;

    /* Get the user data */
    u = (struct xml_user_data *)data;

    /* Check for previous parse errors */
    if (u->s->error_str) {
	/* Don't do any more parsing */
	return;
    }

    dbg_printf("xml_start(0x%p, %s)\n", u, el);

    /* Create a child data element */
    u_prev = u;
    u = calloc(1, sizeof(struct xml_user_data ));
    
    /* Chain it up after u_prev */
    u->next = u_prev->next;
    u->prev = u_prev;
    u->next->prev = u;
    u->prev->next = u;

    /* Copy the stream pointer */
    u->s = u_prev->s;
    
    /* Depth is one higher than for parent */
    u->depth = u_prev->depth + 1;

    /* Get the element name */
    name = el;

    /* Copy the name */
    u->name = strdup(name);

    /* Top level element name is the object name - we save it */
    if (u->depth == 1) {
	bitd_assert(!u->s->object_name);
	u->s->object_name = strdup(name);
    }

    /* Push the user data */
    XML_SetUserData(u->s->p, u);
    
    dbg_printf("push user data %p => %p\n", u_prev, u);

    /* Ensure that the parent type is 'nvp', if set */
    if (u->depth > 1) {
	if (u->prev->object_type_set_p) { 
	    if (u->prev->object.type != bitd_type_nvp) {
		u->s->error_str = xml_error("%s: invalid attribute name '%s', should be 'nvp' or empty",
					    u->prev->name, 
					    bitd_get_type_name(u->prev->object.type));
		return;
	    }
	} else {
	    /* Change the parent type to 'nvp' */
	    u->prev->object.type = bitd_type_nvp;
	    u->prev->object_type_set_p = TRUE;
	}
    }

    /* Get the one attribute we're looking for */
    while (*attr) {
	if (strcmp(*attr, "type")) {
	    u->s->error_str = 
		xml_error("%s: invalid attribute name '%s', should be 'type'",
			  name, *attr);
	    return;
	} else if (type) {
	    u->s->error_str = 
		xml_error("%s: invalid double 'type' attribute",
			  name);
	    return;
	}
	type = *(attr+1);

	/* Type is set */	
	u->object_type_set_p = TRUE;

	/* Get the type */
	if (!strcmp(type, "void")) {
	    u->object.type = bitd_type_void;
	} else if (!strcmp(type, "boolean")) {
	    u->object.type = bitd_type_boolean;
	} else if (!strcmp(type, "int64")) {
	    u->object.type = bitd_type_int64;
	} else if (!strcmp(type, "uint64")) {
	    u->object.type = bitd_type_uint64;
	} else if (!strcmp(type, "double")) {
	    u->object.type = bitd_type_double;
	} else if (!strcmp(type, "string")) {
	    u->object.type = bitd_type_string;
	} else if (!strcmp(type, "blob")) {
	    u->object.type = bitd_type_blob;
	} else if (!strcmp(type, "nvp")) {
	    u->object.type = bitd_type_nvp;
	} else {
	    u->s->error_str = xml_error("%s: invalid type '%s'",
					name, type);
	    return;
	}

	/* Look for next attribute */
	attr += 2;
    }
} 


/*
 *============================================================================
 *                        xml_end
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
static void xml_end(void *data, const XML_Char *el) {
    struct xml_user_data *u = (struct xml_user_data *)data;
    char *c;

    /* Check for previous parse errors */
    if (u->s->error_str) {
	/* Don't do any more parsing */
	return;
    }

    dbg_printf("xml_end(%p, %s)\n", u, el);

    bitd_assert(!strcmp(u->name, el));
    bitd_assert(u->depth > 0);
    
    /* If type is not set, get the type based on the value */
    if (!u->object_type_set_p) {
	/* Convert to object */
	bitd_string_to_object(&u->object, u->value_str);
    } else {
	if (u->object.type != bitd_type_nvp) {
	    /* Convert string value */
	    if (!bitd_typed_string_to_object(&u->object, u->value_str, 
					   u->object.type)) {
		u->s->error_str = xml_error("Invalid element '%s' value '%s'",
					    u->name, 
					    u->value_str ? u->value_str : "NULL");
		return;
	    }
	} else {
	    /* This is an nvp that was already filled in by xml_end() called
	       for children elements. Check that the current xml element
	       has no non-space text. */
	    if (u->value_str) {
		for (c = u->value_str; *c; c++) {
		    if (!isspace(c[0])) {
			u->s->error_str = 
			    xml_error("Invalid nvp element '%s' value '%s'",
				      el, u->value_str);
			return;
		    }
		}
	    }
	}
    }

    if (u->depth > 1) {
	char *name = u->name;

	bitd_assert(u->prev->object.type == bitd_type_nvp);

	/* Convert the name to NULL if name is "_" */
	if (!strcmp(name, "_")) {
	    name = NULL;
	}
	
	/* Add the object to the higher nvp */
	bitd_nvp_add_elem(&(u->prev->object.v.value_nvp), 
			name, 
			&u->object.v, 
			u->object.type);

    } else {
	/* At depth 1, simply copy the object */
	u->s->object = u->object;
	/* Ensure we don't free this object */
	u->object.type = bitd_type_void;
    }

    /* Unchain u */
    u->prev->next = u->next;
    u->next->prev = u->prev;
    
    /* Pull the user data */
    XML_SetUserData(u->s->p, u->prev);
    
    dbg_printf("pop user data %p <= %p\n", u->prev, u);
    
    /* Free u */
    free_user_data(u);
} 


/*
 *============================================================================
 *                        xml_char_data_handler
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
static void xml_char_data_handler(void *data, 
				  const XML_Char *s, int len) {
    struct xml_user_data *u = (struct xml_user_data *)data;

    /* Check for previous parse errors */
    if (u->s->error_str) {
	/* Don't do any more parsing */
	return;
    }

    bitd_assert(u->depth > 0);
    dbg_printf("xml_char_data_handler(%p)\n", u);

    u->value_str = realloc(u->value_str, u->value_len + len + 1);
    memcpy(u->value_str + u->value_len, s, len);
    u->value_len += len;
    
    /* NULL termination */
    u->value_str[u->value_len] = 0;
} 


/*
 *============================================================================
 *                        bitd_xml_to_object
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
bitd_boolean bitd_xml_to_object(bitd_object_t *a, char **object_name,
				char *xml, int xml_nbytes) {
    bitd_boolean ret;
    bitd_xml_stream s;

    /* Parameter check */
    if (!a) {
	return FALSE;
    }

    /* Initialize the OUT parameters */
    bitd_object_init(a);
    if (object_name) {
	*object_name = NULL;
    }

    /* Empty xml converts to void object */
    if (!xml || !xml_nbytes) {
	return TRUE;
    }

    /* Initialize the nx stream */
    s = bitd_xml_stream_init();

    /* Read the whole xml in one pass */
    ret = bitd_xml_stream_read(s, xml, xml_nbytes, TRUE);
    if (ret) {
	/* Get the nvp */
	bitd_xml_stream_get_object(s, a, object_name);
    }

    /* Deinitialize the nx stream */
    bitd_xml_stream_free(s);

    return ret;
} 


/*
 *============================================================================
 *                        bitd_xml_to_nvp
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
bitd_boolean bitd_xml_to_nvp(bitd_nvp_t *nvp, char *xml, int xml_nbytes) {
    int ret;
    bitd_object_t a;

    /* Parameter check */
    if (!nvp) {
	return FALSE;
    }
    
    /* Initialize OUT parameter */
    *nvp = NULL;

    bitd_object_init(&a);

    /* Convert the xml to an object */
    ret = bitd_xml_to_object(&a, NULL, xml, xml_nbytes);
    if (ret) {
	if (a.type != bitd_type_nvp) {
	    ret = FALSE;
	    goto end;
	}

	*nvp = a.v.value_nvp;
	a.v.value_nvp = NULL;
    }

 end:
    bitd_object_free(&a);
    return ret;
} 


/*
 *============================================================================
 *                        bitd_xml_stream_init
 *============================================================================
 * Description:     Create an nvp-xml stream
 * Parameters:    
 * Returns:  
 */
bitd_xml_stream bitd_xml_stream_init(void) {
    bitd_xml_stream s;

    /* Allocate the stream control block */
    s = calloc(1, sizeof(*s));

    /* Initialize the user data as empty list */
    s->head = USER_DATA_HEAD(s);
    s->tail = USER_DATA_HEAD(s);

    /* Set up the stream pointer to itself */
    s->s = s;

    /* Create the expat parser */
    s->p = XML_ParserCreate(NULL);

    /* Set the element handlers */
    XML_SetElementHandler(s->p, xml_start, xml_end);
    XML_SetCharacterDataHandler(s->p, xml_char_data_handler);
    
    /* Initialize the user data */
    XML_SetUserData(s->p, s->head);

    return s;
} 


/*
 *============================================================================
 *                        bitd_xml_stream_free
 *============================================================================
 * Description:     Destroy an nvp-xml stream
 * Parameters:    
 * Returns:  
 */
void bitd_xml_stream_free(bitd_xml_stream s) {
    struct xml_user_data *u;

    if (s) {
	/* Free all elements in the user data chain */
	while (s->head != USER_DATA_HEAD(s)) {
	    u = s->head; 
	    /* Unchain u */
	    u->next->prev = u->prev;
	    u->prev->next = u->next;
	    
	    /* Release u */
	    free_user_data(u);
	}

	/* Deinitialize the user data - may not be needed */
	XML_SetUserData(s->p, NULL);

	/* Destroy the expat parser */
	XML_ParserFree(s->p);

	bitd_object_free(&s->object);

	if (s->object_name) {
	    free(s->object_name);
	}
	if (s->error_str) {
	    free(s->error_str);
	}

	free(s);
    }
} 


/*
 *============================================================================
 *                        bitd_xml_stream_read
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
bitd_boolean bitd_xml_stream_read(bitd_xml_stream s,
				  char *buf, int size,
				  bitd_boolean done) {
    int len, idx;

    if (!s) {
	return FALSE;
    }

    if (!XML_Parse(s->p, buf, size, done)) {
	/* A parse error */
	if (s->error_str) {
	    free(s->error_str);
	    
	}
	len = 1024;
	s->error_str = malloc(len);
	idx = snprintf(s->error_str, len - 1,
		       "Xml parse error: %s at line %lu",
		       XML_ErrorString(XML_GetErrorCode(s->p)),
		       XML_GetCurrentLineNumber(s->p));
	
	/* NULL termination */
	s->error_str[idx] = 0;
    }

    /* If the error is set, return FALSE. */
    if (s->error_str) {
	return FALSE;
    }

    /* Else, no parse error. */
    return TRUE;
} 


/*
 *============================================================================
 *                        bitd_xml_stream_get_object
 *============================================================================
 * Description:     Get the stream object. Clone it
 * Parameters:    
 * Returns:  
 */
void bitd_xml_stream_get_object(bitd_xml_stream s,
				bitd_object_t *a,
				char **object_name) {
    if (s) {
	if (a) {
	    bitd_object_clone(a, &s->object);
	}
	if (object_name) {
	    *object_name = NULL;
	    
	    if (s->object_name) {
		*object_name = strdup(s->object_name);
	    }
	}
    }
} 


/*
 *============================================================================
 *                        bitd_xml_stream_get_nvp
 *============================================================================
 * Description:     Get the stream nvp. Clone it
 * Parameters:    
 * Returns:  
 */
bitd_nvp_t bitd_xml_stream_get_nvp(bitd_xml_stream s) {
    if (s && s->object.type == bitd_type_nvp) {
	return bitd_nvp_clone(s->object.v.value_nvp);
    }

    return NULL;
} 


/*
 *============================================================================
 *                        bitd_xml_stream_get_error
 *============================================================================
 * Description:     Get the parse error string, if any
 * Parameters:    
 * Returns:  
 */
char *bitd_xml_stream_get_error(bitd_xml_stream s) {
    if (s) {
	return s->error_str;
    }
    return NULL;
} 
