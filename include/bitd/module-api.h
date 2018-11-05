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

#ifndef _BITD_MODULE_H_
#define _BITD_MODULE_H_

/*****************************************************************************
 *                                INCLUDE FILES 
 *****************************************************************************/
#include "bitd/common.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

/*****************************************************************************
 *                             MANIFEST CONSTANTS
 *****************************************************************************/



/*****************************************************************************
 *                                  MACROS 
 *****************************************************************************/



/*****************************************************************************
 *                                  TYPES
 *****************************************************************************/

/* Data types and APIs implemented by modules begin with bitd_, and data types
   and APIs implemented by the module manager begin with mmr_. */
typedef struct mmr_module_s *mmr_module_t;
typedef struct mmr_task_s *mmr_task_t;
typedef struct mmr_task_inst_s *mmr_task_inst_t;
typedef struct bitd_task_inst_s *bitd_task_inst_t;

/* Task instance parameters */
typedef struct {
    bitd_nvp_t args;     /* Task instance arguments at creation */
    bitd_nvp_t tags;     /* Task instance tags (or environment) at creation */
    bitd_object_t input; /* Task instance stdin at run start */
} mmr_task_inst_params_t;

/* Task instance results */
typedef struct {
    bitd_object_t output;  /* Task instance stdout */
    bitd_object_t error;   /* Task instance stderr */
    int exit_code;       /* Task instance exit code */
} mmr_task_inst_results_t;

/* The task API */
typedef bitd_task_inst_t (bitd_task_inst_create_t)(char *task_name,
					       char *task_inst_name,
					       mmr_task_inst_t mmr_task_inst_hdl,
					       bitd_nvp_t args,
					       bitd_nvp_t tags);
typedef void (bitd_task_inst_update_t)(bitd_task_inst_t task_inst,
				     bitd_nvp_t args,
				     bitd_nvp_t tags);
typedef void (bitd_task_inst_destroy_t)(bitd_task_inst_t task_inst);
typedef int (bitd_task_inst_run_t)(bitd_task_inst_t task_inst, 
				 bitd_object_t *input);
typedef void (bitd_task_inst_kill_t)(bitd_task_inst_t task_inst, int signo);
#define BITD_TASK_SIGSTOP 1

/* Structure with the task APIs passed to module-mgr by module on load */
typedef struct {
    bitd_task_inst_create_t *task_inst_create;
    bitd_task_inst_update_t *task_inst_update;
    bitd_task_inst_destroy_t *task_inst_destroy;
    bitd_task_inst_run_t *task_inst_run;
    bitd_task_inst_kill_t *task_inst_kill;
} bitd_task_api_t;


/*****************************************************************************
 *                            FUNCTION DEFINITIONS
 *****************************************************************************/

/* Load and unload a module */
bitd_boolean bitd_module_load(mmr_module_t mmr_module,
			  bitd_nvp_t tags,
			  char *module_dir);
void bitd_module_unload(void);

/* Register and unregister a task. Typically called from 
   bitd_module_load/unload(). */
extern mmr_task_t mmr_task_register(mmr_module_t mmr_module,
				    char *task_name,
				    bitd_task_api_t *task_api);
extern void mmr_task_unregister(mmr_task_t task);

/* Report task instance results. Typically called from task_inst_run(). */
extern void mmr_task_inst_report_results(mmr_task_inst_t mmr_task_inst,
					 mmr_task_inst_results_t *results);

/* Task inst params utilities. The params struct is not assumed to be 
   heap-allocated */
extern void mmr_task_inst_params_init(mmr_task_inst_params_t *p);
extern void mmr_task_inst_params_deinit(mmr_task_inst_params_t *p);
extern void mmr_task_inst_params_clone(mmr_task_inst_params_t *p_out, 
				       mmr_task_inst_params_t *p);
extern int mmr_task_inst_params_compare(mmr_task_inst_params_t *p1,
					mmr_task_inst_params_t *p2);

/* Task inst results utilities. The results struct is not assumed to be 
   heap-allocated */
extern void mmr_task_inst_results_init(mmr_task_inst_results_t *r);
extern void mmr_task_inst_results_deinit(mmr_task_inst_results_t *p);
extern void mmr_task_inst_results_clone(mmr_task_inst_results_t *p_out,
					mmr_task_inst_results_t *p);
extern int mmr_task_inst_results_compare(mmr_task_inst_results_t *p1,
					 mmr_task_inst_results_t *p2);

extern bitd_nvp_t mmr_get_raw_results(mmr_task_inst_t mmr_task_inst,
				      mmr_task_inst_results_t *results);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _BITD_MODULE_H_ */
