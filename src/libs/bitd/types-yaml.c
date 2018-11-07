/*****************************************************************************
 *
 * Original Author: 
 * Creation Date:   
 * Description: 
 *
 ****************************************************************************/

/*****************************************************************************
 *                                INCLUDE FILES 
 *****************************************************************************/
#include "bitd/types.h"
#include <stdarg.h>
#include <yaml.h>

/*****************************************************************************
 *                             MANIFEST CONSTANTS
 *****************************************************************************/



/*****************************************************************************
 *                                  MACROS 
 *****************************************************************************/
#define YAML_NO_ERROR(f)			\
    if (!(f)) {					\
	goto error;				\
    }

#define dbg_printf if (0) printf


/*****************************************************************************
 *                                  TYPES
 *****************************************************************************/

struct yaml_emitter_data {
    char *buf;
    int buf_idx;
    int buf_size;
};

enum yaml_context_type {
    document_context_type,
    sequence_context_type,
    mapping_context_type
};

enum yaml_anchor_type {
    sequence_anchor_type,
    mapping_anchor_type,
    scalar_anchor_type
};

/* Anchor control block */
struct yaml_anchor {
    struct yaml_anchor *next;    
    char *name;
    enum yaml_anchor_type anchor_type;
    bitd_object_t object;
    struct yaml_context *yaml_context;
};

/* Yaml parser user data */
struct yaml_context {
    struct yaml_context *next;  /* Enclosed block (=child) context */
    struct yaml_context *prev;  /* Enclosing block (=parent) context */
    int depth;                  /* Should be 3rd struct member */
    struct bitd_yaml_parser_s *s; /* Should be 4th struct member */
    char *name;
    bitd_object_t object;
    struct yaml_anchor *anchor; /* NULL unless current object defines anchor */
    enum yaml_context_type context_type;
    bitd_nvp_t merged_nvp_mapping;
    bitd_nvp_t merged_nvp_list;
};

/* The yaml parser object - wrapper around libyaml parser */
struct bitd_yaml_parser_s {
    struct yaml_context *head;  /* Head, tail of yaml context stack */
    struct yaml_context *tail;  
    int depth;         /* Should be 3rd struct member */
    struct bitd_yaml_parser_s *s; /* Should be 4th struct member */
    yaml_parser_t p;
    bitd_object_t object;
    bitd_nvp_t stream_nvp;
    long doc_count;
    struct yaml_anchor *anchor_head; /* The anchor list */
    char *error_str;
};

#define YAML_CONTEXT_HEAD(s) \
    ((struct yaml_context *)&(s)->head)

/*****************************************************************************
 *                           FUNCTION DECLARATION
 *****************************************************************************/
/* Context APIs */
static struct yaml_context *yaml_context_push(struct yaml_context *yc);
static struct yaml_context *yaml_context_pop(struct yaml_context *yc);
static void yaml_context_free(struct yaml_context *yc);

/* Anchor APIs */
static struct yaml_anchor *yaml_anchor_get(struct bitd_yaml_parser_s *s, 
					   yaml_char_t *anchor_name);
static struct yaml_anchor *yaml_anchor_add(struct bitd_yaml_parser_s *s, 
					   yaml_char_t *anchor_name,
					   struct yaml_context *yc);
static void yaml_anchor_free(struct yaml_anchor *a);

static char *yaml_error(char *fmt, ...);



/*****************************************************************************
 *                                VARIABLES
 *****************************************************************************/



/*****************************************************************************
 *                          FUNCTION IMPLEMENTATION
 *****************************************************************************/


/*
 *============================================================================
 *                        write_handler
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
static int write_handler(void *data, unsigned char *buffer, size_t size) {
    struct yaml_emitter_data *emitter_data = (struct yaml_emitter_data *)data;

    /* If needed, reallocate a larger buffer */
    while (size + emitter_data->buf_idx > emitter_data->buf_size + 1) {
	emitter_data->buf_size *= 2;
	emitter_data->buf = realloc(emitter_data->buf, emitter_data->buf_size);
    }

    /* Copy the data */
    memcpy(emitter_data->buf + emitter_data->buf_idx, buffer, size);

    /* Update the index */
    emitter_data->buf_idx += size;

    /* NULL termination */
    emitter_data->buf[emitter_data->buf_idx] = 0;

    return 1;
} 


/*
 *============================================================================
 *                        bitd_object_to_yaml_element
 *============================================================================
 * Description:     Low-level API for converting an object to yaml
 * Parameters:    
 * Returns:  
 *     1 on success, 0 on error (to follow libyaml convention)
 */
