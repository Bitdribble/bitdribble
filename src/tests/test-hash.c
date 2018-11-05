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
#include "bitd/hash.h"
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
static char *g_prog_name = "";
static void *s_cookie = (void *)0xabcd;
static int s_element_count;


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
static void usage() {

    printf("\nUsage: %s [OPTIONS ... ]\n\n", g_prog_name);
    printf("This program tests the hash API.\n\n");

    printf("Options:\n"
           "    -n count\n"
           "            Number of elements in hash\n"
           "    -h, --help, -?\n"
           "            Show this help.\n");
} 



/*
 *============================================================================
 *                        hash_free
 *============================================================================
 * Description:
 * Parameters:
 * Returns:
 */
static void hash_free(bitd_hash_key k, bitd_hash_value v) {
    free(k);
}


/*
 *============================================================================
 *                        hash_map
 *============================================================================
 * Description:
 * Parameters:
 * Returns:
 */
static void hash_map(bitd_hash_key k, bitd_hash_value v, void *cookie) {
    bitd_assert(cookie == s_cookie);
    bitd_assert(atoll((char *)k) == (long long)v);

#if 0
    printf("Key %s\n", (char *)k);
#endif
    s_element_count++;
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
    bitd_hash h;
    int i, n_elements = 10;
    bitd_boolean bool_ret;
    char *c;

    /* Work around compiler warning */
    (void)bool_ret;

    bitd_sys_init();

    /* Parse program name argument */
    g_prog_name = bitd_get_leaf_filename(argv[0]);

    /* Skip to next parameter */
    argc--;
    argv++;

    /* Parse vector size and help */
    while (argc) {
        if (!strcmp(argv[0], "-n")) {

            /* Skip to next parameter */
            argc--;
            argv++;
            
            if (!argc) {
                usage();
		exit(-1);
            }

            /* Get the timeout */
            n_elements = atoi(argv[0]);
            
        } else if (!strcmp(argv[0], "-h") ||
                   !strcmp(argv[0], "--help") ||
                   !strcmp(argv[0], "-?")) {
            usage();
	    exit(0);
        } else {
            printf("%s: Skipping invalid parameter %s\n", g_prog_name, argv[0]);
        }

        /* Skip to next argument */
        argc--;
        argv++;        
    }

    /* Create the hash */
    h = bitd_hash_create(&bitd_hash_func_string,
                       &bitd_hash_compare_string,
                       &hash_free);

    /* Insert a few elements */
    for (i = 0; i < n_elements; i++) {
        c = malloc(10 + i/10);
        sprintf(c, "%d", i);
        bool_ret = bitd_hash_add(h, (bitd_hash_key)c, (bitd_hash_value)(long long)i);
	bitd_assert(bool_ret);
    }

    /* Test the mapping API */
    s_element_count = 0;
    bitd_hash_map(h, &hash_map, s_cookie);
    bitd_assert(s_element_count == n_elements);

    /* Test re-adding the elements */
    for (i = 0; i < n_elements; i++) {
        c = malloc(10 + i/10);
        sprintf(c, "%d", i);
        bool_ret = bitd_hash_add(h, (bitd_hash_key)c, (bitd_hash_value)(long long)i);
        bitd_assert(!bool_ret);
        free(c);
    }
    
    /* Test the mapping API again */
    s_element_count = 0;
    bitd_hash_map(h, &hash_map, s_cookie);
    bitd_assert(s_element_count == n_elements);

    /* Test deleting the elements */
    for (i = 0; i < n_elements; i++) {
        c = malloc(10 + i/10);
        sprintf(c, "%d", i);
        bool_ret = bitd_hash_remove(h, (bitd_hash_key)c);
        bitd_assert(bool_ret);
        free(c);
    }
    
    /* Test the mapping API again */
    s_element_count = 0;
    bitd_hash_map(h, &hash_map, s_cookie);
    bitd_assert(!s_element_count);

    /* Test re-adding the elements */
    for (i = 0; i < n_elements; i++) {
        c = malloc(10 + i/10);
        sprintf(c, "%d", i);
        bool_ret = bitd_hash_add(h, (bitd_hash_key)c, (bitd_hash_value)(long long)i);
        bitd_assert(bool_ret);
    }

    /* Test the mapping API again */
    s_element_count = 0;
    bitd_hash_map(h, &hash_map, s_cookie);
    bitd_assert(s_element_count == n_elements);

    /* Test the destruction of the hash */
    bitd_hash_destroy(h);

    bitd_sys_deinit();

    return 0;
}

