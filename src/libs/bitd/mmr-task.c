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
 *                        mmr_task_get
 *============================================================================
 * Description:     Get a task control block. Create it if 
 *    it does not exist. Bump up the refcount if it does exist.
 * Parameters:    
 *     module - the owner module
 *     task_name - the name of the task
 *     task_api - the api of the task
 * Returns:  
 *     Pointer to the task control block.
 */
struct mmr_task_s *mmr_task_get(mmr_module_t module,
				char *task_name,
				bitd_task_api_t *task_api) {
    struct mmr_task_s *task;
    
    /* Parameter check */
    if (!module || !task_name || !task_api) {
	return NULL;
    }

    /* Does the task already exist? */
    task = mmr_task_find(task_name);
    if (task) {
	/* Does it have the same module and API? */
	if (task->module == module &&
	    task->api.task_inst_create == task_api->task_inst_create &&
	    task->api.task_inst_destroy == task_api->task_inst_destroy &&
	    task->api.task_inst_run == task_api->task_inst_run &&
	    task->api.task_inst_kill == task_api->task_inst_kill) {
	    /* Bump up the refcount, and return it */
	    task->refcount++;
	    return task;
	} else {
	    /* A module implementation bug */
	    if (task->module != module) {
		mmr_log(log_level_err, "Duplicate task %s in modules %s and %s",
			task_name,
			task->module->dll_name, 
			module->dll_name);
	    } else {
		mmr_log(log_level_err, "Task %s in module %s has duplicate APIs",
			task_name,
			module->dll_name);
	    }
	    return NULL;
	}
    }

    /* Create the task */
    task = calloc(1, sizeof(*task));

    /* Configure the task */
    task->api = *task_api;
    task->name = strdup(task_name);
    task->module = module;
    task->task_inst_head = TASK_INST_HEAD(task);
    task->task_inst_tail = TASK_INST_HEAD(task);
    task->refcount = 1;

    /* Chain this task to the end of the module-owned list */
    task->prev = module->task_tail;
    task->next = module->task_tail->next;
    task->prev->next = task;
    task->next->prev = task;

    return task;
} 


/*
 *============================================================================
 *                        mmr_task_find
 *============================================================================
 * Description:     Get a task control block. Don't create it if 
 *    it does not exist. 
 * Parameters:    
 *     task_name - the name of the task
 * Returns:  
 *     Pointer to the task block.
 */
struct mmr_task_s *mmr_task_find(char *task_name) {
    struct mmr_module_s *module;
    struct mmr_task_s *task;

    if (!task_name) {
	return NULL;
    }

    for (module = g_mmr_cb->module_head; 
	 module != MMR_MODULE_HEAD(g_mmr_cb); 
	 module = module->next) {
	
	for (task = module->task_head;
	     task != MODULE_TASK_HEAD(module);
	     task = task->next) {

	    if (!strcmp(task->name, task_name)) {
		return task;
	    }
	}
    }

    return NULL;
} 


/*
 *============================================================================
 *                        mmr_task_stop
 *============================================================================
 * Description:     Stop a task and all its task instances
 * Parameters:    
 *     task - the task we are stopping
 * Returns:  
 */
void mmr_task_stop(struct mmr_task_s *task) {
    struct mmr_task_inst_s *task_inst;

    if (task->stopping_p) {
	/* Already stopping */
	return;
    }

    /* Set the stop flag */
    task->stopping_p = TRUE;

    /* Stop all task instances of this task */
    for (task_inst = task->task_inst_head;
	 task_inst != TASK_INST_HEAD(task);
	 task_inst = task_inst->next) {	
	mmr_task_inst_stop(task_inst);
    }
} 


/*
 *============================================================================
 *                        mmr_task_is_stopped
 *============================================================================
 * Description:     Check if a task and all its task instances are stopped
 * Parameters:    
 *     task - the task we are stopping
 * Returns:  
 */
bitd_boolean mmr_task_is_stopped(struct mmr_task_s *task) {
    struct mmr_task_inst_s *task_inst;

    /* Check that this task is stopping */
    if (!task->stopping_p) {
	return FALSE;
    }

    /* Check that all task instances of this task have stopped */
    for (task_inst = task->task_inst_head;
	 task_inst != TASK_INST_HEAD(task);
	 task_inst = task_inst->next) {	
	if (!mmr_task_inst_is_stopped(task_inst)) {
	    return FALSE;
	}
    }

    return TRUE;
} 


/*
 *============================================================================
 *                        mmr_task_release
 *============================================================================
 * Description:     Release a task given its control block pointer
 * Parameters:    
 * Returns:  
 */
void mmr_task_release(struct mmr_task_s *task) {

    if (!task) {
	return;
    }

    /* Bump down the reference count */
    task->refcount--;
    bitd_assert(task->refcount >= 0);

    /* When reference count reaches zero, release the task */
    if (!task->refcount) {

	/* Stop this task and all its task instances */
	mmr_task_stop(task);

	/* Wait for the task to be stopped */
	while (!mmr_task_is_stopped(task)) {
	    bitd_mutex_unlock(g_mmr_cb->lock);
	    bitd_event_wait(g_mmr_cb->task_inst_stopped_ev, 100);
	    bitd_mutex_lock(g_mmr_cb->lock);
	}

	/* Remove all task instances */
	while (task->task_inst_head != TASK_INST_HEAD(task)) {
	    mmr_task_inst_release(task->task_inst_head);
	}

	/* Unchain the task */
	task->next->prev = task->prev;
	task->prev->next = task->next;
	
	/* Free the task */
	free(task->name);
	free(task);
    }
} 
