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
 *                        mmr_api_lock
 *============================================================================
 * Description:     Lock the g_mmr_cb->mmr_api_lock and, inside that, the 
 *                g_mmr_cb->lock.
 * Parameters:    
 * Returns:  
 */
void mmr_api_lock(void) {
    bitd_mutex_lock(g_mmr_cb->mmr_api_lock);
    bitd_mutex_lock(g_mmr_cb->lock);
} 


/*
 *============================================================================
 *                        mmr_api_unlock
 *============================================================================
 * Description:     Unlock the g_mmr_cb->mmr_api_lock, after unlockinf the 
 *                g_mmr_cb->lock.
 * Parameters:    
 * Returns:  
 */
void mmr_api_unlock(void) {
    bitd_mutex_unlock(g_mmr_cb->lock);
    bitd_mutex_unlock(g_mmr_cb->mmr_api_lock);
} 


/*
 *============================================================================
 *                        mmr_init
 *============================================================================
 * Description:     Initialize the module manager
 * Parameters:    
 * Returns:  
 */
mmr_err_t mmr_init(void) {
    mmr_err_t ret = mmr_err_ok;
    
    if (!g_mmr_cb) {
	g_mmr_cb = calloc(1, sizeof(*g_mmr_cb));
	
	g_mmr_cb->lock = bitd_mutex_create();
	g_mmr_cb->mmr_api_lock = bitd_mutex_create();
	g_mmr_cb->results_lock = bitd_mutex_create();
	g_mmr_cb->module_head = MMR_MODULE_HEAD(g_mmr_cb);
	g_mmr_cb->module_tail = MMR_MODULE_HEAD(g_mmr_cb);
	g_mmr_cb->triggered_head = TRIGGERED_HEAD(g_mmr_cb);
	g_mmr_cb->triggered_tail = TRIGGERED_HEAD(g_mmr_cb);
	g_mmr_cb->timers = bitd_timer_list_create();
	bitd_timer_list_set_ticks_max(g_mmr_cb->timers, 250);
	
	/* The input queue max size */
	g_mmr_cb->input_queue_max = 250;
	
	/* Create the events */
	g_mmr_cb->event_loop_ev = bitd_event_create(BITD_EVENT_FLAG_POLL);
	g_mmr_cb->task_inst_stopped_ev = bitd_event_create(0);

	/* Initialize the worker thread pool */
	g_mmr_cb->lambda = bitd_lambda_init("mmr-worker-thread-pool");

	/* Start the event loop thread */
	g_mmr_cb->event_loop_th = bitd_create_thread("mmr-event-loop",
						     mmr_event_loop,
						     BITD_DEFAULT_PRIORITY,
						     BITD_DEFAULT_STACK_SIZE,
						     NULL);
	if (!g_mmr_cb->event_loop_th) {
	    bitd_assert(0);
	    ret = mmr_err_unknown;
	}
    }

    if (ret != mmr_err_ok) {
	mmr_deinit();
    }

    return ret;
} 


/*
 *============================================================================
 *                        mmr_deinit
 *============================================================================
 * Description:     Deinitialize the module manager
 * Parameters:    
 * Returns:  
 */
void mmr_deinit(void) {

    if (g_mmr_cb) {
	/* Release all modules */
	mmr_api_lock();

	while (g_mmr_cb->module_head != MMR_MODULE_HEAD(g_mmr_cb)) {
	    /* Release the head module */
	    mmr_module_release(g_mmr_cb->module_head);
	}
	
	mmr_log(log_level_trace, "Module manager deinitialized");

	mmr_api_unlock();

	/* Stop the event loop */
	g_mmr_cb->stopping_p = TRUE;
	bitd_event_set(g_mmr_cb->event_loop_ev);

	/* Wait for event loop to exit */
	bitd_join_thread(g_mmr_cb->event_loop_th);

	/* Deinitialize the worker thread pool */
	bitd_lambda_deinit(g_mmr_cb->lambda);
	
	/* Destroy the events */
	bitd_event_destroy(g_mmr_cb->event_loop_ev);
	bitd_event_destroy(g_mmr_cb->task_inst_stopped_ev);

	bitd_nvp_free(g_mmr_cb->tags);
	bitd_nvp_free(g_mmr_cb->config);

	if (g_mmr_cb->module_path) {
	    free(g_mmr_cb->module_path);
	}
	bitd_timer_list_destroy(g_mmr_cb->timers);
	bitd_mutex_destroy(g_mmr_cb->results_lock);
	bitd_mutex_destroy(g_mmr_cb->mmr_api_lock);
	bitd_mutex_destroy(g_mmr_cb->lock);
	free(g_mmr_cb);
    }
}


