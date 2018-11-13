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

#ifndef _BITD_TYPES_H_
#define _BITD_TYPES_H_

/*****************************************************************************
 *                                INCLUDE FILES 
 *****************************************************************************/
#include "bitd/platform-types.h"


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


/* Type utilities */
extern bitd_type_t bitd_get_type_t(char *type_name);
extern char *bitd_get_type_name(bitd_type_t t);

/* Object utilities */
extern void bitd_object_init(bitd_object_t *a);
extern void bitd_object_clone(bitd_object_t *a1 /* OUT */, 
			      bitd_object_t *a2);
extern void bitd_object_copy(bitd_object_t *a1 /* OUT */, 
			     bitd_object_t *a2);
extern void bitd_object_free(bitd_object_t *a);


/* Returns 0 if the same values, -1 if a1 < a2, 1 otherwise */
extern int bitd_object_compare(bitd_object_t *a1, 
			       bitd_object_t *a2);

/* Format to/from string */
extern char *bitd_object_to_string(bitd_object_t *a);
extern bitd_boolean bitd_typed_string_to_object(bitd_object_t *a, /* OUT */
						char *value_str, 
						bitd_type_t t);

/* Get the default type of a string, and also its value */
extern void bitd_string_to_object(bitd_object_t *a, /* OUT */
				  char *value_str);

/* Value utilities */
extern void bitd_value_init(bitd_value_t *v);
extern void bitd_value_clone(bitd_value_t *v1 /* OUT */, bitd_value_t *v2, 
			     bitd_type_t t) ;
extern void bitd_value_copy(bitd_value_t *v1 /* OUT */, bitd_value_t *v2, 
			    bitd_type_t t) ;
extern void bitd_value_free(bitd_value_t *v, bitd_type_t t);


/* Returns 0 if the same values */
extern int bitd_value_compare(bitd_value_t *v1, bitd_value_t *v2, 
			      bitd_type_t t) ;

/* Format to/from string */
extern char *bitd_value_to_string(bitd_value_t *v, bitd_type_t t);
extern bitd_boolean bitd_typed_string_to_value(bitd_value_t *v, /* OUT */
					       char *value_str, bitd_type_t t);

/* Get the default type of a string, and also its value */
extern void bitd_string_to_type_and_value(bitd_type_t *t, /* OUT */
					  bitd_value_t *v, /* OUT */
					  char *value_str);

/* When are doubles approximately the same, up to 9th decimal? */
int bitd_double_approx_same(double a, double b);

/* Get precision of double */
int bitd_double_precision(double a);

/* Blob apis */
extern bitd_blob *bitd_blob_alloc(int blob_size);
extern bitd_blob *bitd_blob_clone(bitd_blob *v);
extern int bitd_blob_compare(bitd_blob *v1, 
			     bitd_blob *v2);
extern char *bitd_blob_to_string_hex(bitd_blob *v);
extern bitd_boolean bitd_string_hex_to_blob(bitd_blob **v /* OUT */, 
					    char *value_str);
extern char *bitd_blob_to_string_base64(bitd_blob *v);
extern bitd_boolean bitd_string_base64_to_blob(bitd_blob **v /* OUT */, 
					       char *value_str);

/* Nvp apis */
extern bitd_nvp_t bitd_nvp_alloc(int n_elts);
extern bitd_nvp_t bitd_nvp_clone(bitd_nvp_t nvp);
extern void bitd_nvp_free(bitd_nvp_t nvp);
extern int bitd_nvp_elem_compare(bitd_nvp_element_t *e1, 
				 bitd_nvp_element_t *e2);
extern int bitd_nvp_compare(bitd_nvp_t nvp1, bitd_nvp_t nvp2);
extern void bitd_nvp_sort(bitd_nvp_t nvp);
extern bitd_boolean bitd_nvp_add_elem(bitd_nvp_t *pnvp, 
				      char *name, 
				      bitd_value_t *v, bitd_type_t type);
extern void bitd_nvp_delete_elem(bitd_nvp_t nvp, 
				 char *elem_name);
extern void bitd_nvp_delete_elem_by_idx(bitd_nvp_t nvp, 
					int idx);
extern bitd_boolean bitd_nvp_lookup_elem(bitd_nvp_t nvp, 
					 char *elem_name,
					 int *idx);