int bitd_object_to_yaml_element(bitd_object_t *a,
				yaml_emitter_t *emitter,
				int plain_implicit,
				int quoted_implicit) {
    yaml_event_t event;
    int i;
    char *s, *tag = NULL;
    int style;
    int pi, qi;
    bitd_nvp_t nvp;
    bitd_boolean sequence_p;

    /* Compute the tag and the implicit argument */
    pi = plain_implicit;
    qi = quoted_implicit;

    switch (a->type) {
    case bitd_type_void:
	tag = "tag:yaml.org,2002:null";
	break;
    case bitd_type_boolean:
	tag = "tag:yaml.org,2002:bool";
	break;
    case bitd_type_int64:
	tag = "tag:yaml.org,2002:int";
	break;
    case bitd_type_uint64:
	tag = "tag:yaml.org,2002:int";
	break;
    case bitd_type_double:
	tag = "tag:yaml.org,2002:float";
	break;
    case bitd_type_string:
	tag = "tag:yaml.org,2002:str";
	break;
    case bitd_type_blob:
	tag = "tag:yaml.org,2002:binary";
	
	/* Blobs can't be implicit, else they are confused 
	   for strings */
	pi = FALSE;
	qi = FALSE;
	
	break;
    case bitd_type_nvp:
	break;
	tag = "tag:yaml.org,2002:seq";
    default:
	break;	
    }
    
    switch (a->type) {
    case bitd_type_void:
    case bitd_type_boolean:
    case bitd_type_int64:
    case bitd_type_uint64:
    case bitd_type_double:
    case bitd_type_string:
    case bitd_type_blob:
	s = bitd_value_to_string(&a->v, a->type);
	
	/* Initialize the style to default */
	style = YAML_ANY_SCALAR_STYLE;
	
	/* If a string... */
	if (a->type == bitd_type_string) {
	    bitd_type_t t1;

	    if (!s) {
		s = strdup("");
	    }
            
	    /* We should print the string in full if the string
	       could be interpreted as a void, boolean, int64,
	       uint64 or double */
	    
	    bitd_string_to_type_and_value(&t1, NULL, s);
	    if (t1 != bitd_type_string && 
		t1 != bitd_type_blob) {
		style = YAML_SINGLE_QUOTED_SCALAR_STYLE;
	    } else {
		register int len;
		register char *s1, c;
		
		for (s1 = s, len = 0;;) {
		    c = *s1++;
		    if (c == '\n') {
			style = YAML_LITERAL_SCALAR_STYLE;
			break;
		    } else if (len > 40) {
			style = YAML_FOLDED_SCALAR_STYLE;
			break;
		    } else if (!c) {
			break;
		    }
		    len++;
		}
	    }
	}

	/* Create and emit the SCALAR event for the value */
	YAML_NO_ERROR(yaml_scalar_event_initialize(&event, 
						   NULL, 
						   (yaml_char_t *)tag,
						   (yaml_char_t *)s,
						   -1,
						   pi, qi, 
						   style));
	YAML_NO_ERROR(yaml_emitter_emit(emitter, &event));
	free(s);	    
	break;
    case bitd_type_nvp:

	/* The nvp is represented as a mapping if at least one
	   element has a non-empty name - otherwise, it is represented
	   as a sequence. NULL nvps are represented as an empty mapping. */
	nvp = a->v.value_nvp;
	sequence_p = FALSE;
	if (nvp) {
	    /* Potentially a sequence */
	    sequence_p = TRUE;
	    for (i = 0; i < nvp->n_elts; i++) {
		if (nvp->e[i].name && nvp->e[i].name[0]) {
		    /* Not a sequence */
		    sequence_p = FALSE;
		    break;
		}
	    }
	}

	if (sequence_p) {
	    /* Create the SEQUENCE-START event */
	    YAML_NO_ERROR(yaml_sequence_start_event_initialize(&event, NULL, NULL, 1, YAML_BLOCK_SEQUENCE_STYLE));
	} else {
	    /* Create the MAPPING-START event */
	    YAML_NO_ERROR(yaml_mapping_start_event_initialize(&event, NULL, NULL, 1, 
							      YAML_BLOCK_MAPPING_STYLE));
	}
	/* Emit the event */
	YAML_NO_ERROR(yaml_emitter_emit(emitter, &event));

	if (a->v.value_nvp) {
	    bitd_nvp_t nvp = a->v.value_nvp;

	    for (i = 0; i < nvp->n_elts; i++) {
		bitd_object_t a1;

		if (!sequence_p) {
		    /* Normalize the element name */
		    s = nvp->e[i].name;
		    if (!s || !s[0]) {
			s = "_";
		    }
		    
		    /* Create and emit the SCALAR event for the name */
		    YAML_NO_ERROR(yaml_scalar_event_initialize(&event, 
							       NULL, 
							       (yaml_char_t *)"tag:yaml.org,2002:str", 
							       (yaml_char_t *)s,
							       -1,
							       pi, qi, 
							       YAML_PLAIN_SCALAR_STYLE));
		    YAML_NO_ERROR(yaml_emitter_emit(emitter, &event));
		}

		a1.type = nvp->e[i].type;
		a1.v = nvp->e[i].v;

		/* Recurse over the element */
		YAML_NO_ERROR(bitd_object_to_yaml_element(&a1,
							emitter,
							plain_implicit,
							quoted_implicit));
	    }
	}

	if (sequence_p) {
	    /* Create the SEQUENCE-END event */
	    YAML_NO_ERROR(yaml_sequence_end_event_initialize(&event));
	} else {
	    /* Create the MAPPING-END event */
	    YAML_NO_ERROR(yaml_mapping_end_event_initialize(&event));
	}
	/* Emit the event */
	YAML_NO_ERROR(yaml_emitter_emit(emitter, &event));
	break;
    default:
	break;	
    }

    /* Return 1 on success, to follow libyaml convention */
    return 1;

 error:
    /* Return 0  on error */
    return 0;

} 


/*
 *============================================================================
 *                        bitd_object_to_yaml
 *============================================================================
 * Description:     High-level API for converting an nvp to yaml
 * Parameters:    
 * Returns:  
 */
