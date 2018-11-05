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
#include "bitd/timer-list.h"
#include "bitd/file.h"
#include "bitd/lambda.h"


/*****************************************************************************
 *                             MANIFEST CONSTANTS
 *****************************************************************************/
#define VERBOSE_LEVEL_DEF 2



/*****************************************************************************
 *                                  MACROS 
 *****************************************************************************/


/*****************************************************************************
 *                                  TYPES
 *****************************************************************************/
typedef struct {
    bitd_timer t;
    bitd_uint32 tmo;
    long count;
    bitd_boolean lambda_timer_p; /* If TRUE, timer is set from lambda pool */
} test_timer_t;


/*****************************************************************************
 *                           FUNCTION DECLARATION
 *****************************************************************************/
static void expiration(bitd_timer t, void *cookie);


/*****************************************************************************
 *                                VARIABLES
 *****************************************************************************/
static char* g_prog_name = "";
static int g_verbose = VERBOSE_LEVEL_DEF;
static bitd_timer_list g_timer_list;
static bitd_lambda_handle g_lambda;
static long g_timer_count;


/*****************************************************************************
 *                          FUNCTION IMPLEMENTATION
 *****************************************************************************/

/*
 *============================================================================
 *                        add_timer
 *============================================================================
 * Description:
 * Parameters:
 * Returns:
 */
static void add_timer(void *arg, bitd_boolean *stopping_p) {
    test_timer_t *timer = (test_timer_t *)arg;

    /* Add timer to the list */
    bitd_timer_list_add_msec(g_timer_list, timer->t, timer->tmo,
			   &expiration, timer);

    /* Nothing to free since the timer object will be freed when
       the timer expires (or, rather, expires for the last time) */
}


/*
 *============================================================================
 *                        expiration
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
static void expiration(bitd_timer t, void *cookie) {
    test_timer_t *timer = (test_timer_t *)cookie;
    bitd_boolean stopping_p = FALSE;

    bitd_assert(timer->t == t);
    
    /* Decrement the count */
    --timer->count;

    if (g_verbose >= 1) {
	if (timer->count > 0) {
	    printf("Timer expiration %d msecs, remaining count %ld\n", 
		   timer->tmo, timer->count);
	} else {
	    printf("Timer expiration %d msecs\n", timer->tmo);
	}
    }
    
    if (timer->count > 0) {
	/* Reinstall the timer */
	add_timer(timer, &stopping_p);
    } else {
	/* Release the timer */
	bitd_timer_destroy(t);
	free(timer);
    }
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
           "  -t timeout\n"
           "     Create a timer with the passed-in timeout\n"
           "  -tc timeout count\n"
           "     Create a periodic timer with the passed-in timeout. The timer\n"
	   "     will periodically trigger for 'count' times.\n"
           "  -thc timeout count\n"
           "     Same as the above, but timers are installed from different threads.\n"
           "  -v verbose_level\n"
           "     Verbosity. Default: %d.\n"
           "  -h, --help, -?\n"
           "     Show this help.\n",
	   VERBOSE_LEVEL_DEF);

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
    test_timer_t *timer;
    char *arg0;
    bitd_uint32 tmo;
    long count;
    bitd_boolean stopping_p = FALSE;

    bitd_sys_init();

    /* Parse program name argument */
    g_prog_name = bitd_get_leaf_filename(argv[0]);

    /* Skip to next parameter */
    argc--;
    argv++;

    /* Create the timer list */
    g_timer_list = bitd_timer_list_create();
    bitd_assert(g_timer_list);

    /* Create the lambda pool */
    g_lambda = bitd_lambda_init("lambda");

    /* Parse vector size and help */
    while (argc) {
        if (!strcmp(argv[0], "-t") ||
	    !strcmp(argv[0], "-tc") ||
	    !strcmp(argv[0], "-thc")) {
            bitd_timer t;

	    /* Remember this argument */
	    arg0 = argv[0];

            /* Skip to next parameter */
            argc--;
            argv++;
            
            if (!argc) {
                usage();
            }

            /* Get the timeout */
            tmo = atoi(argv[0]);
            
	    /* Assume single expiration, until more args are parsed  */
	    count = 1; 

	    if (!strcmp(arg0, "-tc") ||
		!strcmp(arg0, "-thc")) {
		/* Parse the count parameter */
		argc--;
		argv++;
		
		if (!argc) {
		    usage();
		}
		
		/* Get the count */
		count = atoi(argv[0]);
	    }

            /* Create a timer */
            t = bitd_timer_create();
            bitd_assert(t);
            
	    /* Create the timer control block */
	    timer = calloc(1, sizeof(*timer));
            timer->tmo = tmo;
            timer->t = t;
	    timer->count = count;

	    if (!strcmp(arg0, "-thc")) {
		timer->lambda_timer_p = TRUE;
	    }

	    if (!timer->lambda_timer_p) {
		/* Add timer to the list directly */
		add_timer(timer, &stopping_p);
	    } else {
		bitd_boolean ret;

		/* Add the timer from the lambda pool */
		ret = bitd_lambda_exec_task(g_lambda, &add_timer, timer);
		if (!ret) {
		    fprintf(stderr,
			    "%s: Failed to add timer through lambda pool\n", g_prog_name);
		    exit(-1);
		}
	    }

	    g_timer_count++;

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

    if (g_timer_count) {
	/* Wait a bit for the timers to get installed */
	while ((tmo = bitd_timer_list_get_timeout_msec(g_timer_list)) == BITD_FOREVER) {
	    bitd_sleep(100);
	}
    }

    /* Keep ticking the expiration list */
    while ((tmo = bitd_timer_list_get_timeout_msec(g_timer_list)) != BITD_FOREVER) {
	if (g_verbose >= 3) {
	    printf("Sleeping %u msecs\n", tmo);
	}
        bitd_sleep(tmo);
        bitd_timer_list_tick(g_timer_list);
    }

    /* Destroy the timer list when we're out of timers */
    bitd_timer_list_destroy(g_timer_list);

    /* Destroy the lambda pool */
    bitd_lambda_deinit(g_lambda);

    bitd_sys_deinit();

    return 0;
}

