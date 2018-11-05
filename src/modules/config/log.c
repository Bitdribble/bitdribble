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
#include "bitd/common.h"
#include "bitd/types.h"
#include "bitd/log.h"
#include "bitd/module-api.h"


/*****************************************************************************
 *                             MANIFEST CONSTANTS
 *****************************************************************************/



/*****************************************************************************
 *                                  MACROS 
 *****************************************************************************/



/*****************************************************************************
 *                                  TYPES
 *****************************************************************************/
static bitd_task_inst_create_t task_inst_create;
static bitd_task_inst_destroy_t task_inst_destroy;
static bitd_task_inst_run_t task_inst_run;
static bitd_task_inst_kill_t task_inst_kill;
static ttlog_keyid s_log_keyid;

struct bitd_task_inst_s {
    char *task_name;
    char *task_inst_name;
    mmr_task_inst_t mmr_task_inst_hdl;
    bitd_event stop_ev;  /* The stop event */
    bitd_boolean stopped_p;
};

/*****************************************************************************
 *                           FUNCTION DECLARATION
 *****************************************************************************/



/*****************************************************************************
 *                                VARIABLES
 *****************************************************************************/
static mmr_task_t s_task;


/*****************************************************************************
 *                          FUNCTION IMPLEMENTATION
 *****************************************************************************/


/*
 *============================================================================
 *                        bitd_module_load
 *============================================================================
 * Description:     Load the module.
 * Parameters:    
 *     tags - the module tags
 * 
 * Returns:      TRUE on success
 */
bitd_boolean bitd_module_load(mmr_module_t mmr_module,
			  bitd_nvp_t tags,
			  char *module_dir) {
    bitd_task_api_t task_api;  /* The task image */

    s_log_keyid = ttlog_register("bitd-config-log");

    ttlog(log_level_trace, s_log_keyid,
	  "%s:%d:%s() called", __FILE__, __LINE__, __FUNCTION__);

    /* Initialize the task API structure to zero, in case we're not 
       implementing some APIs */
    memset(&task_api, 0, sizeof(task_api));

    /* Set up the task API structure */
    task_api.task_inst_create = task_inst_create;
    task_api.task_inst_destroy = task_inst_destroy;
    task_api.task_inst_run = task_inst_run;
    task_api.task_inst_kill = task_inst_kill;

    /* Register the task */
    s_task = mmr_task_register(mmr_module, "config-log", &task_api);

    return TRUE;
} 


/*
 *============================================================================
 *                        bitd_module_unload
 *============================================================================
 * Description:     Unload the module, freeing all allocated resources
 * Parameters:    
 * Returns:      
 */
void bitd_module_unload(void) {    
    ttlog(log_level_trace, s_log_keyid,
	  "%s:%d:%s() called", __FILE__, __LINE__, __FUNCTION__);

    /* Unregister the task */
    mmr_task_unregister(s_task);

    ttlog_unregister(s_log_keyid);
} 