char *bitd_object_to_yaml(bitd_object_t *a, 
			  bitd_boolean full_yaml,
			  bitd_boolean is_stream) {

    yaml_emitter_t emitter;
    yaml_event_t event;
    struct yaml_emitter_data emitter_data;
    int plain_implicit = TRUE;
    int quoted_implicit = TRUE;

    /* Parameter check */
    if (!a) {
	return NULL;
    }

    /* If the input is stream, the object has to be nvp - in which case
       we handle the conversion in bitd_nvp_to_yaml()  */
    if (is_stream) {
	if (a->type != bitd_type_nvp) {
	    return NULL;
	}
	return bitd_nvp_to_yaml(a->v.value_nvp, full_yaml, TRUE);
    }

    /* Create the Emitter object. */
    YAML_NO_ERROR(yaml_emitter_initialize(&emitter));

    /* Initialize the emitter data buffer */
    emitter_data.buf_size = 1024;
    emitter_data.buf = malloc(emitter_data.buf_size);
    emitter_data.buf[0] = 0;

    /* Initialize the buffer index */
    emitter_data.buf_idx = 0;

    /* Pass the write handler */
    yaml_emitter_set_output(&emitter, write_handler, &emitter_data);

    /* Set the encoding */
    yaml_emitter_set_encoding(&emitter, YAML_UTF8_ENCODING);

    /* Two spaces indentation */
    yaml_emitter_set_indent(&emitter, 2);

    /* Set width to 80 chars, which is a typical default line width */
    yaml_emitter_set_width(&emitter, 80);

    /* Unicode characters should be escaped */
    yaml_emitter_set_unicode(&emitter, 0);

    /* Let parser choose the line break */
    yaml_emitter_set_break(&emitter, YAML_ANY_BREAK);

    if (full_yaml) {
	/* Don't leave yaml output implicit */
	plain_implicit = FALSE;
	quoted_implicit = FALSE;
    }

    /* Create and emit the STREAM-START event. */
    YAML_NO_ERROR(yaml_stream_start_event_initialize(&event, 
						     YAML_UTF8_ENCODING));
    YAML_NO_ERROR(yaml_emitter_emit(&emitter, &event));

    /* Create and emit the DOCUMENT-START event */
    YAML_NO_ERROR(yaml_document_start_event_initialize(&event, 
						       NULL, NULL, NULL, 
						       TRUE));
    YAML_NO_ERROR(yaml_emitter_emit(&emitter, &event));

    /* Bear trap */
    bitd_assert_object(a);

    YAML_NO_ERROR(bitd_object_to_yaml_element(a, &emitter, 
					      plain_implicit,
					      quoted_implicit));

    /* Create and emit the DOCUMENT-END event */
    YAML_NO_ERROR(yaml_document_end_event_initialize(&event, TRUE));
    YAML_NO_ERROR(yaml_emitter_emit(&emitter, &event));

    /* Create and emit the STREAM-END event. */
    YAML_NO_ERROR(yaml_stream_end_event_initialize(&event));
    YAML_NO_ERROR(yaml_emitter_emit(&emitter, &event));

    /* Destroy the Emitter object. */
    yaml_emitter_delete(&emitter);

    return emitter_data.buf;

error:
    /* Destroy the Emitter object. */
    yaml_emitter_delete(&emitter);
    free(emitter_data.buf);
    
    return NULL;
} 


/*
 *============================================================================
 *                        bitd_nvp_to_yaml
 *============================================================================
 * Description:     High-level API for converting an nvp to yaml
 * Parameters:    
 * Returns:  
 */
char *bitd_nvp_to_yaml(bitd_nvp_t nvp, 
		       bitd_boolean full_yaml, 
		       bitd_boolean is_stream) {
    bitd_object_t a;
    char *buf = NULL, *buf1 = NULL;
    int i, idx = 0, size = 0, len;

    if (!is_stream) {
	a.type = bitd_type_nvp;
	a.v.value_nvp = nvp;

	return bitd_object_to_yaml(&a, full_yaml, FALSE);
    }

    if (nvp) {
	for (i = 0; i < nvp->n_elts; i++) {
	    /* Ignore the element name */
	    a.type = nvp->e[i].type;
	    a.v = nvp->e[i].v;

	    buf1 = bitd_object_to_yaml(&a, full_yaml, FALSE);
	    
	    len = 0;
	    if (buf1) {
		len = strlen(buf1);
	    }
	    if (idx + len + 24 > size) {
		size += 10240;
		buf = realloc(buf, size);
	    }
	    idx += sprintf(buf + idx, "---\n");
	    if (buf1) {
		idx += sprintf(buf + idx, "%s", buf1);
		free(buf1);
	    }
	}
    }

    if (idx + 24 > size) {
	size += 24;
	buf = realloc(buf, size);
    }
    idx += sprintf(buf + idx, "...\n");

    return buf;
} 


/*
 *============================================================================
 *                        bitd_yaml_to_object
 *============================================================================
 * Description:   Convert yaml to object. The is_stream OUT parameter is set 
 *     to TRUE if the yaml document is a stream (in which case the object 
 *     is an nvp with one sub-object named _doc per document in the stream)  
 * Parameters:    
 *     a [OUT] - the parsed object
 *     yaml - the buffer containing the yaml document
 *     yaml_nbytes - length of the buffer
 *     is_stream [OUT] - reports back whether the object returned is a stream
 *     err_buf [OUT] - if provided, we write the parse errors here
 *     err_len [IN] - the length of err_buf
 * Returns:  
 *     TRUE on success, FALSE on parse error or parameter parse error
 */
