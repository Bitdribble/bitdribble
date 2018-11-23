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
#include "bitd/common.h"
#include "bitd/msg.h"
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
bitd_int32 g_verbose;

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
    printf("This program tests the queue API.\n\n");

    printf("Options:\n"
           "  -n test_count\n"
           "    Run only this test.\n"
           "  -v verbose_level, --verbose verbose_level\n"
           "    Set the verbosity level (default: 2).\n"
           "  -h, --help, -?\n"
           "    Show this help.\n");
} 


/*
 *============================================================================
 *                        test1
 *============================================================================
 * Description:     Create queues with the same name
 * Parameters:    
 * Returns:  
 */
static int test1(void) {
    int ret = 0;
    bitd_queue q0 = NULL, q1 = NULL, q2 = NULL, q3 = NULL;

    q0 = bitd_queue_create(NULL, 0, 0);
    q1 = bitd_queue_create(NULL, 0, 0);
    q2 = bitd_queue_create("foo", 0, 0);
    q3 = bitd_queue_create("foo", 0, 0);
    
    if (!q0 || !q1) {
	if (g_verbose > 0) {
	    printf("Could not create queue with NULL name\n");
	}
	ret = -1;
	goto end;
    }

    if (!q2) {
	if (g_verbose > 0) {
	    printf("Could not create queue with non-NULL name\n");
	}
	ret = -1;
	goto end;
    }

    if (!q3) {
	if (g_verbose > 0) {
	    printf("Should allow 2nd queue with same non-NULL name\n");
	}
	ret = -1;
	goto end;
    }

 end:
    bitd_queue_destroy(q0);
    bitd_queue_destroy(q1);
    bitd_queue_destroy(q2);
    bitd_queue_destroy(q3);

    return ret;
} 


/*
 *============================================================================
 *                        test2
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
static int test2(void) {
    int ret = 0;
    bitd_queue q0 = NULL, q1 = NULL;

    q0 = bitd_queue_create(NULL, 0, 0);
    q1 = bitd_queue_create(NULL, 0, 0);

    bitd_queue_destroy(q0);
    bitd_queue_destroy(q1);

    return ret;
} 


/*
 *============================================================================
 *                        test
 *============================================================================
 * Description:     Run the test indicated by test_idx
 * Parameters:    
 *     test_idx - the index of the test
 * Returns:  Exit code of the test
 */
static int test(int test_idx) {

    switch (test_idx) {
    case 0:
	return test1();
    case 1:
	return test2();
    default:
	break;
    }

    return 0;
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
    int ret = 0;
    int i;
    bitd_boolean run_all_tests_p = TRUE;

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
	    
	    run_all_tests_p = FALSE;

            /* Run this one test */
            ret = test(atoi(argv[0]));
	    if (ret) {
		/* Skip to end when test exits with non-zero code */
		goto end;
	    }

        } else if (!strcmp(argv[0], "-v") ||
                   !strcmp(argv[0], "--verbose")) {

            /* Skip to next parameter */
            argc--;
            argv++;
            
            if (!argc) {
                usage();
		exit(-1);
            }

            /* Get the verbose level */
            g_verbose = atoi(argv[0]);
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

    if (run_all_tests_p) {
	for (i = 0; i < 1; i++) {
	    ret = test(i);
	    if (ret) {
		goto end;
	    }
	}
    }

 end:
    bitd_sys_deinit();

    return ret;
}
