/*****************************************************************************
 *
 * Original Author: Andrei Radulescu-Banu
 * Creation Date:   
 * Description: 
 * 
 * Copyright (C) 2018 by Andrei Radulescu-Banu.  All Rights Reserved.
 * Unauthorized reproduction, modification, distribution, transmission, 
 * republication, display or performance are strictly prohibited.
 ****************************************************************************/

/*****************************************************************************
 *                                INCLUDE FILES 
 *****************************************************************************/
#include "mmr.h"


/*****************************************************************************
 *                             MANIFEST CONSTANTS
 *****************************************************************************/



/*****************************************************************************
 *                                  MACROS 
 *****************************************************************************/



/*****************************************************************************
 *                                  TYPES
 *****************************************************************************/



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
 *                        mmr_task_inst_get
 *============================================================================
 * Description:     Get a task inst control block. Create it if 
 *    it does not exist. Bump up the refcount if it does exist.
 * Parameters:    
 *     module - the owner module
 *     task_api - the api of the task
 * Returns:  
 *     Pointer to the task instance control block.
 */
struct mmr_task_inst_s *mmr_task_inst_get(struct mmr_task_s *task,
					  char *task_inst_name) {
    struct mmr_task_inst_s *task_inst;
    
    /* Parameter check */
    if (!task || !task_inst_name) {
	return NULL;
    }

    /* Does the task already exist? */
    task_inst = mmr_task_inst_find(task, task_inst_name);
    if (task_inst) {
	/* Bump up the refcount, and return it */
	task_inst->refcount++;
	return task_inst;
    }

    /* Create the task instance */
    task_inst = calloc(1, sizeof(*task_inst));

    /* Configure the task instance */
    task_inst->task = task;
    task_inst->name = strdup(task_inst_name);
    task_inst->refcount = 1;

    task_inst->input_queue_head = INPUT_QUEUE_HEAD(task_inst);
    task_inst->input_queue_tail = INPUT_QUEUE_HEAD(task_inst);

    /* Chain this task to the end of the task-owned list */
    task_inst->prev = task->task_inst_tail;
    task_inst->next = task->task_inst_tail->next;
    task_inst->prev->next = task_inst;
    task_inst->next->prev = task_inst;

    /* Create the run timer */
    task_inst->run_timer = bitd_timer_create();

    /* Set the magic marker */
    task_inst->magic = TASK_INST_MAGIC;

    return task_inst;
} 


/*
 *============================================================================
 *                        mmr_task_inst_find
 *============================================================================
 * Description:     Get a task inst control block. Don't create it if 
 *    it does not exist. 
 * Parameters:    
 *     task - the tast control block
 *     task_inst_name - the name of the task instance
 * Returns:  
 *     Pointer to the task instance block.
 */
struct mmr_task_inst_s *mmr_task_inst_find(struct mmr_task_s *task,
					   char *task_inst_name) {
    struct mmr_task_inst_s *task_inst;

    if (!task || !task_inst_name) {
	return NULL;
    }

    for (task_inst = task->task_inst_head; 
	 task_inst != TASK_INST_HEAD(task); 
	 task_inst = task_inst->next) {
	
	if (!strcmp(task_inst->name, task_inst_name)) {
	    return task_inst;
	}
    }

    return NULL;
} 


/*
 *============================================================================
 *                        mmr_task_inst_update
 *============================================================================
 * Description:    Update the configuration of a task instance
 * Parameters:    
 *     task_inst - the task instance control block, which already has
 *         had it schedule, or parameters changed, and needs to be updated
 * Returns:  
 */
mmr_err_t mmr_task_inst_update(struct mmr_task_inst_s *task_inst) {
    struct mmr_task_s *task = task_inst->task;
    bitd_boolean create_p;
    
    if (!IS_SET(task_inst->state, TASK_INST_PARAMS_CHANGED)) {
	return mmr_err_ok;
    }

    /* Need to (re)create the task instance */
    if (IS_SET(task_inst->state, TASK_INST_PENDING_RUN)) {
	/* Stop the task instance */
	task_inst->stopping_p = TRUE;
	if (IS_SET(task_inst->state, TASK_INST_RUNNING)) {
	    /* Kill the running task instance. Upon termination, 
	       it will recreate itself, because the 
	       TASK_INST_PARAMS_CHANGED bit is set */
	    task->api.task_inst_kill(task_inst->user_task_inst, 
				     BITD_TASK_SIGSTOP);
	}
    } else {
	/* Not stopping anymore */
	task_inst->stopping_p = FALSE;

	/* The task instance is not running - we can (re)create it */
	create_p = TRUE;
	if (task_inst->user_task_inst) {
	    /* Already created */ 
	    if (task->api.task_inst_update) {
		/* Some tasks support the update API - call it!... */
		task->api.task_inst_update(task_inst->user_task_inst,
					   task_inst->params.args,
					   task_inst->params.tags);		
		/* No need to create it, it's been updated */
		create_p = FALSE;
	    } else {
		/* Destroy it and recreate it */
		task->api.task_inst_destroy(task_inst->user_task_inst);
	    }
	}
	
	if (create_p) {
	    /* Create the task instance */
	    task_inst->user_task_inst = 
		task->api.task_inst_create(task->name,
					   task_inst->name,
					   task_inst,
					   task_inst->params.args,
					   task_inst->params.tags);
	}

	CLR_BIT(task_inst->state, TASK_INST_PARAMS_CHANGED);
	
	/* Was the task instance successfully created? */
	if (!task_inst->user_task_inst) {
	    mmr_log(log_level_trace, 
		    "Task inst %s: %s create failed",
		    task->name, task_inst->name);
	    return mmr_err_task_inst_create_failed;
	}
    }

    return mmr_err_ok;
}


