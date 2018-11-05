/*****************************************************************************
 *
 * Original Author: Andrei Radulescu-Banu
 * Creation Date:   
 * Description: 
 * 
 * Copyright 2018 by Andrei Radulescu-Banu.  All Rights Reserved.
 * Unauthorized reproduction, modification, distribution, transmission, 
 * republication, display or performance are strictly prohibited.
 ****************************************************************************/

/*****************************************************************************
 *                                INCLUDE FILES 
 *****************************************************************************/
#include "bitd/types.h"
#include "bitd/pack.h"
#include "bitd/file.h"

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
static char* g_prog_name = "";

/*****************************************************************************
 *                          FUNCTION IMPLEMENTATION
 *****************************************************************************/



/*
 *============================================================================
 *                        usage
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
void usage() {

    printf("\nUsage: %s [OPTIONS ... ]\n\n", g_prog_name);
    printf("This program tests the pack API.\n\n");

    printf("Options:\n"
           "    -t type value\n"
           "       Pack and unpack value.\n"
           "    -h, --help, -?\n"
           "       Show this help.\n");

    exit(0);
} 



/*
 *============================================================================
 *                        test_pack
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
int test_pack(char *type, char *value) {
    int ret = 0;
    bitd_type_t t;
    bitd_object_t a1, a2;
    char buf[128];
    int idx = 0, idx1 = 0;

    bitd_object_init(&a1);
    bitd_object_init(&a2);

    t = bitd_get_type_t(type);
    if (t >= bitd_type_max) {
	printf("%s: Unsupported type '%s'.\n", g_prog_name, type);
	ret = 1;
	goto end;
    }

    if (!bitd_typed_string_to_object(&a1, value, t)) {
	printf("%s: Invalid value '%s' for type %s.\n", 
	       g_prog_name, value, bitd_get_type_name(t));
	ret = 1;
	goto end;
    }

    if (!bitd_pack_object(buf, sizeof(buf), &idx, &a1) ||
	!bitd_unpack_object(buf, sizeof(buf), &idx1, &a2)) {
	bitd_assert(0);
	ret = 1;
	goto end;
    }

    /* Are they the same value? Exact comparison for non-double */
    if (t != bitd_type_double) {
	if (!bitd_object_compare(&a1, &a2)) {
	    ret = 0;
	    goto end;
	}
    } else if (!bitd_double_approx_same(a1.v.value_double, a2.v.value_double)) {
	/* The values are approximately the same */
	ret = 0;
	goto end;
    }

    ret = 1;

 end:
    bitd_object_free(&a1);
    bitd_object_free(&a2);

    return ret;
} 



/*
 *============================================================================
 *                        main
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
int main(int argc, char ** argv) {
    int ret;

    /* Parse program name argument */
    g_prog_name = bitd_get_leaf_filename(argv[0]);

    /* Skip to next parameter */
    argc--;
    argv++;

    /* Parse vector size and help */
    while (argc) {
        if (!strcmp(argv[0], "-t")) {

            /* Skip to next parameter */
            argc--;
            argv++;
            
            if (argc < 2) {
                usage();
            }

	    ret = test_pack(argv[0], argv[1]);
	    if (ret) {
		return ret;
	    }
	    
            /* Skip to next parameter */
            argc--;
            argv++;

        } else if (!strcmp(argv[0], "-h") ||
                   !strcmp(argv[0], "--help") ||
                   !strcmp(argv[0], "-?")) {
            usage();
        } else {
            printf("%s: Skipping invalid parameter %s\n", g_prog_name, argv[0]);
        }

        /* Skip to next argument */
        argc--;
        argv++;        
    }

    return 0;
}
