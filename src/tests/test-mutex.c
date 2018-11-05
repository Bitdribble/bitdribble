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

#define THREAD_COUNT_DEFAULT 10
#define THREAD_SLEEP_DEF 300
#define VERBOSE_DEF 0

/*****************************************************************************
 *                                  TYPES
 *****************************************************************************/

struct thread_cb {
    bitd_thread th;
    int idx;
    bitd_boolean stop_p;
    bitd_boolean mutex_taken_p;
};

/*****************************************************************************
 *                           FUNCTION DECLARATION
 *****************************************************************************/



/*****************************************************************************
 *                                VARIABLES
 *****************************************************************************/
static char* g_prog_name = "";
static int g_verbose = VERBOSE_DEF;
bitd_mutex g_lock;
bitd_event g_event;
struct thread_cb *g_tcb;
int g_thread_count = THREAD_COUNT_DEFAULT;
bitd_uint32 g_mutex_sleep_msec;
bitd_uint32 g_thread_sleep_msec;

/*****************************************************************************
 *                          FUNCTION IMPLEMENTATION
 *****************************************************************************/



/*
 *============================================================================
 *                        entry
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
static void entry(void *thread_arg) {
    struct thread_cb *tcb = (struct thread_cb *)thread_arg;
    int i;

    if (g_verbose >= 2) {
	printf("Thread %d started\n", tcb->idx);
    }

    while (!tcb->stop_p) {
	if (g_verbose >= 2) {
	    printf("Thread %d sleeps %d msecs outside mutex\n", tcb->idx, g_mutex_sleep_msec);
	}
	bitd_sleep(g_mutex_sleep_msec);

	bitd_mutex_lock(g_lock);
	tcb->mutex_taken_p = TRUE;
	
	for (i = 0; i < g_thread_count; i++) {
	    if (i == tcb->idx) {
		continue;
	    }
	    if (g_tcb[i].mutex_taken_p) {
		printf("Thread %d should not have mutex!\n", i);
		exit(-1);
	    }
	}
	
	if (g_verbose >= 1) {
	    printf("Thread %d takes mutex\n", tcb->idx);
	}
	
	if (g_verbose >= 2) {
	    printf("Thread %d sleeps %d msecs inside mutex\n", tcb->idx, g_mutex_sleep_msec);
	}
	bitd_sleep(g_mutex_sleep_msec);
	
	if (g_verbose >= 1) {
	    printf("Thread %d leaves mutex\n", tcb->idx);
	}
	tcb->mutex_taken_p = FALSE;
	bitd_mutex_unlock(g_lock);
	
	if (g_verbose >= 2) {
	    printf("Thread %d stopping\n", tcb->idx);
	}
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
void usage() {

    printf("\nUsage: %s [OPTIONS ... ]\n\n", g_prog_name);
    printf("This program tests the thread and mutex API.\n\n");

    printf("Options:\n"
           "  -n thread_count\n"
           "      Number of threads. Default: %d.\n"
           "  -ms sleep_msec\n"
           "      How long to sleep inside mutex, in millisecs. Default: 0\n"
           "  -ts sleep_msec\n"
           "      How long to sleep waiting for threads, in millisecs. Default: %d\n"
           "  -v verbose_level, --verbose verbose_level\n"
           "      Set the verbosity level (default: %d).\n"
           "  -h, --help, -?\n"
           "      Show this help.\n",
           THREAD_COUNT_DEFAULT, THREAD_SLEEP_DEF, VERBOSE_DEF);
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
    int i;

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

            /* Get the count */
            g_thread_count = atoi(argv[0]);

	} else if (!strcmp(argv[0], "-ms")) {

            /* Skip to next parameter */
            argc--;
            argv++;
            
            if (!argc) {
                usage();
		exit(-1);
            }

            /* Get the count */
            g_mutex_sleep_msec = atoi(argv[0]);

	} else if (!strcmp(argv[0], "-ts")) {

            /* Skip to next parameter */
            argc--;
            argv++;
            
            if (!argc) {
                usage();
		exit(-1);
            }

            /* Get the count */
            g_thread_sleep_msec = atoi(argv[0]);

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

    /* Allocate the mutex */
    g_lock = bitd_mutex_create();

    /* Allocate the array of thread handles */
    g_tcb = calloc(1, sizeof(*g_tcb) * g_thread_count);

    for (i = 0; i < g_thread_count; i++) {
        char thread_name[100];

	g_tcb[i].idx = i;
        sprintf(thread_name, "Thread %d", i);
        g_tcb[i].th = bitd_create_thread(thread_name, 
				       entry,
				       0, 0, &g_tcb[i]);
        if (!g_tcb[i].th) {
	    printf("Could not create thread %d\n", i);
	    exit(-1);
	}
    }

    bitd_sleep(g_thread_sleep_msec);

    if (g_verbose >= 2) {
	printf("Stopping all threads\n");
    }
    
    /* Stop all threads */
    for (i = g_thread_count - 1; i >= 0; i--) {
        g_tcb[i].stop_p = TRUE;
    }

    if (g_verbose >= 2) {
	printf("Joining all threads\n");
    }

    /* Join all threads - this frees up all thread resources */
    for (i = g_thread_count - 1; i >= 0; i--) {
        bitd_join_thread(g_tcb[i].th);
    }
    
    free(g_tcb);

    /* Deallocate the mutex and the event */
    bitd_mutex_destroy(g_lock);
    bitd_event_destroy(g_event);

    bitd_sys_deinit();

    return 0;
}
