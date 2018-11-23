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

#define THREAD_COUNT_DEFAULT 10
#define MSG_HDR_SIZE 24 /* Should be hard coded to sizeof(struct bitd_msg_s) */
#define MSG_SIZE 10 /* Can be arbitrary non-negative number */

/*****************************************************************************
 *                                  TYPES
 *****************************************************************************/
struct thread_cb {
    bitd_thread th;
    int n_msgs_sent;
    int n_msgs_recv;
    int n_msgs_quota_sent;
    int n_msgs_quota_recv;
};


/*****************************************************************************
 *                           FUNCTION DECLARATION
 *****************************************************************************/



/*****************************************************************************
 *                                VARIABLES
 *****************************************************************************/
static char* g_prog_name = "";
bitd_queue g_queue, g_quota_queue;
bitd_boolean g_stopping = FALSE;
bitd_int32 g_verbose;
struct thread_cb *g_tcb = NULL;

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
    bitd_msg m;
    bitd_uint32 thread_idx = (bitd_uint32)(long long)thread_arg;
    bitd_uint32 start_time;

    m = bitd_msg_alloc(thread_idx, MSG_SIZE);
    if (g_verbose > 1) {
        printf("Thread %u sends quota message\n", thread_idx);
    }
    
    start_time = bitd_get_time_msec();
    if (bitd_msg_send_w_tmo(m, g_quota_queue, 3000) != bitd_msgerr_ok) {
        if (g_verbose > 1) {
            printf("Thread %u quota message timeout after %u msec\n", 
                   thread_idx, bitd_get_time_msec() - start_time);      
        }
        bitd_msg_free(m);
    } else {
	g_tcb[thread_idx].n_msgs_quota_sent++;
        if (g_verbose > 1) {
            printf("Thread %u quota message %d sent after %u msec\n", 
                   thread_idx, 
		   g_tcb[thread_idx].n_msgs_quota_sent,
		   bitd_get_time_msec() - start_time);        
        }
    }

    while (!g_stopping) {
        m = bitd_msg_alloc(thread_idx, MSG_SIZE);
        if (g_verbose > 1) {
            printf("Thread %u sends message %d\n", thread_idx, 
		   g_tcb[thread_idx].n_msgs_sent+1);
        }
        
        if (bitd_msg_send_w_tmo(m, g_queue, 3000) != bitd_msgerr_ok) {
            if (g_verbose > 1) {
                printf("Thread %u message timeout\n", thread_idx);            
            }
            bitd_msg_free(m);
        } else {
	    g_tcb[thread_idx].n_msgs_sent++;
	}

        bitd_sleep(300);
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
    printf("This program tests the thred API.\n\n");

    printf("Options:\n"
           "    -n thread_count\n"
           "            Number of threads. Default: %d.\n"
           "    -p\n"
           "            Use pollable events.\n"
           "    -v verbose_level, --verbose verbose_level\n"
           "            Set the verbosity level (default: 2).\n"
           "    -h, --help, -?\n"
           "            Show this help.\n",
           THREAD_COUNT_DEFAULT);
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
    bitd_uint32 event_flags = 0;
    bitd_uint32* opcode_all;

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

    /* Allocate the main queue */
    g_queue = bitd_queue_create("main queue", 0, 0);
    g_quota_queue = bitd_queue_create("quota queue", 0, 
				      3 * (MSG_HDR_SIZE + MSG_SIZE));

    /* Allocate the array of thread control blocks */
    g_tcb = calloc(thread_count, sizeof(*g_tcb));

    for (i = 0; i < thread_count; i++) {
        char thread_name[100];

        sprintf(thread_name, "thread %d", i);
        g_tcb[i].th = bitd_create_thread(thread_name, 
					 entry,
					 0, 0, (void *)(long long)i);
        bitd_assert(g_tcb[i].th);
    }

    opcode_all = malloc(thread_count * sizeof(*opcode_all));
    for (i = 0; i < thread_count; i++) {
        opcode_all[i] = i;
    }    

    /* Receive one message from every thread on the quota queue */
    for (i = 0; i < thread_count; i++) {
        bitd_msg m;
	bitd_int32 thread_idx;

        m = bitd_msg_receive_selective(g_quota_queue, 
				       thread_count, opcode_all, 
				       BITD_FOREVER);
        bitd_assert(m);

	thread_idx = bitd_msg_get_opcode(m);
	g_tcb[thread_idx].n_msgs_quota_recv++;

        if (g_verbose > 1) {
            printf("Got quota message %u from thread %u\n", 
		   g_tcb[thread_idx].n_msgs_quota_recv, 
		   thread_idx);
        }

        bitd_msg_free(m);
    }

    if (g_verbose) {
	printf("Got all quota messages\n");
    }

    for (i = 0; i < 100; i++) {
        bitd_msg m;
        bitd_uint32 start_time = bitd_get_time_msec();
	bitd_int32 thread_idx;

        m = bitd_msg_receive_selective(g_queue, 0, NULL, BITD_FOREVER);
        if (m) {
	    thread_idx = bitd_msg_get_opcode(m);
	    g_tcb[thread_idx].n_msgs_recv++;

	    if (g_verbose > 1) {
		printf("Got message %u from thread %u\n",
		       g_tcb[thread_idx].n_msgs_recv, thread_idx);
	    }
            bitd_msg_free(m);
        } else {
	    if (g_verbose > 1) {
		printf("Message timeout after %u msec\n", 
		       bitd_get_time_msec() - start_time);
	    }
        }
    }

    if (g_verbose) {
	printf("Got all messages\n");
    }

    g_stopping = TRUE;

    for (i = thread_count - 1; i >= 0; i--) {
        bitd_join_thread(g_tcb[i].th);
    }
    
    free(opcode_all);
    free(g_tcb);

    /* Deallocate the main queue */
    bitd_queue_destroy(g_queue);
    bitd_queue_destroy(g_quota_queue);

    bitd_sys_deinit();

    return 0;
}