bitd_boolean bitd_yaml_to_object(bitd_object_t *a, 
				 char *yaml, int yaml_nbytes,
				 bitd_boolean *is_stream,
				 char *err_buf,
				 int err_len) {
    bitd_boolean ret;
    bitd_yaml_parser s;

    /* Parameter check */
    if (!a) {
	return FALSE;
    }

    /* Initialize the OUT parameters */
    bitd_object_init(a);
    if (is_stream) {
	*is_stream = FALSE;
    }
    if (err_buf && err_len) {
	/* Initialize the error buffer */
	*err_buf = 0;
    }

    if (!yaml || !yaml_nbytes) {
	return TRUE;
    }

    /* Initialize the nx stream */
    s = bitd_yaml_parser_init();

    /* Read the whole yaml in one pass */
    ret = bitd_yaml_parse(s, yaml, yaml_nbytes);
    if (ret) {
	if (s->stream_nvp) {
	    if (s->stream_nvp->n_elts > 1) {
		a->type = bitd_type_nvp;
		a->v.value_nvp = bitd_nvp_clone(s->stream_nvp);

		if (is_stream) {
		    *is_stream = TRUE;
		}
	    } else if (s->stream_nvp->n_elts == 1) {
		a->type = s->stream_nvp->e[0].type;
		bitd_value_clone(&a->v, &s->stream_nvp->e[0].v, a->type);
	    }
	}
    } else {
	if (err_buf && err_len) {
	    /* Save the parse error */
	    snprintf(err_buf, err_len - 1, "%s", 
		     bitd_yaml_parser_get_error(s));
	    err_buf[err_len] = 0;
	}
    }

    /* Deinitialize the nx stream */
    bitd_yaml_parser_free(s);

    return ret;
} 


/*
 *============================================================================
 *                        bitd_yaml_to_nvp
 *============================================================================
 * Description:     Same as above, but return the object as nvp, if it is
 *     an nvp, or NULL if it is not an NVP
 * Parameters:    
 * Returns:  
 */
bitd_boolean bitd_yaml_to_nvp(bitd_nvp_t *pnvp, char *yaml, int yaml_nbytes,
			      bitd_boolean *is_stream,
			      char *err_buf,
			      int err_len) {
    bitd_boolean ret;
    bitd_yaml_parser s;

    /* Initialize the OUT parameters */
    if (!pnvp) {
	return FALSE;
    }
    *pnvp = NULL;
    if (is_stream) {
	*is_stream = FALSE;
    }
    if (err_buf && err_len) {
	/* Initialize the error buffer */
	*err_buf = 0;
    }

    if (!yaml || !yaml_nbytes) {
	return TRUE;
    }

    /* Initialize the nx stream */
    s = bitd_yaml_parser_init();

    /* Read the whole yaml in one pass */
    ret = bitd_yaml_parse(s, yaml, yaml_nbytes);
    if (ret) {
	if (s->stream_nvp) {
	    if (s->stream_nvp->n_elts > 1) {
		if (is_stream) {
		    *is_stream = TRUE;
		}
		*pnvp = bitd_nvp_clone(s->stream_nvp);
	    } else if (s->stream_nvp->n_elts == 1 &&
		       s->stream_nvp->e[0].type == bitd_type_nvp) {
		*pnvp = bitd_nvp_clone(s->stream_nvp->e[0].v.value_nvp);
	    }
	}
    } else {
	if (err_buf && err_len) {
	    /* Save the parse error */
	    snprintf(err_buf, err_len - 1, "%s", 
		     bitd_yaml_parser_get_error(s));
	    err_buf[err_len] = 0;
	}
    }

    /* Deinitialize the nx stream */
    bitd_yaml_parser_free(s);

    return ret;
} 


/*
 *============================================================================
 *                        yaml_anchor_get
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
struct yaml_anchor *yaml_anchor_get(struct bitd_yaml_parser_s *s, 
				    yaml_char_t *anchor_name) {
    struct yaml_anchor *a;

    if (!anchor_name || !anchor_name[0]) {
	return NULL;
    }

    for (a = s->anchor_head; a; a = a->next) {
	if (!strcmp(a->name, (char *)anchor_name)) {
	    return a;
	}
    }

    return NULL;
} 


/*
 *============================================================================
 *                        yaml_anchor_add
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
struct yaml_anchor *yaml_anchor_add(struct bitd_yaml_parser_s *s, 
				    yaml_char_t *anchor_name,
				    struct yaml_context *yc) {
    struct yaml_anchor *a;

    if (!anchor_name || !anchor_name[0]) {
	return NULL;
    }

    a = yaml_anchor_get(s, anchor_name);
    if (a) {
	/* Release the value, if any */
	bitd_object_free(&a->object);
	bitd_object_init(&a->object);
    } else {
	/* Allocate the anchor structure */
	a = calloc(1, sizeof(*a));
	a->name = strdup((char *)anchor_name);
	bitd_object_init(&a->object);

	/* Chain it up */
	a->next = s->anchor_head;
	s->anchor_head = a;
    }

    a->yaml_context = yc;
    return a;
} 


/*
 *============================================================================
 *                        yaml_anchor_free
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
void yaml_anchor_free(struct yaml_anchor *a) {
    
    if (a) {
	if (a->name) {
	    free(a->name);
	}
	bitd_object_free(&a->object);
	free(a);
    }
} 


/*
 *============================================================================
 *                        yaml_context_push
 *============================================================================
 * Description:     Push a new yaml context, and return a pointer to it
 * Parameters:    
 * Returns:  
 */
struct yaml_context *yaml_context_push(struct yaml_context *yc_parent) {
    struct yaml_context *yc;
    
