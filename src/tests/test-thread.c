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
#define VERBOSE_DEF 0

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
static int g_verbose = VERBOSE_DEF;
bitd_mutex g_lock;
bitd_event g_event;

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

    bitd_mutex_lock(g_lock);
    if (g_verbose >= 1) {
	printf("Thread %s takes mutex\n", 
	       bitd_get_thread_name(bitd_get_current_thread()));
    }
    bitd_sleep(300);

    if (g_verbose >= 1) {
	printf("Thread %s leaves mutex\n", 
	       bitd_get_thread_name(bitd_get_current_thread()));
    }
    bitd_mutex_unlock(g_lock);

    if (bitd_event_wait(g_event, BITD_FOREVER)) {
	if (g_verbose >= 1) {
	    printf("Thread %s acquired event\n", 
		   bitd_get_thread_name(bitd_get_current_thread()));
	}
        bitd_sleep(300);
        bitd_event_set(g_event);
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
    printf("This program tests the thread API.\n\n");

    printf("Options:\n"
           "    -n thread_count\n"
           "            Number of threads. Default: %d.\n"
           "    -p\n"
           "            Use pollable events.\n"
           "    -v verbose_level, --verbose verbose_level\n"
           "            Set the verbosity level (default: %d).\n"
           "    -h, --help, -?\n"
           "            Show this help.\n",
           THREAD_COUNT_DEFAULT, VERBOSE_DEF);
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
    bitd_int32 thread_count = THREAD_COUNT_DEFAULT, i;
    bitd_thread *thread_handle;
    bitd_uint32 event_flags = 0;

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
            thread_count = atoi(argv[0]);

        } else if (!strcmp(argv[0], "-p")) {
            event_flags |= BITD_EVENT_FLAG_POLL;
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

    /* Allocate the mutex and the event */
    g_lock = bitd_mutex_create();
    g_event = bitd_event_create(event_flags);

    /* Allocate the array of thread handles */
    thread_handle = malloc(sizeof(*thread_handle) * thread_count);
    memset(thread_handle, 0, sizeof(*thread_handle) * thread_count);

    for (i = 0; i < thread_count; i++) {
        char thread_name[100];

        sprintf(thread_name, "thread %d", i);
        thread_handle[i] = bitd_create_thread(thread_name, 
                                            entry,
                                            0, 0, NULL);
        bitd_assert(thread_handle[i]);
    }

    bitd_sleep(5000);
    bitd_event_set(g_event);

    for (i = thread_count - 1; i >= 0; i--) {
        bitd_join_thread(thread_handle[i]);
    }
    
    free(thread_handle);

    /* Deallocate the mutex and the event */
    bitd_mutex_destroy(g_lock);
    bitd_event_destroy(g_event);

    bitd_sys_deinit();

    return 0;
}
