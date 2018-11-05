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
#include "bitd/timer-thread.h"
#include "bitd/timer-list.h"
#include "bitd/file.h"


/*****************************************************************************
 *                             MANIFEST CONSTANTS
 *****************************************************************************/
#define SLEEP_DEF 5000
#define VERBOSE_LEVEL_DEF 2



/*****************************************************************************
 *                                  MACROS 
 *****************************************************************************/


/*****************************************************************************
 *                                  TYPES
 *****************************************************************************/
typedef struct {
    tth_timer_t t;
    bitd_uint32 tmo;
} test_timer_t;



/*****************************************************************************
 *                           FUNCTION DECLARATION
 *****************************************************************************/



/*****************************************************************************
 *                                VARIABLES
 *****************************************************************************/
static char* g_prog_name = "";
static int g_timer_count, g_expirations;
static test_timer_t *g_timer;
static bitd_uint32 g_sleep = SLEEP_DEF;
static int g_verbose = VERBOSE_LEVEL_DEF;



/*****************************************************************************
 *                          FUNCTION IMPLEMENTATION
 *****************************************************************************/


/*
 *============================================================================
 *                        expiration
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
static void expiration(void *cookie) {
    test_timer_t *timer = (test_timer_t *)cookie;

    if (g_verbose >= 1) {
	printf("Timer expiration %d msecs\n", timer->tmo);
    }

    g_expirations++;
} 


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
    printf("This program tests the timer list API.\n\n");

    printf("Options:\n"
           "    -t timeout\n"
           "       Create a one-shot timer with the passed-in timeout.\n"
           "    -tp timeout\n"
           "       Create a periodic timer with the passed-in timeout.\n"
           "    -s sleep\n"
           "       How long to sleep after timers are created. Default: %u msecs.\n"
           "    -v verbose_level\n"
           "       Verbosity. Default: %d.\n"
           "    -h, --help, -?\n"
           "       Show this help.\n",
	   SLEEP_DEF, VERBOSE_LEVEL_DEF);

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
    bitd_uint32 tmo;
    int i;
    bitd_boolean periodic_p;

    bitd_sys_init();

    /* Parse program name argument */
    g_prog_name = bitd_get_leaf_filename(argv[0]);

    /* Skip to next parameter */
    argc--;
    argv++;

    /* Create the timer thread */
    tth_init();

    /* Create the global timer array */
    g_timer = malloc((argc+1)*sizeof(test_timer_t));
    memset(g_timer, 0, (argc+1)*sizeof(test_timer_t));    
    g_timer_count = 0;

    /* Parse vector size and help */
    while (argc) {
        if (!strcmp(argv[0], "-t") ||
	    !strcmp(argv[0], "-tp")) {

	    if (!strcmp(argv[0], "-tp")) {
		periodic_p = TRUE;
	    } else {
		periodic_p = FALSE;
	    }

            /* Skip to next parameter */
            argc--;
            argv++;
            
            if (!argc) {
                usage();
            }

            /* Get the timeout */
            tmo = atoi(argv[0]);
            
            g_timer[g_timer_count].tmo = tmo;
            g_timer[g_timer_count].t = 
		tth_timer_set_msec(tmo, &expiration, &g_timer[g_timer_count], 
				   periodic_p);
	    
            g_timer_count++;

	} else if (!strcmp(argv[0], "-s")) {
            /* Skip to next parameter */
            argc--;
            argv++;
            
            if (!argc) {
                usage();
		exit(-1);
            }

	    g_sleep = atoi(argv[0]);

	} else if (!strcmp(argv[0], "-v")) {
            /* Skip to next parameter */
            argc--;
            argv++;
            
            if (!argc) {
                usage();
		exit(-1);
            }

	    g_verbose = atoi(argv[0]);

        } else if (!strcmp(argv[0], "-h") ||
                   !strcmp(argv[0], "--help") ||
                   !strcmp(argv[0], "-?")) {
            usage();
        } else {
            fprintf(stderr,
		    "%s: Skipping invalid parameter %s\n", g_prog_name, argv[0]);
        }

        /* Skip to next argument */
        argc--;
        argv++;        
    }

    bitd_sleep(g_sleep);

    /* Destroy the timer list */
    for (i = 0; i < g_timer_count; i++) {
	tth_timer_destroy(g_timer[i].t);
    }

    free(g_timer);

    /* Destroy the timer thread */
    tth_deinit();

    bitd_sys_deinit();

    return 0;
}