extern bitd_nvp_t bitd_nvp_trim(bitd_nvp_t nvp, 
				char **elem_names,
				int n_elem_names);
extern bitd_nvp_t bitd_nvp_trim_bytype(bitd_nvp_t nvp, 
				       char **elem_names,
				       int n_elem_names,
				       bitd_type_t type);
extern bitd_nvp_t bitd_nvp_chunk(bitd_nvp_t nvp);
extern bitd_nvp_t bitd_nvp_unchunk(bitd_nvp_t nvp);
extern bitd_nvp_t bitd_nvp_merge(bitd_nvp_t old_nvp, 
				 bitd_nvp_t new_nvp,
				 bitd_nvp_t base_nvp);

extern int bitd_nvp_name_len_max(bitd_nvp_t v);
extern int bitd_nvp_to_string_size(bitd_nvp_t v, int prefix_len);

extern char *bitd_nvp_to_string(bitd_nvp_t nvp, char *prefix);
extern bitd_boolean bitd_string_to_nvp(bitd_nvp_t *nvp /* OUT */, 
				       char *value_str);

/* An object node represents an object embedded possibly deep into another
   nvp-type object. Used by map functions to pass around data. */
typedef struct object_node_s {
    char *full_name;
    char *name;
    char sep;
    int depth;
    bitd_object_t a;
} object_node_t;

typedef void (object_node_map_func_t)(object_node_t *node,
				      void *cookie);

/* Recursively map an object, calling f(a_node, cookie) for each non-nvp
   value as well as for each NULL nvp value. We recurse over non-NULL nvps. */
extern void bitd_object_node_map(object_node_t *node, 
				 object_node_map_func_t *f, 
				 void *cookie);
    
typedef void (object_map_func_t)(void *cookie, 
				 char *full_name, char *name, int depth,
				 bitd_value_t *v, bitd_type_t type);

/* Recursively map an object, calling f(cookie, v, type) for each non-nvp
   value as well as for each NULL nvp value. We recurse over non-NULL nvps. */
extern void bitd_object_map(char *name, char sep, int depth, bitd_object_t *a, 
			    object_map_func_t *f, void *cookie);

/* Same for an nvp and a value */
extern void bitd_nvp_map(char *name, char sep, int depth, bitd_nvp_t nvp, 
			 object_map_func_t *f, void *cookie);
extern void bitd_value_map(char *name, char sep, int depth, 
			   bitd_value_t *v, bitd_type_t type,
			   object_map_func_t *f, void *cookie);

/* Assert the validity of an object, nvp, value */
extern void _bitd_assert_object(bitd_object_t *a);
extern void _bitd_assert_nvp(bitd_nvp_t nvp);
extern void _bitd_assert_value(bitd_value_t *v, bitd_type_t type);

#ifdef NDEBUG
# define bitd_assert_object(a) do {} while (0)
# define bitd_assert_nvp(nvp) do {} while (0)
# define bitd_assert_value(v, type) do {} while (0)
#else
# define bitd_assert_object(a) _bitd_assert_object(a)
# define bitd_assert_nvp(nvp) _bitd_assert_nvp(nvp)
# define bitd_assert_value(v, type) _bitd_assert_value(v, type)
#endif

/* Buffer types */
typedef enum {
    bitd_buffer_type_auto = 0,
    bitd_buffer_type_string,
    bitd_buffer_type_blob,
    bitd_buffer_type_xml,
    bitd_buffer_type_yaml
} bitd_buffer_type_t;

/* Parse buffer and convert to object */
extern void bitd_buffer_to_object(bitd_object_t *a, char **object_name,
				  char *buf, int buf_nbytes,
				  bitd_buffer_type_t buffer_type);

/* Print object into buffer */
extern void bitd_object_to_buffer(char **buf, int *buf_nbytes,
				  bitd_object_t *a, 
				  char *object_name,
				  bitd_buffer_type_t buffer_type);

/*
 * Json apis
 */
/* Convert object to xml */
extern char *bitd_object_to_json(bitd_object_t *a,
				 bitd_boolean full_xml);

extern char *bitd_object_to_json_element(bitd_object_t *a,
					 int indentation, /* How much to indent */
					 bitd_boolean full_json);

/*
 * Xml apis
 */

/* Convert object to xml */
extern char *bitd_object_to_xml(bitd_object_t *a,
				char *object_name, /* '_' if not set */
				bitd_boolean full_xml);