/*
 *============================================================================
 *                        mmr_get_error_name
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
char *mmr_get_error_name(mmr_err_t err) {
    static char *err_name[] = {
	"success",
	"pending",
	"not initialized",
	"invalid parameters",
	"module not found",
	"invalid bitd_module_load or bitd_module_unload symbol in module",
	"module load failed",
	"no task found",
	"task create failed",
	"task inst create failed",
	"unknown error"
    };

    if ((int)err >= 0 && err < mmr_err_max) {
	return err_name[err];
    }
    
    bitd_assert(0);

    return "unknown error";
} 


/*
 *============================================================================
 *                        mmr_set_config
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
mmr_err_t mmr_set_config(bitd_nvp_t config) {
    /* List of supported config options */
    char *elem_names[] = {"n-worker-threads"};
    int n_elem_names = sizeof(elem_names)/sizeof(elem_names[0]);

    if (!g_mmr_cb) {
	return mmr_err_not_initialized;
    }

    /* Copy the config */
    mmr_api_lock();
    bitd_nvp_free(g_mmr_cb->config);
    g_mmr_cb->config = bitd_nvp_trim(config, elem_names, n_elem_names);
    mmr_api_unlock();

    return mmr_err_ok;    
} 


/*
 *============================================================================
 *                        mmr_get_config
 *============================================================================
 * Description:     
 * Parameters:    
 *     config_nvp [OUT] - the mmr configuration nvp
 * Returns:  
 */
mmr_err_t mmr_get_config(bitd_nvp_t *config) {

    if (!config) {
	return mmr_err_invalid_param;
    }

    if (!g_mmr_cb) {
	return mmr_err_not_initialized;
    }

    /* Copy the config */
    mmr_api_lock();
    *config = bitd_nvp_clone(g_mmr_cb->config);
    mmr_api_unlock();

    return mmr_err_ok;    
} 


/*
 *============================================================================
 *                        mmr_set_vlog_func
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
mmr_err_t mmr_set_vlog_func(mmr_vlog_func_t *f) {

    if (!g_mmr_cb) {
	return mmr_err_not_initialized;
    }

    g_mmr_cb->vlog = f;

    return mmr_err_ok;    
} 


/*
 *============================================================================
 *                        mmr_set_tags
 *============================================================================
 * Description:     Set the global mmr tags
 * Parameters:    
 * Returns:  
 */
mmr_err_t mmr_set_tags(bitd_nvp_t tags) {
    if (!g_mmr_cb) {
	return mmr_err_not_initialized;
    }

    /* Copy the tags */
    mmr_api_lock();
    bitd_nvp_free(g_mmr_cb->tags);
    g_mmr_cb->tags = bitd_nvp_clone(tags);
    mmr_api_unlock();

    return mmr_err_ok;    
} 


/*
 *============================================================================
 *                        mmr_get_tags
 *============================================================================
 * Description:     
 * Parameters:    
 *     tags [OUT] - pointer to the mmr global tags
 * Returns:  
 */
mmr_err_t mmr_get_tags(bitd_nvp_t *tags) {

    if (!tags) {
	return mmr_err_invalid_param;
    }

    if (!g_mmr_cb) {
	return mmr_err_not_initialized;
    }

    /* Copy the tags */
    mmr_api_lock();
    *tags = bitd_nvp_clone(g_mmr_cb->tags);
    mmr_api_unlock();

    return mmr_err_ok;    
} 


/*
 *============================================================================
 *                        mmr_set_module_path
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
mmr_err_t mmr_set_module_path(char *path) {

    if (!g_mmr_cb) {
	return mmr_err_not_initialized;
    }

    mmr_api_lock();
    if (g_mmr_cb->module_path) {
	free(g_mmr_cb->module_path);
	g_mmr_cb->module_path = NULL;
    }
    if (path) {
	g_mmr_cb->module_path = strdup(path);
    }
    mmr_api_unlock();

    return mmr_err_ok;    
} 


/*
 *============================================================================
 *                        mmr_load_module
 *============================================================================
 * Description:     Load a module
 * Parameters:    
 *     module_name - the module name 
 * Returns:  
 */
