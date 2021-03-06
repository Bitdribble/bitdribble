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

/* The module control block */
struct module_s {
} g_module;

struct bitd_task_inst_s {
    char *task_name;
    char *task_inst_name;
    mmr_task_inst_t mmr_task_inst_hdl;
    bitd_nvp_t args;     /* Task instance arguments at creation */
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

    s_log_keyid = ttlog_register("bitd-assert");

    ttlog(log_level_trace, s_log_keyid,
	  "%s:%d:%s() called", __FILE__, __LINE__, __FUNCTION__);

    ttlog(log_level_trace, s_log_keyid,
	  "Module dir: %s", module_dir);

    /* Initialize the task API structure to zero, in case we're not 
       implementing some APIs */
    memset(&task_api, 0, sizeof(task_api));

    /* Set up the task API structure */
    task_api.task_inst_create = task_inst_create;
    task_api.task_inst_destroy = task_inst_destroy;
    task_api.task_inst_run = task_inst_run;
    task_api.task_inst_kill = task_inst_kill;

    /* Register the task */
    s_task = mmr_task_register(mmr_module, "assert", &task_api);

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

    /* Save the args */
    p->args = bitd_nvp_clone(args);

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

    bitd_nvp_free(p->args);
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
    bitd_nvp_t input_nvp, merged_nvp;
    mmr_task_inst_results_t results;
    char *buf;

    ttlog(log_level_trace, s_log_keyid,
	  "%s: %s() called", p->task_inst_name, __FUNCTION__);

    if (input->type != bitd_type_nvp) {
	ttlog(log_level_trace, s_log_keyid,
	      "%s: Input not of nvp type, exiting", p->task_inst_name);
	return 0;
    }

    buf = bitd_object_to_string(input);
    ttlog(log_level_trace, s_log_keyid,
	  "%s: Input:\n%s", p->task_inst_name, buf);
    free(buf);

    input_nvp = input->v.value_nvp;
    
    /* Is the input a superset of the args? */
    merged_nvp = bitd_nvp_merge(input_nvp, p->args, NULL);
    if (bitd_nvp_compare(merged_nvp, input_nvp)) {
	/* Args are not subset of input. */
	memset(&results, 0, sizeof(results));
	results.exit_code = -1;
	mmr_task_inst_report_results(p->mmr_task_inst_hdl, &results);
    }
    bitd_nvp_free(merged_nvp);

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
} 
