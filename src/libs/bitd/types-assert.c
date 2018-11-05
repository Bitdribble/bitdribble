/*****************************************************************************
 *
 * Original Author: 
 * Creation Date:   
 * Description: 
 * 
 * Copyright (C) by Andrei Radulescu-Banu, 2018. All Rights Reserved.
 * Unauthorized reproduction, modification, distribution, transmission, 
 * republication, display or performance are strictly prohibited.
 ****************************************************************************/

/*****************************************************************************
 *                                INCLUDE FILES 
 *****************************************************************************/
#include "bitd/types.h"



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
 *                        map_object_node_assert
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
static void map_object_node_assert(object_node_t *node, void *cookie) {
    char *full_name = node->full_name;
    char *name = node->name;
    bitd_type_t type = node->a.type;
    bitd_value_t *v = &node->a.v;

    if (full_name && full_name[0]) {
	_bitd_assert(strlen(full_name));
    }
    if (name && name[0]) {
	_bitd_assert(strlen(name));
    }

    /* Format the value */
    switch (type) {
    case bitd_type_string:
	if (v->value_string && v->value_string[0]) {
	    _bitd_assert(v->value_string);
	}
	break;
    case bitd_type_blob:
	/* No-op */
	break;
    case bitd_type_nvp:
	_bitd_assert(!v->value_nvp);
	break;
    default:
	break;	
    }
} 


/*
 *============================================================================
 *                        _bitd_assert_object
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
void _bitd_assert_object(bitd_object_t *a) {
    object_node_t node;

    node.full_name = NULL;
    node.name = NULL;
    node.sep = '.';
    node.depth = 0;
    node.a = *a;

    bitd_object_node_map(&node, &map_object_node_assert, NULL);
} 


/*
 *============================================================================
 *                        _bitd_assert_nvp
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
void _bitd_assert_nvp(bitd_nvp_t nvp) {
    object_node_t node;

    node.full_name = NULL;
    node.name = NULL;
    node.sep = '.';
    node.depth = 0;
    node.a.type = bitd_type_nvp;
    node.a.v.value_nvp = nvp;

    bitd_object_node_map(&node, &map_object_node_assert, NULL);
} 


/*
 *============================================================================
 *                        _bitd_assert_value
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
void _bitd_assert_value(bitd_value_t *v, bitd_type_t type) {
    object_node_t node;

    node.full_name = NULL;
    node.name = NULL;
    node.sep = '.';
    node.depth = 0;
    node.a.type = type;
    node.a.v = *v;

    bitd_object_node_map(&node, &map_object_node_assert, NULL);
} 
