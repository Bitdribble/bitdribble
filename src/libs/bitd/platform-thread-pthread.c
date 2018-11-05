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

#include "pthread.h"
#define MIN_STACK_SIZE PTHREAD_STACK_MIN


/*****************************************************************************
 *                             MANIFEST CONSTANTS
 *****************************************************************************/



/*****************************************************************************
 *                                  MACROS 
 *****************************************************************************/
#define bitd_printf if (0) printf

/* #define THREAD_DEBUG */ /* Enable for extra debugging */

/*****************************************************************************
 *                                  TYPES
 *****************************************************************************/
/* The thread control block */
struct bitd_arch_thread_s {
    bitd_arch_thread_entrypoint_t *entry; /* the thread entrypoint */
    bitd_thread th;                  /* the platform thread handle */
    pthread_t h;                   /* the thread handle */
};

/* The mutex control block */
struct bitd_arch_mutex_s {
    pthread_mutex_t m;
    pthread_t owner;
    bitd_uint32 count;
};

/* The event control block */
struct bitd_arch_event_s {
    pthread_cond_t cond;
    pthread_mutex_t lock;
    bitd_boolean is_set;
};


/*****************************************************************************
 *                           FUNCTION DECLARATION
 *****************************************************************************/



/*****************************************************************************
 *                                VARIABLES
 *****************************************************************************/
static pthread_key_t local_thread_key;
static bitd_boolean local_thread_key_p = FALSE;


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
static void* entrypoint(void* parameter) {
  bitd_arch_thread ath = (bitd_arch_thread)parameter;
  bitd_thread th = ath->th;
  void *thread_arg = bitd_get_thread_arg(th);

  pthread_setspecific(local_thread_key, ath);
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
static bitd_arch_thread create_thread_cb(bitd_thread_entrypoint_t *entry,
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
 *    th - the thread handle
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
    if (!local_thread_key_p) {
        pthread_key_create(&local_thread_key, NULL);
        local_thread_key_p = TRUE;
    }
    
    /* Create and start the thread. Result is zero if success. */
    {
        int result = 0;
        pthread_attr_t attrs;
        
        pthread_attr_init(&attrs);
        result = pthread_attr_setstacksize(&attrs, stack_nbytes);
        if (result == 0) {
            result = pthread_create(&ath->h, &attrs, entrypoint, ath);
	    if (result) {
		pthread_attr_destroy(&attrs);
		destroy_thread_cb(ath);
		return NULL;
	    }
        }
        pthread_attr_destroy(&attrs);
    }

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
        pthread_join(ath->h, 0);
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
    bitd_arch_thread ath;

    if (!local_thread_key_p) {
        pthread_key_create(&local_thread_key, NULL);
        local_thread_key_p = TRUE;
    }

    ath = (bitd_arch_thread )pthread_getspecific(local_thread_key);
    
    return ath;
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
        return NULL;
    }

    m->owner = 0;
    m->count = 0;

    if (pthread_mutex_init(&m->m, NULL)) {
        free(m);
        return NULL;
    }

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
        pthread_mutex_destroy(&m->m);
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
    pthread_t self = pthread_self();

    if (!pthread_equal(self, m->owner)) {
	pthread_mutex_lock(&m->m);
	bitd_assert(!m->owner);
	bitd_assert(!m->count);
	m->owner = self;
    }
    m->count++;

    bitd_printf("Thread %llu mutex %p taken, owner %llu, count %u\n",
	      (bitd_uint64)self, m, (bitd_uint64)m->owner, m->count);
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
    pthread_t self = pthread_self();

#ifdef THREAD_DEBUG
    if (!pthread_equal(self, m->owner)) {
	/* Don't unlock someone else's mutex */
	bitd_assert(0);
        return;
    }
#endif

    if (!m->count) {
	bitd_assert(0);
    }
    
    bitd_printf("Thread %llu mutex %p released, owner %llu, count %u\n",
	      (bitd_uint64)self, m, (bitd_uint64)m->owner, m->count);

    if (--m->count == 0) {
        m->owner = 0;
        pthread_mutex_unlock(&m->m);
    }
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
        return NULL;
    }

    if (pthread_cond_init(&e->cond, NULL)) {
        free(e);
        return NULL;
    }

    if (pthread_mutex_init(&e->lock, NULL)) {
        pthread_cond_destroy(&e->cond);
        free(e);
        return NULL;
    }
    
    e->is_set = FALSE;

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
        pthread_cond_destroy(&e->cond);
        pthread_mutex_destroy(&e->lock);
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
    pthread_mutex_lock(&e->lock);
    e->is_set = TRUE;
    pthread_cond_signal(&e->cond);
    pthread_mutex_unlock(&e->lock);
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
    pthread_mutex_lock(&e->lock);
    e->is_set = 0;
    pthread_mutex_unlock(&e->lock);
} 



/*
 *============================================================================
 *                        bitd_arch_event_wait
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
bitd_boolean bitd_arch_event_wait(bitd_arch_event e, bitd_uint32 tmo) {
    bitd_boolean result = TRUE;
    struct timeval start_time, timeout;
    struct timespec end_time;
    
    if (tmo != BITD_FOREVER && tmo > 0) {
        bitd_gettimeofday(&start_time);
        BITD_MILLI_TO_TIMEVAL(&timeout, tmo);
        BITD_TIMEVAL_ADD(&timeout, &timeout, &start_time);
        end_time.tv_sec = timeout.tv_sec;
        end_time.tv_nsec = timeout.tv_usec * 1000;
    }

    pthread_mutex_lock(&e->lock);

    if (tmo) {
        while (!e->is_set) {
            if (tmo == BITD_FOREVER) {
                pthread_cond_wait(&e->cond, &e->lock);
            } else {
                if (pthread_cond_timedwait(&e->cond, &e->lock, &end_time)) {
                    break;
                }
            }
        }
    }

    result = e->is_set;
    e->is_set = FALSE;

    pthread_mutex_unlock(&e->lock);

    return result;
}
