/*****************************************************************************
 *
 * Original Author: Andrei Radulescu-Banu
 * Creation Date:   
 * Description:
 *
 * Copyright (C) 2018 Andrei Radulescu-Banu.  All Rights Reserved.
 * Unauthorized reproduction, modification, distribution, transmission,
 * republication, display or performance are strictly prohibited.
 ****************************************************************************/

/*****************************************************************************
 *                                INCLUDE FILES
 *****************************************************************************/
#include "bitd/lambda.h"


/*****************************************************************************
 *                             MANIFEST CONSTANTS
 *****************************************************************************/



/*****************************************************************************
 *                                  MACROS
 *****************************************************************************/
#define dbg_printf if(0) printf

#define THREAD_MAX_DEF 100 /* Limited to 100 */
#define THREAD_IDLE_TMO_DEF 300000 /* 300 secs (5 minutes) */
#define TASK_MAX_DEF 0 /* Unlimited */

/*****************************************************************************
 *                                  TYPES
 *****************************************************************************/

/*
 * All threads are chained by thread_head/thread/tail/next/prev. 
 * Idle threads await event to wake up and perform tasks.
 * Exited threads await being joined to free up resources.
 */
typedef struct bitd_lambda_thread {
    /* List of all threads */
    struct bitd_lambda_thread *next;      
    struct bitd_lambda_thread *prev;
    /* Idle thread list */
    struct bitd_lambda_thread *idle_next; 
    struct bitd_lambda_thread *idle_prev; 
    /* Exited thread list */
    struct bitd_lambda_thread *exited_next; 
    struct bitd_lambda_thread *exited_prev;
    struct bitd_lambda *lambda;     /* The worker thread pool control block */
    char *name;             /* The thread name */   
    bitd_thread th;
    bitd_event ev;            /* Kicked when idle thread needs to perform task */
    bitd_uint32 last_active;  /* Timestamp of last task */
} bitd_lambda_thread;


typedef struct bitd_lambda_task {
    bitd_lambda_task_func_t *f;
    void *cookie;
    struct bitd_lambda_task *next;
} bitd_lambda_task;


typedef struct bitd_lambda {
    char *name; /* The thread pool name */
    struct bitd_lambda_thread *thread_head; /* List of all threads */
    struct bitd_lambda_thread *thread_tail;
    struct bitd_lambda_thread *thread_idle_head; /* List of idle threads */
    struct bitd_lambda_thread *thread_idle_tail;
    struct bitd_lambda_thread *thread_exited_head; /* List of exited threads */
    struct bitd_lambda_thread *thread_exited_tail;
    bitd_mutex m;
    int thread_count; /* How many threads are on thread list */
    int thread_idx;   /* Used to ensure new thread names are unique */
    int thread_max;   /* Max number of threads */
    bitd_uint32 thread_idle_tmo; /* Threads idle for longer will exit (msec) */
    int n_tasks;
    int task_max;
    bitd_lambda_task *task_start;
    bitd_lambda_task *task_end;
    bitd_boolean stopping_p;  /* TRUE if the thread pool is stopping */
} bitd_lambda;

#define THREAD_HEAD(lambda)				\
    ((struct bitd_lambda_thread *)&(lambda)->thread_head)
#define THREAD_IDLE_HEAD(lambda)				\
    ((struct bitd_lambda_thread *)((char *)(&(lambda)->thread_idle_head) -	\
			      offsetof(struct bitd_lambda_thread, idle_next)))
#define THREAD_EXITED_HEAD(lambda)				\
    ((struct bitd_lambda_thread *)((char *)(&(lambda)->thread_exited_head) -	\
			      offsetof(struct bitd_lambda_thread, exited_next)))

/*****************************************************************************
 *                           FUNCTION DECLARATION
 *****************************************************************************/



/*****************************************************************************
 *                                VARIABLES
 *****************************************************************************/



/*****************************************************************************
 *                          FUNCTION IMPLEMENTATION
 *****************************************************************************/


/*
 *============================================================================
 *                        lambda_entrypoint
 *============================================================================
 * Description:   Worker thread entrypoint
 * Parameters:
 * Returns:
 */
