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

#include "process.h"
#define MIN_STACK_SIZE 1024

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
struct bitd_arch_thread_s {
    bitd_arch_thread_entrypoint_t *entry; /* the thread entrypoint */
    bitd_thread th;                  /* the platform thread handle */
    HANDLE h;                      /* the thread handle */
    DWORD thread_id;               /* the thread id */
};

/* The mutex control block */
struct bitd_arch_mutex_s {
    HANDLE m;
};

/* The event control block */
struct bitd_arch_event_s {
    HANDLE e;
};

/*****************************************************************************
 *                           FUNCTION DECLARATION
 *****************************************************************************/



/*****************************************************************************
 *                                VARIABLES
 *****************************************************************************/
static DWORD local_thread_key = 0;


/*****************************************************************************
 *                          FUNCTION IMPLEMENTATION
 *****************************************************************************/



/*
 *============================================================================
 *                        bitd_sys_arch_thread_init
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
bitd_boolean bitd_sys_arch_thread_init(void) {
    return TRUE;
} 



/*
 *============================================================================
 *                        bitd_sys_arch_thread_deinit
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
void bitd_sys_arch_thread_deinit(void) {
} 



/*
 *============================================================================
 *                        entrypoint
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
static DWORD WINAPI entrypoint(LPVOID parameter) {
    bitd_arch_thread ath = (bitd_arch_thread)parameter;
    bitd_thread th = ath->th;
    void *thread_arg = bitd_get_thread_arg(th);

    TlsSetValue(local_thread_key, ath);
    ath->entry(thread_arg);
    
    return 0;
}


/*
 *============================================================================
 *                        create_thread_cb
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
static bitd_arch_thread create_thread_cb(bitd_arch_thread_entrypoint_t *entry,
				       bitd_thread th) {
    bitd_arch_thread ath;

    ath = malloc(sizeof(*ath));
    if (!ath) {
        return NULL;
    }
    memset(ath, 0, sizeof(*ath));

    ath->entry = entry;
    ath->th = th;

    return ath;
} 



/*
 *============================================================================
 *                        destroy_thread_cb
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
static void destroy_thread_cb(bitd_arch_thread ath) {
    if (ath) {
        free(ath);
    }
} 


/*
 *============================================================================
 *                        bitd_arch_create_thread
 *============================================================================
 * Description:     Create and start the thread
 * Parameters:    
 *    entry - thread entrypoint
 *    priority, stack_nbytes - priority and stack size
 *    th - platform thread handle
 * Returns:  
 *    the thread handle, if thread was successfully created
 */
bitd_arch_thread bitd_arch_create_thread(bitd_arch_thread_entrypoint_t *entry, 
					 bitd_int32 priority, 
					 bitd_uint32 stack_nbytes,
					 bitd_thread th) {
    bitd_arch_thread ath;
    
    if (stack_nbytes < MIN_STACK_SIZE) {
        stack_nbytes = MIN_STACK_SIZE;
    }

    ath = create_thread_cb(entry, th);
    if (!ath) {
        return NULL;
    }

    /* Create and start the thread */
    if (!local_thread_key) {
        local_thread_key = TlsAlloc();
    }
    ath->h = CreateThread(NULL, stack_nbytes, &entrypoint, ath, 0, 
			  &ath->thread_id);
    if (!ath->h) {
        destroy_thread_cb(ath);
        return NULL;
    }

    return ath;
}



/*
 *============================================================================
 *                        bitd_arch_join_thread
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
void bitd_arch_join_thread(bitd_arch_thread ath) {
    if (ath) {
        WaitForSingleObject(ath->h, INFINITE);
        CloseHandle(ath->h);

        destroy_thread_cb(ath);
    }
}



/*
 *============================================================================
 *                        bitd_arch_get_current_thread
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
bitd_arch_thread bitd_arch_get_current_thread() {
    if (!local_thread_key) {
        local_thread_key = TlsAlloc();
    }

    return((bitd_arch_thread )TlsGetValue(local_thread_key));
}

/*
 *============================================================================
 *                        bitd_arch_get_platform_thread
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
bitd_thread bitd_arch_get_platform_thread(bitd_arch_thread ath) {
    return ath ? ath->th : NULL;
}


/*
 *============================================================================
 *                        bitd_arch_mutex_create
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
bitd_arch_mutex bitd_arch_mutex_create(void) {
    bitd_arch_mutex m;

    m = malloc(sizeof(*m));
    if (!m) {
        goto end;
    }
    m->m = CreateMutex(NULL, FALSE, NULL);
    if (!m->m) {
        free(m);
        m = NULL;
    }
end:
    return m;
} 


/*
 *============================================================================
 *                        bitd_arch_mutex_destroy
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
void bitd_arch_mutex_destroy(bitd_arch_mutex m) {
    if (m) {
        CloseHandle(m->m);
        free(m);
    }
} 



/*
 *============================================================================
 *                        bitd_arch_mutex_lock
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
void bitd_arch_mutex_lock(bitd_arch_mutex m) {
    WaitForSingleObject(m->m, INFINITE);    
} 


/*
 *============================================================================
 *                        bitd_arch_mutex_unlock
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
void bitd_arch_mutex_unlock(bitd_arch_mutex m) {
    ReleaseMutex(m->m);
} 


/*
 *============================================================================
 *                        bitd_arch_event_create
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
bitd_arch_event bitd_arch_event_create(void) {
    bitd_arch_event e;
    
    e = malloc(sizeof(*e));
    if (!e) {
        goto end;
    }
    e->e = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (!e->e) {
        free(e);
        e = NULL;
    }
end:
    return e;    
} 

/*
 *============================================================================
 *                        bitd_arch_event_destroy
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
void bitd_arch_event_destroy(bitd_arch_event e) {
    if (e) {
        CloseHandle(e->e);
        free(e);
    }
} 


/*
 *============================================================================
 *                        bitd_arch_event_set
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
void bitd_arch_event_set(bitd_arch_event e) {
    SetEvent(e->e);
} 



/*
 *============================================================================
 *                        bitd_arch_event_clear
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
void bitd_arch_event_clear(bitd_arch_event e) {
    ResetEvent(e->e);
} 



/*
 *============================================================================
 *                        bitd_arch_event_wait
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
bitd_boolean bitd_arch_event_wait(bitd_arch_event e, bitd_uint32 timeout) {
    if (timeout == BITD_FOREVER) {
        WaitForSingleObject(e->e, INFINITE);
        return TRUE;
    }
     
    if (WaitForSingleObject(e->e, timeout) == WAIT_OBJECT_0) {
        return TRUE;
    }
    return FALSE;
} 