/*
 *============================================================================
 *                        mmr_task_inst_stop
 *============================================================================
 * Description:     Stop a task instance
 * Parameters:    
 *     task_inst - the task instance
 * Returns:  
 */
void mmr_task_inst_stop(struct mmr_task_inst_s *task_inst) {
    struct mmr_task_s *task = task_inst->task;

    if (task_inst->stopping_p) {
	/* Already stopping */
	return;
    }

    /* Set the stop flag */
    task_inst->stopping_p = TRUE;
    
    /* Is the task instance running? */
    if (IS_SET(task_inst->state, TASK_INST_RUNNING)) {
	/* Call the stop API */
	task->api.task_inst_kill(task_inst->user_task_inst,
				 BITD_TASK_SIGSTOP);

	mmr_log(log_level_trace, 
		"%s: %s: task_inst_kill(BITD_TASK_SIGSTOP) called",
		task->name, task_inst->name);
    }
} 


/*
 *============================================================================
 *                        mmr_task_inst_is_stopped
 *============================================================================
 * Description:     Check if a task instance has stopped
 * Parameters:    
 *     task_inst - the task instance
 * Returns:  
 */
bitd_boolean mmr_task_inst_is_stopped(struct mmr_task_inst_s *task_inst) {

    if (task_inst->stopping_p && 
	!IS_SET(task_inst->state, TASK_INST_RUNNING)) {
	return TRUE;
    }

    return FALSE;
} 


/*
 *============================================================================
 *                        mmr_task_inst_release
 *============================================================================
 * Description:     Release a task instance given its control block pointer
 * Parameters:    
 *     force_p - wait for the task instace to stop after it was stopped
 * Returns:  
 */
void mmr_task_inst_release(struct mmr_task_inst_s *task_inst) {
    struct mmr_task_s *task;
 
    if (!task_inst) {
	return;
    }

    task = task_inst->task;

    /* If the task instance is running, and the refcount is one, stop it */
    if (task_inst->refcount == 1) {

	if (task_inst->triggered_next) {
	    /* Remove from triggered list */
	    task_inst->triggered_prev->triggered_next = task_inst->triggered_next;
	    task_inst->triggered_next->triggered_prev = task_inst->triggered_prev;
	}

	/* Stop the task instance */
	mmr_task_inst_stop(task_inst);

	/* Wait until task instance is not running, and (if a triggered
	   task instance) not scheduled */
	for (;;) {
	    if (mmr_task_inst_is_stopped(task_inst)) {
		/* Task instance is stopped */
		if (task_inst->sched_type == task_inst_sched_triggered_t ||
		    task_inst->sched_type == task_inst_sched_triggered_raw_t) {
		    /* A triggered task instance. Wait until it's not
		       scheduled, pending to run or running */
		    if (!IS_SET(task_inst->state, 
				TASK_INST_SCHEDULED|
				TASK_INST_PENDING_RUN|
				TASK_INST_RUNNING)) {
			break;
		    }
		} else {
		    /* A non triggered task instance. Wait until it's not
		       pending to run, or running. Periodic task instances
		       are always scheduled. */
		    if (!IS_SET(task_inst->state, 
				TASK_INST_PENDING_RUN|
				TASK_INST_RUNNING)) {
			break;
		    }
		}
	    }

	    bitd_mutex_unlock(g_mmr_cb->lock);
	    bitd_event_wait(g_mmr_cb->task_inst_stopped_ev, 100);
	    bitd_mutex_lock(g_mmr_cb->lock);
	}

	/* This instance is not referenced anywhere else, and can be
	   destroyed */
	if (task_inst->user_task_inst) {
	    /* Destroy the task instance */
	    task->api.task_inst_destroy(task_inst->user_task_inst);
	    task_inst->user_task_inst = NULL;
	}
    }
	    
    /* Bump down the reference count */
    task_inst->refcount--;

    /* When the refcount is zero, release the task instance. */
    if (!task_inst->refcount) {
	/* Unchain the task instance */
	task_inst->next->prev = task_inst->prev;
	task_inst->prev->next = task_inst->next;

	/* Clear the input queue */
	while (task_inst->input_queue_head != INPUT_QUEUE_HEAD(task_inst)) {
	    struct input_queue_s *iq = task_inst->input_queue_head;
	    
	    /* Unchain from the list */
	    iq->prev->next = iq->next;
	    iq->next->prev = iq->prev;
	    
	    /* Release the memory */
	    bitd_object_free(&iq->input);
	    free(iq);
	}

	/* Destroy the run timer */
	bitd_timer_destroy(task_inst->run_timer);
	
	/* Free the task instance */
	free(task_inst->name);
	bitd_nvp_free(task_inst->sched);
	bitd_nvp_free(task_inst->params.args);
	bitd_object_free(&task_inst->params.input);
	bitd_nvp_free(task_inst->params.tags);
	task_inst->magic = 0;
	free(task_inst);
    }
} 


