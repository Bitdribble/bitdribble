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
static int verbose = 2;


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
    printf("This program tests the time API.\n\n");

    printf("Options:\n"
           "    -c loop_count, --count loop_count\n"
           "            Number of times to loop.\n"
           "            Default: 100.\n"
           "    -i loop_interval, --interval loop_interval\n"
           "            Number of milliseconds to sleep between loops.\n"
           "            Default: 100.\n"
           "    -v verbose_level, --verbose verbose_level\n"
           "            Set the verbosity level (default: 2).\n"
           "    -h, --help, -?\n"
           "            Show this help.\n");

    exit(0);
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
    int count = 100, interval = 100, i;
    struct timeval time, time_last;

    bitd_sys_init();

    /* Parse program name argument */
    g_prog_name = bitd_get_leaf_filename(argv[0]);

    /* Skip to next parameter */
    argc--;
    argv++;

    /* Parse vector size and help */
    while (argc) {
        if (!strcmp(argv[0], "-c") ||
            !strcmp(argv[0], "--count")) {

            /* Skip to next parameter */
            argc--;
            argv++;
            
            if (!argc) {
                usage();
            }

            /* Get the count */
            count = atoi(argv[0]);
        } else if (!strcmp(argv[0], "-i") ||
                   !strcmp(argv[0], "--interval")) {

            /* Skip to next parameter */
            argc--;
            argv++;
            
            if (!argc) {
                usage();
            }

            /* Get the interval */
            interval = atoi(argv[0]);
        } else if (!strcmp(argv[0], "-v") ||
                   !strcmp(argv[0], "--verbose")) {

            /* Skip to next parameter */
            argc--;
            argv++;
            
            if (!argc) {
                usage();
            }

            /* Get the interval */
            verbose = atoi(argv[0]);
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

    for (i = 0; i < count; i++) {
        bitd_gettimeofday(&time);

        if (verbose > 1) {
            printf("%u %6dus\n", 
                   (unsigned)time.tv_sec, 
                   (unsigned)time.tv_usec);
        }
        
        if (i > 0) {
            /* Check to make sure time goes forward */
            if (BITD_TIMEVAL_CMP(&time_last, &time, >)) {
                struct timeval delta;

                BITD_TIMEVAL_SUB(&delta, &time_last, &time);

                printf("Unexpected error: time goes back instead of forward\n");
                printf("Last: %u:%u, Now: %u:%u\n", 
                       (unsigned)time_last.tv_sec, 
                       (unsigned)time_last.tv_usec, 
                       (unsigned)time.tv_sec, 
                       (unsigned)time.tv_usec);
                printf("Delta: %u:%u\n", 
                       (unsigned)delta.tv_sec, 
                       (unsigned)delta.tv_usec);
                exit(0);
            }
        }

        /* Save the time */
        BITD_TIMEVAL_CPY(&time_last, &time);

        if (i != count - 1) {
            bitd_sleep(interval);
        }
    }

    bitd_sys_deinit();
    
    return 0;
}