mmr_err_t mmr_load_module(char *module_name) {
    mmr_err_t ret = mmr_err_ok;
    struct mmr_module_s *module;

    if (!g_mmr_cb) {
	return mmr_err_not_initialized;
    }

    if (!module_name) {
	return mmr_err_invalid_param;
    }
   
    mmr_api_lock();
    module = mmr_module_get(module_name, &ret);
    if (!module) {
	mmr_log(log_level_err, "Failed to load module %s: %s", 
		module_name, mmr_get_error_name(ret));
	ret = mmr_err_module_load_failed;
	goto end;
    }

    mmr_log(log_level_debug, "Loaded module %s refcount %d", 
	    module_name, module->refcount);
 end:
    mmr_api_unlock();

    return ret;
} 


/*
 *============================================================================
 *                        mmr_unload_module
 *============================================================================
 * Description:     Unload a module
 * Parameters:    
 *     module_name - the module name 
 * Returns:  
 */
mmr_err_t mmr_unload_module(char *module_name) {
    struct mmr_module_s *module;
    int refcount;

    if (!g_mmr_cb) {
	return mmr_err_not_initialized;
    }
   
    mmr_api_lock();
    module = mmr_module_find(module_name);
    if (!module) {
	mmr_log(log_level_debug, 
		"Failed to unload module %s, module not found", 
		module_name);
    } else {
	refcount = module->refcount - 1;
	mmr_module_release(module);
	mmr_log(log_level_debug, "Unloaded module %s, refcount %d", 
		module_name, refcount);
    }
    mmr_api_unlock();

    return mmr_err_ok;    
} 


/*
 *============================================================================
 *                        mmr_task_register
 *============================================================================
 * Description:     Called from within a module to register a task
 * Parameters:    
 * Returns:  
 */
mmr_task_t mmr_task_register(mmr_module_t module,
			     char *task_name,
			     bitd_task_api_t *task_api) {
    struct mmr_task_s *task;

    if (!task_name || !task_api || !g_mmr_cb) {
	return NULL;
    }

   
    mmr_api_lock();

    task = mmr_task_get(module, task_name, task_api);
    if (!task) {
	mmr_log(log_level_err, "Module %s: failed to register task %s",
		module->name, 
		task_name);
    } else {
	mmr_log(log_level_debug, 
		"Module %s: task %s registered, refcount %d",
		module->name, 
		task_name,
		task->refcount);
    }

    mmr_api_unlock();

    return task;
} 


/*
 *============================================================================
 *                        mmr_task_unregister
 *============================================================================
 * Description:     Called from within a task to unregister a module
 * Parameters:    
 * Returns:  
 */
void mmr_task_unregister(mmr_task_t task) {
    
    if (!task || !g_mmr_cb) {
	return;
    }

    mmr_api_lock();

    mmr_log(log_level_debug, 
	    "Module %s: task %s unregistered, refcount %d",
	    task->module->name,
	    task->name,
	    task->refcount-1);
    mmr_task_release(task);

    mmr_api_unlock();
} 


/*
 *============================================================================
 *                        mmr_task_inst_create
 *============================================================================
 * Description:     Create a task instance
 * Parameters:    
 *     task_name, task_inst_name - the name of the task and the task instance
 *     sched - the schedule of the task
 *     params - task instance parameters
 * Returns:  
 */