    yc = calloc(1, sizeof(*yc));
    yc->prev = yc_parent;
    yc->next = YAML_CONTEXT_HEAD(yc_parent->s);
    yc->prev->next = yc;
    yc->next->prev = yc;
    yc->depth = yc_parent->depth+1;
    yc->s = yc_parent->s;

    dbg_printf("^^^ Context push, depth %d\n", yc->depth);

    return yc;
}


/*
 *============================================================================
 *                        yaml_context_pop
 *============================================================================
 * Description:     Pop the yaml context, return the parent context
 * Parameters:    
 * Returns:  
 */
struct yaml_context *yaml_context_pop(struct yaml_context *yc) {
    struct yaml_context *yc_parent = yc->prev;
    struct yaml_anchor *anchor;
    int i;
    bitd_nvp_t nvp;
    
    bitd_assert(yc->depth > 0);
	    
    if (yc->depth > 1) {
	if (yc->object.type == bitd_type_nvp) {
	    if (yc->merged_nvp_mapping) {
		/* Merge the nvp */
		nvp = bitd_nvp_merge(yc->merged_nvp_mapping, 
				   yc->object.v.value_nvp,
				   NULL);
		bitd_nvp_free(yc->object.v.value_nvp);
		yc->object.v.value_nvp = nvp;	    
	    } else if (yc->merged_nvp_list &&
		       yc->merged_nvp_list->n_elts) {
		for (i = 0; i < yc->merged_nvp_list->n_elts; i++) {
		    bitd_assert(yc->merged_nvp_list->e[i].type == bitd_type_nvp);
		    nvp = bitd_nvp_merge(yc->merged_nvp_list->e[i].v.value_nvp,
				       yc->object.v.value_nvp,
				       NULL);
		    bitd_nvp_free(yc->object.v.value_nvp);
		    yc->object.v.value_nvp = nvp;	    
		}
	    }
	}

	if (yc_parent->object.type == bitd_type_nvp) {
	    /* Add the object to the parent nvp */
	    bitd_nvp_add_elem(&yc_parent->object.v.value_nvp, 
			    yc_parent->name, 
			    &yc->object.v, 
			    yc->object.type);
	} else {
	    /* Replace the parent object with the current object */
	    bitd_object_free(&yc_parent->object);
	    yc_parent->object = yc->object;
	    bitd_object_init(&yc->object);
	}
	
	/* Clear the parent name, if any */
	if (yc_parent->name) {
	    free(yc_parent->name);
	    yc_parent->name = NULL;
	}

    } else {
	/* Replace the base object with the current object */
	bitd_object_free(&yc->s->object);
	yc->s->object = yc->object;
	bitd_object_init(&yc->object);
    }

    /* Also save the object into any anchors pointing to this yaml context */
    for (anchor = yc->s->anchor_head; anchor; anchor = anchor->next) {
	if (anchor->yaml_context == yc) {
	    bitd_object_clone(&anchor->object, &yc->object);
	    anchor->yaml_context = NULL;
	}
    }

    bitd_nvp_free(yc->merged_nvp_mapping);
    bitd_nvp_free(yc->merged_nvp_list);

    /* Unchain from list */
    yc->prev->next = yc->next;
    yc->next->prev = yc->prev;
    
    /* Release memory */
    yaml_context_free(yc);

    dbg_printf("^^^ Context pop, depth %d\n", yc_parent->depth);

    return yc_parent;
}


/*
 *============================================================================
 *                        yaml_context_free
 *============================================================================
 * Description:     Free memory associated to a yaml context
 * Parameters:    
 * Returns:  
 */
void yaml_context_free(struct yaml_context *yc) {

    if (yc) {
	bitd_object_free(&yc->object);
	if (yc->name) {
	    free(yc->name);
	}
	free(yc);
    }
} 


/*
 *============================================================================
 *                        bitd_yaml_parser_init
 *============================================================================
 * Description:     Create an nvp-yaml parser
 * Parameters:    
 * Returns:  
 */
bitd_yaml_parser bitd_yaml_parser_init(void) {
    bitd_yaml_parser s;

    /* Allocate the stream control block */
    s = calloc(1, sizeof(*s));

    /* Initialize the user data as empty list */
    s->head = YAML_CONTEXT_HEAD(s);
    s->tail = YAML_CONTEXT_HEAD(s);

    /* Point s->s to itself so user data referencing it an read it */
    s->s = s;

    /* Create the yaml parser */
    if (!yaml_parser_initialize(&s->p)) {
	free(s);
	return NULL;
    }

    /* Set the encoding */
    yaml_parser_set_encoding(&s->p, YAML_UTF8_ENCODING);

    return s;
} 


/*
 *============================================================================
 *                        bitd_yaml_parser_free
 *============================================================================
 * Description:     Destroy an nvp-yaml parser
 * Parameters:    
 * Returns:  
 */
void bitd_yaml_parser_free(bitd_yaml_parser s) {
    struct yaml_context *yc;
    struct yaml_anchor *a, *a1;

    if (s) {
	/* Free all elements in the user data chain */
	while (s->head != YAML_CONTEXT_HEAD(s)) {
	    yc = s->head; 
	    /* Unchain u */
	    yc->next->prev = yc->prev;
	    yc->prev->next = yc->next;
	    
	    /* Release u */
	    yaml_context_free(yc);
	}

	/* Free all the anchor structs */
	a = s->anchor_head;
	while (a) {
	    a1 = a->next;
	    yaml_anchor_free(a);
	    a = a1;
	}

	yaml_parser_delete(&s->p);
	bitd_object_free(&s->object);
	bitd_nvp_free(s->stream_nvp);
	
	if (s->error_str) {
	    free(s->error_str);
	}

	free(s);
    }
} 


