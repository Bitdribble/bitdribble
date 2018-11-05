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
 *                        mmr_module_get
 *============================================================================
 * Description:     Get a module control block. Create it if 
 *    it does not exist. Bump up the refcount if it does exist.
 * Parameters:    
 *     module_name - the name of the module
 * Returns:  
 *     Pointer to the module control block.
 */
struct mmr_module_s *mmr_module_get(char *module_name,
				    mmr_err_t *err) {
    struct mmr_module_s *module;
    
    if (!module_name) {
	return NULL;
    }

    /* Initialize OUT parameter */
    if (err) {
	*err = mmr_err_ok;
    }

    for (module = g_mmr_cb->module_head; 
	 module != MMR_MODULE_HEAD(g_mmr_cb); 
	 module = module->next) {
	
	if (!strcmp(module->name, module_name)) {
	    /* Increment the reference count */
	    module->refcount++;
	    return module;
	}
    }

    /* Module does not exist. We must create it. */

    /* Create module */
    module = calloc(1, sizeof(*module));

    /* Configure the module */
    module->name = strdup(module_name);
    module->task_head = MODULE_TASK_HEAD(module);
    module->task_tail = MODULE_TASK_HEAD(module);

    /* Chain the module at the end of the list */
    module->prev = g_mmr_cb->module_tail;
    module->next = g_mmr_cb->module_tail->next;
    module->prev->next = module;
    module->next->prev = module;

    module->refcount = 1;

    bitd_assert(!module->dll_name);
    bitd_assert(!module->dll_full_name);

    /* Build up the platform-specific dll name */
    module->dll_name = bitd_get_dll_name(module_name);
    module->dll_full_name = bitd_envpath_find(g_mmr_cb->module_path, 
					    module->dll_name, 
					    S_IREAD);
    if (!module->dll_full_name) {
	mmr_log(log_level_err, "Could not find dll %s in path %s",
		module->dll_name, 
		g_mmr_cb->module_path ? g_mmr_cb->module_path : "NULL");
	if (err) {
	    *err = mmr_err_module_not_found;
	}
	goto error;
    }

    /* Load the dll */
    module->dll_hdl = bitd_dll_load(module->dll_full_name);
    if (!module->dll_hdl) {
	char *buf;
	int bufsize = 256;

	buf = malloc(bufsize);
	mmr_log(log_level_err, "Could not load %s: %s.",
		module->dll_full_name, bitd_dll_error(buf, bufsize));

	if (err) {
	    *err = mmr_err_module_load_failed;
	}
	free(buf);
	goto error;
    }

    /* Look up the module entrypoint */
    module->load = bitd_dll_sym(module->dll_hdl, "bitd_module_load");
    if (!module->load) {
	mmr_log(log_level_err, "Could not find bitd_module_load symbol in %s.",
		module->dll_full_name);
	if (err) {
	    *err = mmr_err_module_invalid_symbol;
	}
	goto error;
    }

    /* Look up the module exitpoint */
    module->unload = bitd_dll_sym(module->dll_hdl, "bitd_module_unload");
    if (!module->unload) {
	mmr_log(log_level_err, "Could not find bitd_module_unload symbol in %s.",
		module->dll_full_name);
	if (err) {
	    *err = mmr_err_module_invalid_symbol;
	}
	goto error;
    }

    /* Find the directory containig the dll */
    module->dll_dir = bitd_get_path_dir(module->dll_full_name);

    /* Call the module entrypoint */
    if (!module->load(module, g_mmr_cb->tags, module->dll_dir)) {
	mmr_log(log_level_err, "bitd_module_load() returns FALSE in %s.",
		module->dll_full_name);
	if (err) {
	    *err = mmr_err_module_load_failed;
	}
	goto error;
    }

    return module;

 error:    
    mmr_module_release(module);
    return NULL;
} 


/*
 *============================================================================
 *                        mmr_module_find
 *============================================================================
 * Description:     Get a module control block. Don't create it if 
 *    it does not exist. 
 * Parameters:    
 *     module_name - the name of the module
 * Returns:  
 *     Pointer to the module control block.
 */
struct mmr_module_s *mmr_module_find(char *module_name) {
    struct mmr_module_s *module;

    if (!module_name) {
	return NULL;
    }

    for (module = g_mmr_cb->module_head; 
	 module != MMR_MODULE_HEAD(g_mmr_cb); 
	 module = module->next) {
	
	if (!strcmp(module->name, module_name)) {
	    return module;
	}
    }

    return NULL;
} 


/*
 *============================================================================
 *                        mmr_module_stop
 *============================================================================
 * Description:     Stop a module and all its tasks and task instances
 * Parameters:    
 *     module - the module we are stopping
 * Returns:  
 */
void mmr_module_stop(struct mmr_module_s *module) {
    struct mmr_task_s *task;

    if (module->stopping_p) {
	/* Already stopping */
	return;
    }

    /* Set the stop flag */
    module->stopping_p = TRUE;

    /* Stop all tasks of this module */
    for (task = module->task_head;
	 task != MODULE_TASK_HEAD(module);
	 task = task->next) {	
	mmr_task_stop(task);
    }
} 


/*
 *============================================================================
 *                        mmr_module_is_stopped
 *============================================================================
 * Description:     Check if a module and all its tasks and 
 *     task instance are stopped.
 * Parameters:    
 *     module - the module control block
 * Returns:  
 */
bitd_boolean mmr_module_is_stopped(struct mmr_module_s *module) {
    struct mmr_task_s *task;

    /* Check that this module is stopping */
    if (!module->stopping_p) {
	return FALSE;
    }

    /* Check that all tasks of this module have stopped */
    for (task = module->task_head;
	 task != MODULE_TASK_HEAD(module);
	 task = task->next) {	
	if (!mmr_task_is_stopped(task)) {
	    return FALSE;
	}
    }

    return TRUE;
} 


/*
 *============================================================================
 *                        mmr_module_release
 *============================================================================
 * Description:     Release a module given its control block pointer
 * Parameters:    
 * Returns:  
 */
void mmr_module_release(struct mmr_module_s *module) {

    if (!module) {
	return;
    }

    /* Bump down the reference count */
    module->refcount--;
    bitd_assert(module->refcount >= 0);
    
    /* When reference count reaches zero, release the group */
    if (!module->refcount) {
	
	/* Stop this module and all its tasks and task instances */
	mmr_module_stop(module);

	/* Wait for the module to be stopped */
	while (!mmr_module_is_stopped(module)) {
	    bitd_mutex_unlock(g_mmr_cb->lock);
	    bitd_event_wait(g_mmr_cb->task_inst_stopped_ev, 100);
	    bitd_mutex_lock(g_mmr_cb->lock);
	}

	if (module->unload) {
	    /* Call the unload symbol */
	    module->unload();
	}

	/* Free all the tasks in this module */
	while (module->task_head != MODULE_TASK_HEAD(module)) {
	    mmr_task_release(module->task_head);
	}
	
	/* Unload the dll */
	bitd_dll_unload(module->dll_hdl);
	
	/* Unchain the module */
	module->next->prev = module->prev;
	module->prev->next = module->next;
	
	if (module->name) {
	    free(module->name);
	}
	if (module->dll_name) {
	    free(module->dll_name);
	}
	if (module->dll_full_name) {
	    free(module->dll_full_name);
	}
	if (module->dll_dir) {
	    free(module->dll_dir);
	}
	free(module);
    }
}