mmr_err_t mmr_task_inst_create(char *task_name,
			       char *task_inst_name,
			       bitd_nvp_t sched,
			       mmr_task_inst_params_t *params) {
    mmr_err_t ret = mmr_err_ok;
    struct mmr_task_s *task;
    struct mmr_task_inst_s *task_inst = NULL;
    bitd_nvp_t task_inst_tags; /* No need to free at the end */
    bitd_nvp_t merged_tags = NULL, merged_tags1 = NULL;

    if (!task_name || !task_inst_name) {
	return mmr_err_invalid_param;
    }

    if (!g_mmr_cb) {
	return mmr_err_not_initialized;
    }

    mmr_api_lock();

    task = mmr_task_find(task_name);
    if (!task) {
	mmr_log(log_level_err, 
		"Could not create task inst %s: %s: task does not exist",
		task_name, task_inst_name);
	ret = mmr_err_no_task;
	goto end;
    }
    
    task_inst = mmr_task_inst_get(task, task_inst_name);
    if (!task_inst) {
	mmr_log(log_level_err, 
		"Failed to create task inst %s: %s",
		task_name, task_inst_name);
	ret = mmr_err_task_inst_create_failed;
	goto end;
    }
    
    /* The task name and task inst name become tags. Note that
       task_inst_tags only has the task_inst_tags pointer itself
       allocated on the heap - we will call free() on it directly */
    task_inst_tags = bitd_nvp_alloc(2);
    task_inst_tags->e[0].name = "task";
    task_inst_tags->e[0].v.value_string = task_name;
    task_inst_tags->e[0].type = bitd_type_string;
    task_inst_tags->e[1].name = "task-instance";
    task_inst_tags->e[1].v.value_string = task_inst_name;
    task_inst_tags->e[1].type = bitd_type_string;
    task_inst_tags->n_elts = 2;
    
    /* Compute the merged tags. The base of the merge is NULL,
       we're simply overlaying the tags in the 2nd argument on top of
       the tags in the 1st argument */
    merged_tags = bitd_nvp_merge(task_inst_tags, params->tags, NULL);
    merged_tags1 = merged_tags;
    merged_tags = bitd_nvp_merge(g_mmr_cb->tags, merged_tags1, NULL);

    /* Free the task inst tags */
    free(task_inst_tags);

    /* Release the merged_tags1 */
    bitd_nvp_free(merged_tags1);

    /* Change the params */
    if (bitd_nvp_compare(params->args, task_inst->params.args)) {
	bitd_nvp_free(task_inst->params.args);
	task_inst->params.args = bitd_nvp_clone(params->args);
	SET_BIT(task_inst->state, TASK_INST_PARAMS_CHANGED);
    }
    if (bitd_object_compare(&params->input, &task_inst->params.input)) {
	bitd_object_free(&task_inst->params.input);
	bitd_object_clone(&task_inst->params.input, &params->input);
	SET_BIT(task_inst->state, TASK_INST_PARAMS_CHANGED);
    }
    if (bitd_nvp_compare(merged_tags, task_inst->params.tags)) {
	bitd_nvp_free(task_inst->params.tags);
	task_inst->params.tags = merged_tags;
	merged_tags = NULL;
	SET_BIT(task_inst->state, TASK_INST_PARAMS_CHANGED);
    }

    /* If the refcount is 1, even if the params have not changed, 
       we need to update the task */
    if (task_inst->refcount == 1) {
	SET_BIT(task_inst->state, TASK_INST_PARAMS_CHANGED);
    }

    /* Has the schedule nvp changed? */
    if (bitd_nvp_compare(sched, task_inst->sched)) {
	/* Save the schedule */
	bitd_nvp_free(task_inst->sched);
	task_inst->sched = bitd_nvp_clone(sched);
	SET_BIT(task_inst->state, TASK_INST_SCHED_CHANGED);
    }

    /* If the schedule type for the task inst is 'config', we force
       a schedule change. This ensure the task inst will re-run
       each time mmr_task_inst_create() is called. Config tasks are
       idempotent, and if the config does not chage, the run should
       be a no-op. When the module-agent gets a SIGHUP, mmr_task_inst_create() 
       is called, and 'config' type task instances will re-run, with the
       effect that SIGHUP triggers a re-read of the configuration. */
    if (task_inst->sched_type == task_inst_sched_config_t) {
	SET_BIT(task_inst->state, TASK_INST_SCHED_CHANGED);
    }

    /* Update the task instance, in case it changed */
    ret = mmr_task_inst_update(task_inst);
    if (ret != mmr_err_ok) {
	goto end;
    }

    /* Schedule the next run */
    mmr_schedule_task_inst(task_inst);
    
    mmr_log(log_level_debug, 
	    "Task inst %s: %s created, refcount %d",
	    task_name, task_inst_name, task_inst->refcount);

 end:
    if (ret != mmr_err_ok) {
	/* Release the task inst control block */
	mmr_task_inst_release(task_inst);
    }

    mmr_api_unlock();

    /* Release memory */
    bitd_nvp_free(merged_tags);

    return ret;
} 


