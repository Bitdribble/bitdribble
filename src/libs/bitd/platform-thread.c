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


/*****************************************************************************
 *                             MANIFEST CONSTANTS
 *****************************************************************************/



/*****************************************************************************
 *                                  MACROS 
 *****************************************************************************/



/*****************************************************************************
 *                                  TYPES
 *****************************************************************************/
/* The thread control block */
struct bitd_thread_s {
    char* name;                    /* the thread name */
    bitd_arch_thread arch_th;        /* the thread handle */
    void *thread_arg;              /* passed to thread entrypoint */
};


/*****************************************************************************
 *                           FUNCTION DECLARATION
 *****************************************************************************/
static bitd_thread create_thread_cb(const char* name, void *thread_arg);
static void destroy_thread_cb(bitd_thread th);


/*****************************************************************************
 *                                VARIABLES
 *****************************************************************************/
static bitd_event s_sleep_event = 0;


/*****************************************************************************
 *                          FUNCTION IMPLEMENTATION
 *****************************************************************************/



/*
 *============================================================================
 *                        bitd_sys_thread_init
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
bitd_boolean bitd_sys_thread_init(void) {
    bitd_boolean ret;
    
    ret = bitd_sys_arch_thread_init();
    if (!ret) {
        goto end;
    }

    /* Create the sleep event */
    s_sleep_event = bitd_event_create(0);
    if (!s_sleep_event) {
        ret = FALSE;
        goto end;
    }

end:
    if (!ret) {
        bitd_sys_thread_deinit();
    }

    return ret;
} 



/*
 *============================================================================
 *                        bitd_sys_thread_deinit
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
void bitd_sys_thread_deinit(void) {
    if (s_sleep_event) {
        bitd_event_destroy(s_sleep_event);
    }
    bitd_sys_arch_thread_deinit();
} 


/*
 *============================================================================
 *                        create_thread_cb
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
bitd_thread create_thread_cb(const char* name, void *thread_arg) {
    bitd_thread th;

    th = malloc(sizeof(struct bitd_thread_s));
    if (!th) {
        goto end;
        return NULL;
    }
    memset(th, 0, sizeof(struct bitd_thread_s));

    th->thread_arg = thread_arg;

    if (name) {
        th->name = strdup(name);
        if (!th->name) {
            destroy_thread_cb(th);
            th = NULL;
            goto end;
        }
    } else {
        th->name = NULL;
    }

end:
    return th;
} 


/*
 *============================================================================
 *                        destroy_thread_cb
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
void destroy_thread_cb(bitd_thread th) {
    if (th) {
        if (th->name) {
            free(th->name);
        }
        free(th);
    }
} 


/*
 *============================================================================
 *                        bitd_create_thread
 *============================================================================
 * Description:     Create and start the thread
 * Parameters:    
 *    name - thread name, if any
 *    entry - thread entrypoint
 *    priority, stack_nbytes - priority and stack size
 *    thread_arg - argument passed to the thread entrypoint
 * Returns:  
 *    the thread handle, if thread was successfully created
 */
bitd_thread bitd_create_thread(const char* name,
			       bitd_thread_entrypoint_t *entry, 
			       bitd_int32 priority, bitd_uint32 stack_nbytes,
			       void *thread_arg) {
    bitd_thread th;
    
    th = create_thread_cb(name, thread_arg);
    if (!th) {
        return NULL;
    }

    /* On OSX, round up the stack_nbytes to the closest 4096 multiple */
#if defined(__APPLE__) && defined(__MACH__)
    if (stack_nbytes) {
	stack_nbytes = 4096 * (1 + stack_nbytes/4096);
    }
#endif    

    /* Create and start the thread */
    th->arch_th = bitd_arch_create_thread((bitd_arch_thread_entrypoint_t *)entry,
                                        priority,
                                        stack_nbytes, 
					th);
    if (!th->arch_th) {
        destroy_thread_cb(th);
        return NULL;
    }

    return th;
}


/*
 *============================================================================
 *                        bitd_join_thread
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
void bitd_join_thread(bitd_thread th)
{
    if (th) {
        bitd_arch_join_thread(th->arch_th);
        destroy_thread_cb(th);
    }
}


/*
 *============================================================================
 *                        bitd_get_current_thread
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
bitd_thread bitd_get_current_thread() {
    bitd_arch_thread arch_thread;
    bitd_thread th;

    arch_thread = bitd_arch_get_current_thread();
    
    th = bitd_arch_get_platform_thread(arch_thread);

    return th;
}


/*
 *============================================================================
 *                        bitd_get_thread_name
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
const char* bitd_get_thread_name(bitd_thread th) {
    return th ? th->name : NULL;
}


/*
 *============================================================================
 *                        bitd_get_thread_arg
 *============================================================================
 * Description:     Get the thread argument
 * Parameters:    
 * Returns:  
 */
void* bitd_get_thread_arg(bitd_thread th) {
    return th ? th->thread_arg : NULL;
}


/*
 *============================================================================
 *                        bitd_sleep
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
void bitd_sleep(bitd_uint32 msec) {
    bitd_event_wait(s_sleep_event, msec);
} 


