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
#include "bitd/lambda.h"
#include "bitd/file.h"


/*****************************************************************************
 *                             MANIFEST CONSTANTS
 *****************************************************************************/



/*****************************************************************************
 *                                  MACROS 
 *****************************************************************************/


#define THREAD_MAX_DEF 3
#define THREAD_IDLE_TMO_MSEC_DEF 300000 /* 5 minutes */
#define TASK_MAX_DEF 0
#define TASK_COUNT_DEF 30
#define TASK_SLEEP_MSEC_DEF 500
#define TASK_BETWEEN_SLEEP_MSEC_DEF 0
#define SLEEP_MSEC_DEF 1
#define VERBOSE_LEVEL_DEF 2

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
int g_thread_max = THREAD_MAX_DEF;
int g_thread_idle_tmo_msec = THREAD_IDLE_TMO_MSEC_DEF;
int g_task_max = TASK_MAX_DEF;
int g_task_count = TASK_COUNT_DEF;
int g_task_sleep_msec = TASK_SLEEP_MSEC_DEF;
int g_task_between_sleep_msec = TASK_BETWEEN_SLEEP_MSEC_DEF;
int g_sleep_msec = SLEEP_MSEC_DEF;
int g_verbose = VERBOSE_LEVEL_DEF;


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
    printf("This program tests the worker thread pool API.\n\n");

    printf("Options:\n"
           "    -n|--thread-max thread_max\n"
           "         The worker thread pool size. Default: %d.\n"
           "    --idle-tmo thread_idle_tmo_msecs\n"
           "         How many seconds to wait before destroying an idle\n"
	   "         thread. Default: %d msecs.\n"
           "    -tm|--task-max task_max\n"
           "         How many tasks to allow.\n"
	   "         A zero task_max means there is no max. Default: %d.\n"
           "    -tc|--task-count task_count\n"
           "         How many tasks to execute. Default: %d.\n"
	   "    -ts|--task-sleep sleep_msecs\n"
	   "         How long will a task sleep. Default: %d,\n"
	   "    -tbs|--task-between-sleep sleep_msecs\n"
	   "         How long to sleep between tasks. Default: %d,\n"
	   "    -s|--sleep sleep_msecs\n"
	   "         How long will unit tester sleep at the end. Default: %d,\n"
           "    -v verbose_level\n"
           "         Verbosity. Default: %d.\n"
           "    -h, --help, -?\n"
           "         Show this help.\n",
	   THREAD_MAX_DEF, THREAD_IDLE_TMO_MSEC_DEF,
	   TASK_MAX_DEF, TASK_COUNT_DEF,
	   TASK_SLEEP_MSEC_DEF,
	   TASK_BETWEEN_SLEEP_MSEC_DEF,
	   SLEEP_MSEC_DEF,
	   VERBOSE_LEVEL_DEF);
} 



/*
 *============================================================================
 *                        exec_func
 *============================================================================
 * Description:
 * Parameters:
 * Returns:
 */
void exec_func(void *arg, bitd_boolean *stopping_p) {
    
    if (g_verbose >= 2) {
	printf("Hello from %s (%s)\n", 
	       bitd_get_thread_name(bitd_get_current_thread()), (char *)arg);
    }
    
    if (!*stopping_p) {
	bitd_sleep(g_task_sleep_msec);
    }

    free(arg);
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
    bitd_lambda_handle lambda;
    bitd_boolean ret;
    bitd_uint32 i;

    bitd_sys_init();

    /* Parse program name argument */
    g_prog_name = bitd_get_leaf_filename(argv[0]);

    /* Skip to next parameter */
    argc--;
    argv++;

    /* Parse vector size and help */
    while (argc) {
        if (!strcmp(argv[0], "-n") ||
	    !strcmp(argv[0], "--thred-max")) {
            /* Skip to next parameter */
            argc--;
            argv++;
            
            if (!argc) {
                usage();
		exit(-1);
            }

	    g_thread_max = atoi(argv[0]);

	} else if (!strcmp(argv[0], "--idle-tmo")) {
            /* Skip to next parameter */
            argc--;
            argv++;
            
            if (!argc) {
                usage();
		exit(-1);
            }

	    g_thread_idle_tmo_msec = atoi(argv[0]);

	} else if (!strcmp(argv[0], "-tm") ||
		   !strcmp(argv[0], "--task-max")) {
            /* Skip to next parameter */
            argc--;
            argv++;
            
            if (!argc) {
                usage();
		exit(-1);
            }

	    g_task_max = atoi(argv[0]);

	} else if (!strcmp(argv[0], "-tc") ||
		   !strcmp(argv[0], "--task-count")) {
            /* Skip to next parameter */
            argc--;
            argv++;
            
            if (!argc) {
                usage();
		exit(-1);
            }

	    g_task_count = atoi(argv[0]);

	} else if (!strcmp(argv[0], "-ts") ||
		   !strcmp(argv[0], "--task-sleep")) {
            /* Skip to next parameter */
            argc--;
            argv++;
            
            if (!argc) {
                usage();
		exit(-1);
            }

	    g_task_sleep_msec = atoi(argv[0]);

	} else if (!strcmp(argv[0], "-tbs") ||
		   !strcmp(argv[0], "--task-between-sleep")) {
            /* Skip to next parameter */
            argc--;
            argv++;
            
            if (!argc) {
                usage();
		exit(-1);
            }

	    g_task_between_sleep_msec = atoi(argv[0]);

	} else if (!strcmp(argv[0], "-s") ||
		   !strcmp(argv[0], "--sleep")) {
            /* Skip to next parameter */
            argc--;
            argv++;
            
            if (!argc) {
                usage();
		exit(-1);
            }

	    g_sleep_msec = atoi(argv[0]);

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
	    exit(0);
        } else {
            printf("%s: Skipping invalid parameter %s\n", 
                   g_prog_name, argv[0]);
        }

        /* Skip to next argument */
        argc--;
        argv++;        
    }

    if (g_task_max && (g_task_count > g_task_max)) {
	if (g_verbose >= 1) {
	    printf("%s: Warning: Task count %d larger than the task max %d\n", 
		   g_prog_name, g_task_count, g_task_max);
	}
    }

    if (g_verbose >= 1) {
	printf("%s: Initializing the worker thread pool\n", 
	       g_prog_name);
    }

    lambda = bitd_lambda_init("test_lambda");

    bitd_lambda_set_thread_max(lambda, g_thread_max);
    bitd_lambda_set_thread_idle_tmo(lambda, g_thread_idle_tmo_msec);
    bitd_lambda_set_task_max(lambda, g_task_max);

    for (i = 0; i < g_task_count; i++) {
        char *buf = malloc(256);

        sprintf(buf, "execution %u", i);

        ret = bitd_lambda_exec_task(lambda, exec_func, buf);
	if (!ret) {
	    if (g_verbose >= 1) {
		printf("%s: bitd_lambda_task_exec() returned FALSE\n", 
		       g_prog_name);
	    }
	    /* Free the argument */
	    free(buf);
	}

	bitd_sleep(g_task_between_sleep_msec);
    }

    bitd_sleep(g_sleep_msec);

    if (g_verbose >= 1) {
	printf("%s: Deinitializing the worker thread pool\n", 
	       g_prog_name);
    }

    bitd_lambda_deinit(lambda);

    if (g_verbose >= 1) {
	printf("%s: worker thread pool\n", 
	       g_prog_name);
    }

    bitd_sys_deinit();

    return 0;
}