static void lambda_entrypoint(void *thread_arg) {
    bitd_lambda_thread *thread = (bitd_lambda_thread *)thread_arg;
    bitd_lambda_task *task;
    bitd_lambda *lambda;
    bitd_uint32 current_time, tmo;
    
    /* Get the worker thread pool handle */
    lambda = thread->lambda;

    while (!lambda->stopping_p) {
	/* Is there a task block queued up? */
	bitd_mutex_lock(lambda->m);
	task = lambda->task_start;
	if (task) {
	    lambda->task_start = task->next;
	    if (lambda->task_end == task) {
		lambda->task_end = NULL;
	    }
	    
	    lambda->n_tasks--;
	    bitd_assert(lambda->n_tasks >= 0);
	}
	bitd_mutex_unlock(lambda->m);
	
	if (task) {
	    /* Execute the task outside the mutex. This function may block. */
	    task->f(task->cookie, &lambda->stopping_p);
	    free(task);

	    /* Record the last active time */
	    thread->last_active = bitd_get_time_msec();

	    /* Go back to look for another task */
	    continue;
	}
            
	current_time = bitd_get_time_msec();
	bitd_mutex_lock(lambda->m);

	/* Has the thread been idle for too long? */
	if (current_time - thread->last_active >= lambda->thread_idle_tmo) {
	    /* Thread needs to exit. Place it on the exited list, and exit. */
	    thread->exited_prev = lambda->thread_exited_tail;
	    thread->exited_next = lambda->thread_exited_tail->exited_next;
	    thread->exited_prev->exited_next = thread;
	    thread->exited_next->exited_prev = thread;
	    bitd_mutex_unlock(lambda->m);
	    dbg_printf("%s inactive for %d msecs, exited\n",
		      thread->name, current_time - thread->last_active);
	    return;
	}
	
	/* Place the thread on the idle list */
	thread->idle_prev = lambda->thread_idle_tail;
	thread->idle_next = lambda->thread_idle_tail->idle_next;
	thread->idle_prev->idle_next = thread;
	thread->idle_next->idle_prev = thread;
        
        bitd_mutex_unlock(lambda->m);

	/* Check if we're stopping before waiting on event */
	if (lambda->stopping_p) {
	    break;
	}

	/* Wait on the event long enough to be past the thread_idle_tmo
	   if we're not woken up to handle a task */
	tmo = lambda->thread_idle_tmo - (current_time - thread->last_active);
	tmo = MAX(1, tmo);
	tmo = MIN(tmo, 0xefffffff); /* Wrap guard */
	
        bitd_event_wait(thread->ev, tmo);
        
        dbg_printf("%s woke up\n", 
                  bitd_get_thread_name(bitd_get_current_thread()));
    } /* while (!lambda->stoppin_p) */

    dbg_printf("%s exited\n", thread->name);
}


/*
 *============================================================================
 *                        lambda_create_thread
 *============================================================================
 * Description:     Create a worker thread. Worker threads are created
 *     dynamically as tasks get executed and need threads to execute them.
 * Parameters:    
 *     lambda - the worker thread pool
 * Returns:  
 */
struct bitd_lambda_thread *lambda_create_thread(bitd_lambda *lambda) {
    struct bitd_lambda_thread *thread;

    /* A thread_max of 0 means the thread count is unlimited. By default,
       we limit the thread count, but the user can set the thread_max 
       to zero, which effectively disables the thread_max */
    if (lambda->thread_max && (lambda->thread_count >= lambda->thread_max)) {
	return NULL;
    }
    
    thread = calloc(1, sizeof(*thread));
    thread->lambda = lambda;
    thread->name = malloc(strlen(lambda->name) + 256);

    bitd_mutex_lock(lambda->m);

    sprintf(thread->name, "%s-%u", lambda->name, lambda->thread_idx);
    
    /* Add to the thread list */
    thread->prev = lambda->thread_tail;
    thread->next = lambda->thread_tail->next;
    thread->prev->next = thread;
    thread->next->prev = thread;
    
    lambda->thread_count++;
    lambda->thread_idx++;

    /* Initialize the last active time */
    thread->last_active = bitd_get_time_msec();

    /* Create the event, then the thread */
    thread->ev = bitd_event_create(0);
    thread->th = 
	bitd_create_thread(thread->name,
			 &lambda_entrypoint,
			 BITD_DEFAULT_PRIORITY,
			 65536, /* Larger stack to avoid curl crash in yaml code. Stack size must be multiple of 4096 on OSX. */
			 thread);

    dbg_printf("%s created\n", thread->name);
    
    bitd_mutex_unlock(lambda->m);

    return thread;
} 


/*
 *============================================================================
 *                        lambda_join_thread
 *============================================================================
 * Description:     Wait for a thread to exit and free its resources
 * Parameters:    
 * Returns:  
 */