/*
 *============================================================================
 *                        bitd_yaml_parse
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
bitd_boolean bitd_yaml_parse(bitd_yaml_parser s,
			 char *buf, int size) {
    yaml_event_t event;
    int done = FALSE;
    int idx;
    int error_size = 10240;
    struct yaml_context *yc = (struct yaml_context *)s;
    struct yaml_anchor *anchor;
    bitd_type_t t;
    bitd_value_t v;    

    if (!s) {
	return FALSE;
    }

    /* Pass the input to the parser */
    yaml_parser_set_input_string(&s->p, (unsigned char *)buf, size);

    while (!done) {
	if (!yaml_parser_parse(&s->p, &event) ||
	    s->p.error != YAML_NO_ERROR) {
	    if (s->error_str) {
		free(s->error_str);
	    }
	    s->error_str = calloc(1, error_size + 1);
	    switch (s->p.error) {
	    case YAML_MEMORY_ERROR:
		snprintf(s->error_str,
			 error_size,
			 "Memory error: Not enough memory for parsing");
		break;

	    case YAML_READER_ERROR:
		if (s->p.problem_value != -1) {
		    snprintf(s->error_str,
			     error_size,
			     "Reader error: %s: #%X at %ld", 
			     s->p.problem,
			     s->p.problem_value, 
			     (long)s->p.problem_offset);
		} else {
		    snprintf(s->error_str,
			     error_size,
			     "Reader error: %s at %ld", s->p.problem,
			     (long)s->p.problem_offset);
		}
		break;

	    case YAML_SCANNER_ERROR:
		if (s->p.context) {
		    snprintf(s->error_str,
			     error_size,
			     "Scanner error: %s at line %d, column %d"
			     "%s at line %d, column %d", s->p.context,
			     (int)s->p.context_mark.line+1, 
			     (int)s->p.context_mark.column+1,
			     s->p.problem, (int)s->p.problem_mark.line+1,
			     (int)s->p.problem_mark.column+1);
		} else {
		    snprintf(s->error_str,
			     error_size,
			     "Scanner error: %s at line %d, column %d",
			     s->p.problem, (int)s->p.problem_mark.line+1,
			     (int)s->p.problem_mark.column+1);
		}
		break;
		
	    case YAML_PARSER_ERROR:
		if (s->p.context) {
		    snprintf(s->error_str,
			     error_size,
			     "Parser error: %s at line %d, column %d"
			     "%s at line %d, column %d", s->p.context,
			     (int)s->p.context_mark.line+1, 
			     (int)s->p.context_mark.column+1,
			     s->p.problem, (int)s->p.problem_mark.line+1,
			     (int)s->p.problem_mark.column+1);
		} else {
		    snprintf(s->error_str,
			     error_size,
			     "Parser error: %s at line %d, column %d",
			     s->p.problem, (int)s->p.problem_mark.line+1,
			     (int)s->p.problem_mark.column+1);
		}
		break;
		
	    default:
		/* Couldn't happen. */
		snprintf(s->error_str,
			 error_size,
			 "Internal error");
		break;
	    }
	    goto end;
	}
	
	switch (event.type) { 
	case YAML_NO_EVENT: 
	    dbg_printf("~~~No event!\n"); 
	    break;
	    /* Stream start/end */
	case YAML_STREAM_START_EVENT: 
	    dbg_printf("~~~STREAM START\n"); 
	    break;
	case YAML_STREAM_END_EVENT:   
	    dbg_printf("~~~STREAM END\n");   
	    break;
	    /* Block delimeters */
	case YAML_DOCUMENT_START_EVENT: 
	    dbg_printf("~~~Start Document\n"); 

	    /* Create child user data and set yc pointer to it */
	    yc = yaml_context_push(yc);
	    yc->context_type = document_context_type;
	    
	    /* Document object can be of any type - initialize to void */
	    yc->object.type = bitd_type_void;
	    break;
	case YAML_DOCUMENT_END_EVENT:   
	    dbg_printf("~~~End Document\n");   

	    yc = yaml_context_pop(yc);
	    bitd_assert(!yc->depth);
	    s->doc_count++;

	    /* Move the object as new element in the stream_nvp.
	       To accomplish the move, we add a void element, 
	       then we move the contents. */
	    bitd_nvp_add_elem(&s->stream_nvp,
			    "_doc",
			    NULL,
			    bitd_type_void);
	    idx = s->stream_nvp->n_elts - 1;
	    s->stream_nvp->e[idx].type = s->object.type;
	    bitd_value_copy(&s->stream_nvp->e[idx].v,
			  &s->object.v,
			  s->object.type);

	    /* Reinit the object */
	    bitd_object_init(&s->object);
	    break;
	case YAML_SEQUENCE_START_EVENT: 
	    dbg_printf("~~~Start Sequence\n"); 

	    /* Create child user data and set yc pointer to it */
	    yc = yaml_context_push(yc);
	    yc->context_type = sequence_context_type;

	    /* Sequence object is of nvp type, with NULL names */
	    yc->object.type = bitd_type_nvp;

	    if (event.data.sequence_start.anchor) {
		dbg_printf(">>>Got sequence anchor '%s'\n", 
			   event.data.sequence_start.anchor); 
		yc->anchor = yaml_anchor_add(s, 
					     event.data.sequence_start.anchor,
					     yc);
	    }
	    break;
	case YAML_SEQUENCE_END_EVENT:   
	    dbg_printf("~~~End Sequence\n");   

	    if (yc->depth > 1) {
		if (yc->prev->name && !strcmp(yc->prev->name, "<<")) {
		    /* End of merge sequence. Clear the parent name. */
		    free(yc->prev->name);
		    yc->prev->name = NULL;
		}
	    }

	    /* Pop the yaml context. As a side effect, parsed elements will
	       be written into the parent object. */
	    yc = yaml_context_pop(yc);

	    break;
	case YAML_MAPPING_START_EVENT:  
	    dbg_printf("~~~Start Mapping\n");  

	    /* Create child user data and set u pointer to it */
	    yc = yaml_context_push(yc);
	    yc->context_type = mapping_context_type;

	    /* Mapping object is of nvp type, with non-NULL names */
	    yc->object.type = bitd_type_nvp;

	    if (event.data.mapping_start.anchor) {
		dbg_printf(">>>Got mapping anchor '%s'\n", 
			   event.data.sequence_start.anchor); 
		yc->anchor = yaml_anchor_add(s, 
					     event.data.mapping_start.anchor,
					     yc);
	    }
	    break;
	case YAML_MAPPING_END_EVENT:    
	    dbg_printf("~~~End Mapping\n");    

	    yc = yaml_context_pop(yc);
	    break;
	    /* Data */
	case YAML_ALIAS_EVENT:  
	    dbg_printf(">>>Got alias (anchor '%s')\n", 
		       event.data.alias.anchor); 

	    anchor = yaml_anchor_get(yc->s, event.data.alias.anchor);
	    if (!anchor) {
		s->error_str = calloc(1, error_size + 1);		
		snprintf(s->error_str, error_size, 
			 "Unreferenced anchor '%s'",
			 event.data.alias.anchor);
		yaml_event_delete(&event);
		goto end;
	    }

	    if (yc->context_type == mapping_context_type) {
		/* Are we asked to merge the alias? */
		if (yc->name && !strcmp(yc->name, "<<")) {
		    if (anchor->object.type != bitd_type_nvp ||
			!anchor->object.v.value_nvp ||
			!anchor->object.v.value_nvp->n_elts ||
			!anchor->object.v.value_nvp->e[0].name) {
			s->error_str = calloc(1, error_size + 1);		
			snprintf(s->error_str, error_size, 
				 "Merged anchor '%s' not of map type",
				 event.data.alias.anchor);
			yaml_event_delete(&event);
			goto end;
		    }
		    yc->merged_nvp_mapping = 
			bitd_nvp_clone(anchor->object.v.value_nvp);
		    if (yc->name) {
			free(yc->name);
			yc->name = NULL;
		    }
		    break;
		}
	    }
	    if (yc->context_type == sequence_context_type) {
		/* Are we asked to merge the alias? */
		if (yc->depth > 1 &&
		    yc->prev->context_type == mapping_context_type &&
		    yc->prev->name &&
		    !strcmp(yc->prev->name, "<<")) {

		    if (anchor->object.type != bitd_type_nvp ||
			!anchor->object.v.value_nvp ||
			!anchor->object.v.value_nvp->n_elts ||
			!anchor->object.v.value_nvp->e[0].name) {
			s->error_str = calloc(1, error_size + 1);		
			snprintf(s->error_str, error_size, 
				 "Merged anchor '%s' not of map type",
				 event.data.alias.anchor);
			yaml_event_delete(&event);
			goto end;
		    }
		    
		    v.value_nvp = anchor->object.v.value_nvp;
		    bitd_nvp_add_elem(&yc->prev->merged_nvp_list,
				    NULL,
				    &v, bitd_type_nvp);

		    if (yc->name) {
			free(yc->name);
			yc->name = NULL;
		    }
		    break;
		}
	    }

	    /* Add the anchor value */
	    if (yc->context_type == sequence_context_type ||
		yc->context_type == mapping_context_type) {
		bitd_assert(yc->object.type == bitd_type_nvp);

		/* Add the value to the current-level nvp */
		bitd_nvp_add_elem(&(yc->object.v.value_nvp), 
				yc->name, 
				&anchor->object.v, 
				anchor->object.type);
	    } else {
		bitd_object_free(&yc->object);
		bitd_object_clone(&yc->object, &anchor->object);
	    }

	    if (yc->name) {
		free(yc->name);
		yc->name = NULL;
	    }
	    break;
	case YAML_SCALAR_EVENT: 
	    if (event.data.scalar.tag) {
 		dbg_printf(">>>Got scalar tag '%s'\n", 
			   event.data.scalar.tag); 
	    }
	    dbg_printf(">>>Got scalar (value '%s')\n", 
		       event.data.scalar.value); 

	    if (yc->context_type == mapping_context_type) {
		if (!yc->name) {
		    /* First scalar represents name. Save the name. */
		    bitd_assert(!event.data.scalar.tag);
		    yc->name = strdup((char *)event.data.scalar.value);
		    break;
		}
	    } else if (yc->context_type == sequence_context_type) {
		bitd_assert(!yc->name);
	    }
	
	    /* Scalar represents non-nvp value. */ 
	    if (yc->context_type == mapping_context_type) {
		if (!strcmp(yc->name, "<<")) {
		    s->error_str = calloc(1, error_size + 1);		
		    snprintf(s->error_str, error_size, 
			     "Merge operator '<<' should not have scalar value");
		    yaml_event_delete(&event);
		    goto end;		    
		}
	    }

	    /* Save the value to the parent nvp. */
	    if (!event.data.scalar.tag) {
		/* Convert to value and type */
		bitd_string_to_type_and_value(&t, &v, 
					      (char *)event.data.scalar.value);
	    } else {
		int idx = sizeof("tag:yaml.org,2002:") - 1;
		
		/* Initialize type and value  */
		t = bitd_type_string;

		if (!strncmp((char *)event.data.scalar.tag,
			     "tag:yaml.org,2002:",
			     idx)) { 
		    char *c = (char *)event.data.scalar.tag + idx;
		    
		    if (!strcmp(c, "null")) {
			t = bitd_type_void;
		    } else if (!strcmp(c, "bool")) {
			t = bitd_type_boolean;
		    } else if (!strcmp(c, "int")) {
			t = bitd_type_int64;
		    } else if (!strcmp(c, "float")) {
			t = bitd_type_double;
		    } else if (!strcmp(c, "str")) {
			t = bitd_type_string;
		    } else if (!strcmp(c, "binary")) {
			t = bitd_type_blob;
		    } 
		}
		if (!bitd_typed_string_to_value(&v, 
					      (char *)event.data.scalar.value, 
					      t)) {
		    if (!s->error_str) {
			s->error_str = yaml_error("Invalid element '%s' value '%s'",
						  yc->name, 
						  event.data.scalar.value);
			break;
		    }
		} 
	    }
	    
	    if (event.data.scalar.anchor) {
 		dbg_printf(">>>Got scalar anchor '%s'\n", 
			   event.data.scalar.anchor); 
		anchor = yaml_anchor_add(s, 
					 event.data.scalar.anchor,
					 yc);
		anchor->object.type = t;
		bitd_value_clone(&anchor->object.v, &v, t);
		anchor->yaml_context = NULL;
	    }
	
	    /* Clear the anchor in the yaml context - we've added
	       a free floating anchor, because the value is complete */
	    yc->anchor = NULL;

	    if (yc->context_type == sequence_context_type ||
		yc->context_type == mapping_context_type) {
		bitd_assert(yc->object.type == bitd_type_nvp);
		/* Add the value to the current-level nvp */
		bitd_nvp_add_elem(&(yc->object.v.value_nvp), 
				yc->name, &v, t);
		
		bitd_value_free(&v, t);
	    } else {
		bitd_object_free(&yc->object);
		yc->object.type = t;
		yc->object.v = v;
	    }
	    
	    if (yc->name) {
		free(yc->name);
		yc->name = NULL;
	    }
	    break;
	}

        /* Check if this is the stream end. */	
        if (event.type == YAML_STREAM_END_EVENT) {
            done = 1;
        }

	yaml_event_delete(&event);
    }

 end:
    /* If the error is set, return FALSE. */
    if (s->error_str) {
	bitd_object_free(&s->object);
	bitd_object_init(&s->object);
	bitd_nvp_free(s->stream_nvp);
	s->stream_nvp = NULL;
	return FALSE;
    }

    /* Else, no parse error. */
    return TRUE;
} 