extern char *bitd_object_to_xml_element(bitd_object_t *a,
					char *object_name, /* '_' if not set */
					int indentation, /* How much to indent */
					bitd_boolean full_xml);

/* Convert nvp to xml */
extern char *bitd_nvp_to_xml(bitd_nvp_t nvp,
			     char *nvp_name, /* '_' if not set */
			     bitd_boolean full_xml);
extern char *bitd_nvp_to_xml_elem(bitd_nvp_t nvp, 
				  char *nvp_name, /* '_' if not set */
				  int indentation,
				  bitd_boolean full_xml);

/* Convert xml to object */
bitd_boolean bitd_xml_to_object(bitd_object_t *a, char **object_name,
				char *xml, int xml_nbytes);

/* Convert xml to nvp, if the xml converts to an nvp type object */
bitd_boolean bitd_xml_to_nvp(bitd_nvp_t *nvp, char *xml, int xml_nbytes);

/* The xml stream handle */
typedef struct bitd_xml_stream_s *bitd_xml_stream;

/* Init/deinit an xml stream, used for parsing xml into nvp */
extern bitd_xml_stream bitd_xml_stream_init(void);
extern void bitd_xml_stream_free(bitd_xml_stream s);

/* Set done=TRUE at end of xml stream. Returns FALSE on parse error */
extern bitd_boolean bitd_xml_stream_read(bitd_xml_stream s, 
					 char *buf, int size, 
					 bitd_boolean done);
/* The parsed object and object name, retrievable only after done=TRUE. */
extern void bitd_xml_stream_get_object(bitd_xml_stream s,
				       bitd_object_t *a,
				       char **object_name);
/* The parsed object as nvp, if the object is an nvp */
extern bitd_nvp_t bitd_xml_stream_get_nvp(bitd_xml_stream s);

/* Get parse error string */
extern char *bitd_xml_stream_get_error(bitd_xml_stream s);

/*
 * Yaml apis
 */

/* Convert object to yaml */
extern char *bitd_object_to_yaml(bitd_object_t *a, 
				 bitd_boolean full_yaml,
				 bitd_boolean is_stream);

/* Convert nvp to yaml */
extern char *bitd_nvp_to_yaml(bitd_nvp_t nvp, 
			      bitd_boolean full_yaml,
			      bitd_boolean is_stream);

/* Convert yaml to object. The is_stream OUT parameter is set to TRUE if
   the yaml document is a stream (in which case the object is an nvp
   with one sub-object named _doc per document in the stream) */
bitd_boolean bitd_yaml_to_object(bitd_object_t *a, 
				 char *yaml, int yaml_nbytes,
				 bitd_boolean *is_stream,
				 char *err_buf,
				 int err_len);

/* Same as above - but return the object as nvp (if it is an nvp), or NULL
   (if it is not an nvp) */
bitd_boolean bitd_yaml_to_nvp(bitd_nvp_t *nvp, char *yaml, int yaml_nbytes,
			      bitd_boolean *is_stream,
			      char *err_buf,
			      int err_len);

/* The yaml parser handle */
typedef struct bitd_yaml_parser_s *bitd_yaml_parser;

/* Init/deinit a yaml parser */
extern bitd_yaml_parser bitd_yaml_parser_init(void);
extern void bitd_yaml_parser_free(bitd_yaml_parser s);

/* Returns FALSE on parse error */
extern bitd_boolean bitd_yaml_parse(bitd_yaml_parser s, 
				    char *buf, int size);

/* The yaml doc parsed as object. If the yaml stream has multiple docs,
   this is the last in the stream, and intermediate docs are reported 
   through the hook mechanism. */
extern void bitd_yaml_parser_get_document_object(bitd_object_t *a, 
						 bitd_yaml_parser s);

/* The yaml doc parsed as nvp, if the yaml object is an nvp. */
extern bitd_nvp_t bitd_yaml_parser_get_document_nvp(bitd_yaml_parser s);


/* If the yaml doc was a stream, this returns the stream nvp. If the yaml
   doc was not a stream, this returns NULL. */
extern bitd_nvp_t bitd_yaml_parser_get_stream_nvp(bitd_yaml_parser s);

/* Get parse error string */
extern char *bitd_yaml_parser_get_error(bitd_yaml_parser s);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _BITD_TYPES_H_ */