void lambda_join_thread(bitd_lambda_thread *thread) {
    dbg_printf("%s joined\n", thread->name);
    
    /* Wait for the thread to exit */
    bitd_join_thread(thread->th);

    /* Unchain the thread from the thread list */
    thread->next->prev = thread->prev;
    thread->prev->next = thread->next;

    if (thread->idle_next) {
	bitd_assert(thread->idle_prev);
	thread->idle_prev->idle_next = thread->idle_next;
	thread->idle_next->idle_prev = thread->idle_prev;
    }
    if (thread->exited_next) {
	bitd_assert(thread->exited_prev);
	thread->exited_prev->exited_next = thread->exited_next;
	thread->exited_next->exited_prev = thread->exited_prev;
    }

    thread->lambda->thread_count--;

    dbg_printf("%s freed\n", thread->name);

    /* Free all thread data */
    free(thread->name);
    bitd_event_destroy(thread->ev);

    /* Free the thread */
    free(thread);
} 


/*
 *============================================================================
 *                        lambda_join_all_exited_threads
 *============================================================================
 * Description:     Garbage collector for all exited threads
 *     lambda - the worker thread pool
 * Parameters:    
 * Returns:  
 */
void lambda_join_all_exited_threads(bitd_lambda_handle lambda) {

    bitd_mutex_lock(lambda->m);
    
    /* Walk the thread pool once, to stop each thread */
    while (lambda->thread_exited_head != THREAD_EXITED_HEAD(lambda)) {
	lambda_join_thread(lambda->thread_exited_head);
    }

    bitd_mutex_unlock(lambda->m);
} 



/*
 *============================================================================
 *                        lambda_wake_up_all_threads
 *============================================================================
 * Description:     Kick each thread's event to wake it up
 * Parameters:    
 *     lambda - the worker thread pool
 * Returns:  
 */
static void lambda_wake_up_all_threads(bitd_lambda_handle lambda) {
    bitd_lambda_thread *thread;

    bitd_mutex_lock(lambda->m);
    
    /* Walk the thread pool once, to stop each thread */
    for (thread = lambda->thread_head;
	 thread != THREAD_HEAD(lambda);
	 thread = thread->next) {
	/* Kick the event, to wake up the thread one last time */
	bitd_event_set(thread->ev);
    }

    bitd_mutex_unlock(lambda->m);
} 


/*
 *============================================================================
 *                        bitd_lambda_init
 *============================================================================
 * Description:
 * Parameters:
 * Returns:
 */
bitd_lambda_handle bitd_lambda_init(char *name) {
    bitd_lambda *lambda;

    /* Parameter check */
    if (!name) {
        return NULL;
    }

    /* Allocate the pool control block */
    lambda = calloc(1, sizeof(*lambda));
    lambda->thread_max = THREAD_MAX_DEF;
    lambda->thread_idle_tmo = THREAD_IDLE_TMO_DEF;
    lambda->task_max = TASK_MAX_DEF;

    /* Create the pool mutex */
    lambda->m = bitd_mutex_create();

    /* Initialize all lists */
    lambda->thread_head = THREAD_HEAD(lambda);
    lambda->thread_tail = THREAD_HEAD(lambda);
    lambda->thread_idle_head = THREAD_IDLE_HEAD(lambda);
    lambda->thread_idle_tail = THREAD_IDLE_HEAD(lambda);
    lambda->thread_exited_head = THREAD_EXITED_HEAD(lambda);
    lambda->thread_exited_tail = THREAD_EXITED_HEAD(lambda);
    
    lambda->name = strdup(name);
   
    return lambda;
}


/*
 *============================================================================
 *                        bitd_lambda_deinit
 *============================================================================
 * Description:
 * Parameters:
 *     lambda - the worker thread pool
 * Returns:
 */
void bitd_lambda_deinit(bitd_lambda_handle lambda) {
    bitd_lambda_task *task;

    if (lambda) {
        lambda->stopping_p = TRUE;

        /* Walk the thread pool once, to stop each thread */
	lambda_wake_up_all_threads(lambda);

	/* No need to take mutex. No new tasks are allowed since lambda is 
	   stopping. Therefore, no new threads are created. */
	
        /* Wait for each thread to exit */
	while (lambda->thread_head != THREAD_HEAD(lambda)) {
	    lambda_join_thread(lambda->thread_head);
	}

	/* Free the task list, in case there were tasks pending 
	   while the worker thread pool was stopped */
	while (lambda->task_start) {
	    task = lambda->task_start;
	    lambda->task_start = task->next;
	    
	    /* Execute the task but tell it we're stopping so it can
	       quickly exit */
	    task->f(task->cookie, &lambda->stopping_p);
	    free(task);

	    lambda->n_tasks--;	    
	}

	bitd_assert(!lambda->n_tasks);

        /* Destroy the mutex */
        bitd_mutex_destroy(lambda->m);
	
	free(lambda->name);
        free(lambda);
    }
}


