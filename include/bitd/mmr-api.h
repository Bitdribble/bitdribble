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

#ifndef _BITD_MODULE_MGR_API_H_
#define _BITD_MODULE_MGR_API_H_

/*****************************************************************************
 *                                INCLUDE FILES 
 *****************************************************************************/
#include "bitd/common.h"
#include "bitd/types.h"
#include "bitd/log.h"
#include "bitd/module-api.h"


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
typedef enum {
    mmr_err_ok = 0,
    mmr_err_pending = 1,
    mmr_err_not_initialized = 2,
    mmr_err_invalid_param = 3,
    mmr_err_module_not_found = 4,
    mmr_err_module_invalid_symbol = 5,
    mmr_err_module_load_failed = 6,
    mmr_err_no_task = 7,
    mmr_err_task_create_failed = 8,
    mmr_err_task_inst_create_failed = 9,
    mmr_err_unknown = 10,
    
    mmr_err_max /* sentinel */
} mmr_err_t;

/*****************************************************************************
 *                            FUNCTION DEFINITIONS
 *****************************************************************************/

/* Module manager init/deinit */
mmr_err_t mmr_init();
void mmr_deinit();

/* Set/get configuration. Config must be set after init. */
mmr_err_t mmr_set_config(bitd_nvp_t config_nvp);
mmr_err_t mmr_get_config(bitd_nvp_t *config_nvp);

typedef int (mmr_vlog_func_t)(ttlog_level level, char *format_string, 
			      va_list args);

/* Set a log function. Must be called after init. */
mmr_err_t mmr_set_vlog_func(mmr_vlog_func_t *f);

/* Set/get the global tags. The tags must be set after init. */
mmr_err_t mmr_set_tags(bitd_nvp_t tags);
mmr_err_t mmr_get_tags(bitd_nvp_t *tags);

/* Set the module dll path */
mmr_err_t mmr_set_module_path(char *path);

/* Load and unload a module. Module load operation is reference counted. */
mmr_err_t mmr_load_module(char *module_name);
mmr_err_t mmr_unload_module(char *module_name);

/* Create and destroy a task instance */
mmr_err_t mmr_task_inst_create(char *task_name,
			       char *task_inst_name,
			       bitd_nvp_t schedule,
			       mmr_task_inst_params_t *params);
mmr_err_t mmr_task_inst_prepare_destroy(char *task_name,
					char *task_inst_name);
mmr_err_t mmr_task_inst_destroy(char *task_name,
				char *task_inst_name);

typedef void (mmr_report_results_t)(char *task_name,
				    char *task_inst_name,
				    bitd_nvp_t tags,
				    bitd_uint64 run_id,
				    bitd_uint64 tstamp_ns,
				    mmr_task_inst_results_t *r);

/* Register a task instance results hook */
mmr_err_t mmr_results_register(mmr_report_results_t *f);

/* Convert error names to strings */
char *mmr_get_error_name(mmr_err_t err);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _BITD_MODULE_MGR_API_H_ */