/*
 *============================================================================
 *                        mmr_task_inst_prepare_destroy
 *============================================================================
 * Description:     Stop a task instance (if its refcount is one). The
 *     stop request is ignored if the refcount is not one.
 * Parameters:    
 *     task_name, task_inst_name - the name of the task and the task instance
 * Returns:  
 */
mmr_err_t mmr_task_inst_prepare_destroy(char *task_name,
					char *task_inst_name) {
    mmr_err_t ret = mmr_err_ok;
    struct mmr_task_s *task;
    struct mmr_task_inst_s *task_inst;
    
    if (!task_name || !task_inst_name) {
	return mmr_err_invalid_param;
    }
    
    if (!g_mmr_cb) {
	return mmr_err_not_initialized;
    }
    
    mmr_api_lock();
    
    task = mmr_task_find(task_name);
    if (!task) {
	mmr_log(log_level_err, 
		"Could not find task inst %s: %s: task does not exist",
		task_name, task_inst_name);
	ret = mmr_err_no_task;
	goto end;
    }
    
    task_inst = mmr_task_inst_find(task, task_inst_name);
    if (task_inst && task_inst->refcount <= 1) {
	mmr_log(log_level_debug, 
		"Task inst %s: %s stopping",
		task_name, task_inst_name);
	mmr_task_inst_stop(task_inst);
    }    

 end:
    mmr_api_unlock();
    return ret;
} 


/*
 *============================================================================
 *                        mmr_task_inst_destroy
 *============================================================================
 * Description:     Destroy a task instance. 
 * Parameters:    
 *     task_name, task_inst_name - the name of the task and the task instance
 * Returns:  
 */
mmr_err_t mmr_task_inst_destroy(char *task_name,
				char *task_inst_name) {
    mmr_err_t ret = mmr_err_ok;
    struct mmr_task_s *task;
    struct mmr_task_inst_s *task_inst;
    
    if (!task_name || !task_inst_name) {
	return mmr_err_invalid_param;
    }
    
    if (!g_mmr_cb) {
	return mmr_err_not_initialized;
    }
    
    mmr_api_lock();
    
    task = mmr_task_find(task_name);
    if (!task) {
	mmr_log(log_level_err, 
		"Could not find task inst %s: %s: task does not exist",
		task_name, task_inst_name);
	ret = mmr_err_no_task;
	goto end;
    }
    
    task_inst = mmr_task_inst_find(task, task_inst_name);
    if (task_inst) {
	mmr_log(log_level_debug, 
		"Task inst %s: %s destroyed, refcount %d",
		task_name, task_inst_name, task_inst->refcount - 1);
	mmr_task_inst_release(task_inst);
    }    

 end:
    mmr_api_unlock();
    return ret;
} 


/*
 *============================================================================
 *                        mmr_results_register
 *============================================================================
 * Description:     Install a results report callback
 * Parameters:    
 *     f - the callback
 * Returns:  
 */
mmr_err_t mmr_results_register(mmr_report_results_t *f) {
    mmr_err_t ret = mmr_err_ok;

    if (!g_mmr_cb) {
	return mmr_err_not_initialized;
    }
    
    bitd_mutex_lock(g_mmr_cb->results_lock);
    g_mmr_cb->report_results = f;
    bitd_mutex_unlock(g_mmr_cb->results_lock);

    return ret;
}    


/*
 *============================================================================
 *                        mmr_task_inst_report_results
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
void mmr_task_inst_report_results(mmr_task_inst_t task_inst,
				  mmr_task_inst_results_t *r) {
    bitd_boolean ret;

    mmr_log(log_level_trace, "%s: %s: Results",
	    task_inst->task->name,
	    task_inst->name);

    /* Get the result timestamp */
    task_inst->run_tstamp_ns = bitd_get_time_nsec();

    /* Trigger task instances that wait for these results */
    ret = mmr_schedule_triggers(task_inst, r);
    if (ret) {
	mmr_log(log_level_trace, "%s: %s: Results consumed by triggers",
		task_inst->task->name,
		task_inst->name);
	return;
    }

    bitd_mutex_lock(g_mmr_cb->results_lock);
    if (g_mmr_cb->report_results) {
	g_mmr_cb->report_results(task_inst->task->name,
				 task_inst->name,
				 task_inst->params.tags,
				 task_inst->run_id,
				 task_inst->run_tstamp_ns,
				 r);
    }
    bitd_mutex_unlock(g_mmr_cb->results_lock);

} 