/*
 *============================================================================
 *                        bitd_lambda_set_thread_max
 *============================================================================
 * Description:     Set the max number of worker thread pool tasks
 * Parameters:    
 *     lambda - the worker thread pool
 *     task_max - the max number of tasks
 * Returns:  
 */
void bitd_lambda_set_thread_max(bitd_lambda_handle lambda, int thread_max) {
    lambda->thread_max = thread_max;
} 


/*
 *============================================================================
 *                        bitd_lambda_set_thread_idle_tmo
 *============================================================================
 * Description:     Set the thread idle timeout. Threads that are idle
 *     for longer than the thread_idle_tmo will exit.
 * Parameters:    
 *     lambda - the worker thread pool
 *     thread_idle_tmo - the inactivity timeout (in msecs)
 * Returns:  
 */
void bitd_lambda_set_thread_idle_tmo(bitd_lambda_handle lambda,
				bitd_uint32 thread_idle_tmo) {
    bitd_boolean wake_up_threads = FALSE;

    if (thread_idle_tmo < lambda->thread_idle_tmo) {
	wake_up_threads = TRUE;
    }

    lambda->thread_idle_tmo = thread_idle_tmo;
    if (wake_up_threads) {
	lambda_wake_up_all_threads(lambda);
    }
} 


/*
 *============================================================================
 *                        bitd_lambda_set_task_max
 *============================================================================
 * Description:     Set the max number of worker thread pool tasks
 * Parameters:    
 *     lambda - the worker thread pool
 *     task_max - the max number of tasks
 * Returns:  
 */
void bitd_lambda_set_task_max(bitd_lambda_handle lambda, int task_max) {
    lambda->task_max = task_max;
} 


/*
 *============================================================================
 *                        bitd_lambda_exec_task
 *============================================================================
 * Description:  Execute a task. User needs to free f_arg 
 *     if return code is FALSE.
 * Parameters:
 * Returns:
 */
bitd_boolean bitd_lambda_exec_task(bitd_lambda_handle lambda,
				   bitd_lambda_task_func_t f,
				   void *cookie) {
    bitd_lambda_task *task = NULL;
    bitd_lambda_thread *thread;

    if (!lambda || !f) {
        return FALSE;
    }

    /* Check if we're stopping */
    if (lambda->stopping_p) {
	/* Give up without freeing argument */
	return FALSE;
    }

    /* Run the exited thread garbage collector */
    lambda_join_all_exited_threads(lambda);

    bitd_mutex_lock(lambda->m);

    /* Ensure we're not exceeding the max number of tasks */
    if (lambda->task_max && lambda->n_tasks >= lambda->task_max) {
	/* Give up without freeing argument */
	bitd_mutex_unlock(lambda->m);
	return FALSE;
    }

    /* Allocate the task */
    task = calloc(1, sizeof(*task));
    task->f = f;
    task->cookie = cookie;

    /* Increment the number of tasks */
    lambda->n_tasks++;

    /* Enqueue to the end of the task list */
    if (!lambda->task_start) {
        lambda->task_start = task;
        
    }
    if (lambda->task_end) {
        lambda->task_end->next = task;
    }
    lambda->task_end = task;

    /* Is there an idle thread available? */
    thread = lambda->thread_idle_head;
    if (thread != THREAD_IDLE_HEAD(lambda)) {
	/* Dequeue this thread from the idle list */
	thread->idle_prev->idle_next = thread->idle_next;
	thread->idle_next->idle_prev = thread->idle_prev;
	thread->idle_next = NULL;
	thread->idle_prev = NULL;

	/* Wake it up */
        dbg_printf("Kick %s event\n", thread->name);
        bitd_event_set(thread->ev);
    } else {
	/* Try to create a new thread - this may fail, in which case
	   the task is queued and will be executed when a thread
	   becomes available */
	lambda_create_thread(lambda);
    }

    bitd_mutex_unlock(lambda->m);

    return TRUE;
}
