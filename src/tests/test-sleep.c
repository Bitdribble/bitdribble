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
static void usage(void) {
    printf("%s: Usage: test-sleep duration_msec.\n", g_prog_name);
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
int main(int argc, char **argv) {
    bitd_uint32 msec = 0;
    struct timeval tv_start, tv_end;

    /* Parse program name argument */
    g_prog_name = bitd_get_leaf_filename(argv[0]);

    /* Skip to next argument */
    argc--;
    argv++;

    if (argc != 1) {
        usage();
    } else if (!strcmp(argv[0], "-h") |
               !strcmp(argv[0], "--help") ||
               !strcmp(argv[0], "-?")) {
        usage();
    } else {
        msec = atoi(argv[0]);
    }

    bitd_sys_init();
    
    bitd_gettimeofday(&tv_start);
    bitd_sleep(msec);
    bitd_gettimeofday(&tv_end);
    
    BITD_TIMEVAL_SUB(&tv_end, &tv_end, &tv_start);
    printf("%s: Slept for %d secs %d usecs\n", 
           g_prog_name, (int)tv_end.tv_sec, (int)tv_end.tv_usec);

    bitd_sys_deinit();

    return 0;
} 