/*
 *============================================================================
 *                        task_inst_create
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
bitd_task_inst_t task_inst_create(char *task_name,
				  char *task_inst_name,
				  mmr_task_inst_t mmr_task_inst_hdl, 
				  bitd_nvp_t args,
				  bitd_nvp_t tags) {
    bitd_task_inst_t p;

    ttlog(log_level_trace, s_log_keyid,
	  "%s: %s() called", task_inst_name, __FUNCTION__);

    p = calloc(1, sizeof(*p));
    
    p->task_name = task_name;
    p->task_inst_name = task_inst_name;
    p->mmr_task_inst_hdl = mmr_task_inst_hdl;
    p->stop_ev = bitd_event_create(0);

    return p;
} 


/*
 *============================================================================
 *                        task_inst_destroy
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
void task_inst_destroy(bitd_task_inst_t p) {

    ttlog(log_level_trace, s_log_keyid,
	  "%s: %s() called", p->task_inst_name, __FUNCTION__);

    bitd_event_destroy(p->stop_ev);
    free(p);
} 


/*
 *============================================================================
 *                        task_inst_run
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
int task_inst_run(bitd_task_inst_t p, bitd_object_t *input) {
    int idx, i, j;
    bitd_nvp_t nvp, nvp1, nvp_keys;
    ttlog_level level = log_level_none, level_old;
    int n_keys;
    char **keys_old, *key_name;
    char *elem_names = "log-key";
    bitd_boolean found_p;

    ttlog(log_level_trace, s_log_keyid,
	  "%s: %s() called", p->task_inst_name, __FUNCTION__);

    if (input->type != bitd_type_nvp) {
	ttlog(log_level_err, s_log_keyid,
	      "%s: Invalid input type %s (should be nvp)", 
	      p->task_inst_name, bitd_get_type_name(input->type));
	return -1;
    }

    nvp = input->v.value_nvp;
    if (bitd_nvp_lookup_elem(nvp, "log-level", &idx)) {
	if (nvp->e[idx].type != bitd_type_string) {
	    ttlog(log_level_err, s_log_keyid,
		  "%s: Invalid input: log-level not of type string", 
		  p->task_inst_name);
	    return -1;
	}

	/* Get the desired log level */
	level = ttlog_get_level_from_name(nvp->e[idx].v.value_string);
	if (!level && (nvp->e[idx].v.value_string &&
		       strcasecmp(nvp->e[idx].v.value_string, "none"))) {
	    ttlog(log_level_err, s_log_keyid,
		  "%s: Invalid log level %s.", 
		  p->task_inst_name, nvp->e[idx].v.value_string);
	    return -1;
	}
    }

    /* Get the actual log level */
    level_old = ttlog_get_level(NULL);

    /* Change it if different */
    if (level != level_old) {
	ttlog_set_level(NULL, level);
	ttlog_raw("%s: Changed log level from %s to %s",
		  p->task_inst_name, 
		  ttlog_get_level_name(level_old),
		  ttlog_get_level_name(level));
    }

    /* Get the new keys */
    nvp_keys = bitd_nvp_trim_bytype(nvp, &elem_names, 1, bitd_type_nvp);

    /* Get listing of the keys */
    ttlog_get_keys(&keys_old, &n_keys);

    /* All new keys should have the correct level */
    if (nvp_keys) {
	for (i = 0; i < nvp_keys->n_elts; i++) {
	    if (nvp_keys->e[i].type != bitd_type_nvp) {
		ttlog(log_level_err, s_log_keyid,
		      "%s: Invalid input: log-key not of type nvp", 
		      p->task_inst_name);
		continue;
	    }
	    nvp1 = nvp_keys->e[i].v.value_nvp;
	    if (!bitd_nvp_lookup_elem(nvp1, "key-name", &idx) ||
		nvp1->e[idx].type != bitd_type_string) {
		ttlog(log_level_err, s_log_keyid,
		      "%s: Invalid input: log-key.key-name not of type string", 
		      p->task_inst_name);
		continue;
	    }
	    key_name = nvp1->e[idx].v.value_string;
	    if (!key_name) {
		ttlog(log_level_err, s_log_keyid,
		      "%s: Invalid input: log-key.key-name should not be NULL", 
		      p->task_inst_name);
		continue;
	    }
	    if (!bitd_nvp_lookup_elem(nvp1, "log-level", &idx) ||
		nvp1->e[idx].type != bitd_type_string) {
		ttlog(log_level_err, s_log_keyid,
		      "%s: Invalid input: log-key.log-level missing or of non-string type", 
		      p->task_inst_name);
		continue;
	    }
	    level = ttlog_get_level_from_name(nvp1->e[idx].v.value_string);
	    if (!level) {
		if (nvp1->e[idx].v.value_string && 
		    strcasecmp(nvp1->e[idx].v.value_string, "none")) {
		    ttlog(log_level_err, s_log_keyid,
			  "%s: Invalid input: log-key.log-level '%s' is invalid", 
			  p->task_inst_name, nvp1->e[idx].v.value_string);
		    continue;
		}
	    }
	    
	    for (j = 0; j < n_keys; j++) {
		if (!strcmp(keys_old[j], key_name)) {
		    break;
		}
	    }
	    
	    if (j < n_keys) {
		/* Key is found */
		level_old = ttlog_get_level(key_name);
		
		/* Change it if different */
		if (level != level_old) {
		    ttlog_set_level(key_name, level);
		    ttlog_raw("%s: Changed %s log level from %s to %s",
			      p->task_inst_name, 
			      key_name,
			      ttlog_get_level_name(level_old),
			      ttlog_get_level_name(level));
		}	    
	    } else {
		/* Key is not found - set it */
		ttlog_set_level(key_name, level);
		ttlog_raw("%s: Set %s log level to %s",
			  p->task_inst_name, 
			  key_name,
			  ttlog_get_level_name(level));
	    }
	}
    }

    /* No old keys should not have a corresponding new key */
    for (j = 0; j < n_keys; j++) {

	found_p = FALSE;
	if (nvp_keys) {
	    for (i = 0; i < nvp_keys->n_elts; i++) {
		nvp1 = nvp_keys->e[i].v.value_nvp;
		if (bitd_nvp_lookup_elem(nvp1, "key-name", &idx) &&
		    nvp1->e[idx].type == bitd_type_string) {
		    key_name = nvp1->e[idx].v.value_string;
		    if (key_name && !strcmp(key_name, keys_old[j])) {
			found_p  = TRUE;
			break;
		    }
		}
	    }
	}
	
	if (!found_p) {
	    level_old = ttlog_get_level(keys_old[j]);
	    if (level_old != log_level_none) {
		ttlog_set_level(keys_old[j], log_level_none);
		ttlog_raw("%s: Changed %s log level from %s to %s",
			  p->task_inst_name, 
			  keys_old[j],
			  ttlog_get_level_name(level_old),
			  ttlog_get_level_name(log_level_none));
	    }
	}
    }

    /* Release memory */
    bitd_nvp_free(nvp_keys);
    ttlog_free_keys(keys_old, n_keys);

    return 0;
} 


/*
 *============================================================================
 *                        task_inst_kill
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
void task_inst_kill(bitd_task_inst_t p, int signo) {

    ttlog(log_level_trace, s_log_keyid,
	  "%s:%d:%s() called", __FILE__, __LINE__, __FUNCTION__);

    p->stopped_p = TRUE;
    bitd_event_set(p->stop_ev);
} 