/*
 *============================================================================
 *                        bitd_yaml_parser_get_document_object
 *============================================================================
 * Description:     Get a copy of the document object, if document was not
 *               a stream.
 * Parameters:    
 * Returns:  
 */
void bitd_yaml_parser_get_document_object(bitd_object_t *a, 
					  bitd_yaml_parser s) {

    if (!a) {
	return;
    }

    bitd_object_init(a);
    if (s && s->stream_nvp && s->stream_nvp->n_elts > 0) {
	a->type = s->stream_nvp->e[0].type;
	bitd_value_clone(&a->v, &s->stream_nvp->e[0].v, a->type);
    }
} 


/*
 *============================================================================
 *                        bitd_yaml_parser_get_document_nvp
 *============================================================================
 * Description:     Get a copy of the document nvp, if document was not
 *               a stream, and the document results in an nvp.
 * Parameters:    
 * Returns:  
 */
bitd_nvp_t bitd_yaml_parser_get_document_nvp(bitd_yaml_parser s) {

    if (s && s->stream_nvp && s->stream_nvp->n_elts > 0 && 
	s->stream_nvp->e[0].type == bitd_type_nvp) {
	return bitd_nvp_clone(s->stream_nvp->e[0].v.value_nvp);
    }

    return NULL;
} 


/*
 *============================================================================
 *                        bitd_yaml_parser_get_stream_nvp
 *============================================================================
 * Description:     Get the stream nvp
 * Parameters:    
 * Returns:  
 */
bitd_nvp_t bitd_yaml_parser_get_stream_nvp(bitd_yaml_parser s) {

    if (s) {
	return bitd_nvp_clone(s->stream_nvp);
    }
    
    return NULL;
} 


/*
 *============================================================================
 *                        bitd_yaml_parser_get_error
 *============================================================================
 * Description:     Get the parse error string, if any
 * Parameters:    
 * Returns:  
 */
char *bitd_yaml_parser_get_error(bitd_yaml_parser s) {
    if (s) {
	return s->error_str;
    }
    return NULL;
} 


/*
 *============================================================================
 *                        yaml_error
 *============================================================================
 * Description:     Allocate an error message on the heap, and return
 *           the pointer.
 * Parameters:    
 * Returns:  The error message. Must be freed by caller.
 */
char *yaml_error(char *fmt, ...) {
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