/*
 *============================================================================
 *                        mmr_inst_run_timer_expired
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
void mmr_task_inst_run_timer_expired(bitd_timer t,
				     void *cookie) {
    struct mmr_task_inst_s *task_inst = (struct mmr_task_inst_s *)cookie;
    bitd_boolean ret;
    
    mmr_log(log_level_trace, "%s: %s: Run timer expired",
	    task_inst->task->name,
	    task_inst->name);

    SET_BIT(task_inst->state, TASK_INST_PENDING_RUN);

    /* Bear trap */
    if (task_inst->sched_type == task_inst_sched_triggered_t ||
	task_inst->sched_type == task_inst_sched_triggered_raw_t) {
	/* If input queue is empty, this means the run method completed its run,
	   and the task was rescheduled twice for the same input */
	if (task_inst->input_queue_head == INPUT_QUEUE_HEAD(task_inst)) {
	    return;
	}
    }

    ret = bitd_lambda_exec_task(g_mmr_cb->lambda,
			      mmr_task_inst_run,
			      task_inst);
    if (!ret) {
	mmr_log(log_level_err, "%s: %s: bitd_lambda_task_exec() returned FALSE",
		task_inst->task->name,
		task_inst->name);

	/* Should never happen */
	bitd_assert(0);
    }
} 


/*
 *============================================================================
 *                        mmr_task_inst_run
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
void mmr_task_inst_run(void *cookie, bitd_boolean *stopping_p) {
    struct mmr_task_inst_s *task_inst = (struct mmr_task_inst_s *)cookie;
    mmr_err_t ret = mmr_err_ok;
    int task_inst_ret;
    struct input_queue_s *iq = NULL;
    bitd_object_t *input = NULL;

    /* Set the run flag. This ensures that the config can't change past
       this point. */
    bitd_mutex_lock(g_mmr_cb->lock);

    if (task_inst->magic != TASK_INST_MAGIC) {
	bitd_assert(0);
	bitd_mutex_unlock(g_mmr_cb->lock);
	return;
    }

    if (task_inst->stopping_p) {
	/* Skip this run */
	goto update;
    }

    switch (task_inst->sched_type) {
    case task_inst_sched_triggered_t:
    case task_inst_sched_triggered_raw_t:
	iq = task_inst->input_queue_head;
	if (iq != INPUT_QUEUE_HEAD(task_inst)) {
	    /* Dequeue the element */
	    iq->prev->next = iq->next;
	    iq->next->prev = iq->prev;
	    
	    /* Use enqueued input */
	    input = &iq->input;

	    g_mmr_cb->input_queue_size--;
	    
	    if (g_mmr_cb->input_queue_size > g_mmr_cb->input_queue_max/2 - 1) {
		mmr_log(log_level_warn, "Input queue size decremented to %d/%d",
			g_mmr_cb->input_queue_size, g_mmr_cb->input_queue_max);
	    }
	    
	} else {
	    /* All input has been processed */
	    goto end;
	}
	break;
    default:
	/* Use params input */
	input = &task_inst->params.input;
	break;
    }

    mmr_log(log_level_trace, "%s: %s: Run %llu begin",
	    task_inst->task->name,
	    task_inst->name,
	    task_inst->run_id);

    SET_BIT(task_inst->state, TASK_INST_RUNNING);

    bitd_mutex_unlock(g_mmr_cb->lock);    
    

    /* Execute the task instance run. This will typically block, but
       we are in a worker thread context, outside the mmr lock - and the mmr 
       will not be blocked. */
    task_inst_ret = task_inst->task->api.task_inst_run(task_inst->user_task_inst,
						       input);

    mmr_log(log_level_trace, "%s: %s: Run %llu end, ret %d",
	    task_inst->task->name,
	    task_inst->name,
	    task_inst->run_id,
	    task_inst_ret);

    if (iq) {
	/* Release memory */
	bitd_object_free(&iq->input);
	free(iq);
    }

    bitd_mutex_lock(g_mmr_cb->lock);

    /* Bump up the run_id */
    task_inst->run_id++;

 update:
    /* Clear the scheduled and run flags */
    CLR_BIT(task_inst->state, 
	    TASK_INST_SCHEDULED|TASK_INST_PENDING_RUN|TASK_INST_RUNNING);

    /* Update the task instance, in case the config changed */
    ret = mmr_task_inst_update(task_inst);
    if (ret != mmr_err_ok) {
	mmr_log(log_level_info, 
		"Task inst %s: %s update failed, stopping task instance",
		task_inst->task->name, task_inst->name);
    } else {
	/* Reschedule the task instance run */
	mmr_schedule_task_inst(task_inst);
    }

 end:
    bitd_mutex_unlock(g_mmr_cb->lock);
} 

