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
 *                        mmr_task_inst_params_init
 *============================================================================
 * Description:     Initialize the task inst parameters structure
 * Parameters:    
 * Returns:  
 */
void mmr_task_inst_params_init(mmr_task_inst_params_t *p) {
    memset(p, 0, sizeof(*p));
} 


/*
 *============================================================================
 *                        mmr_task_inst_params_deinit
 *============================================================================
 * Description:     Deinitialize the task inst parameters
 * Parameters:    
 * Returns:  
 */
void mmr_task_inst_params_deinit(mmr_task_inst_params_t *p) {

    if (p) {
	bitd_nvp_free(p->args);
	bitd_object_free(&p->input);
	bitd_nvp_free(p->tags);
	/* Don't free 'p', we're not assuming it is heap allocated */
    }
} 


/*
 *============================================================================
 *                        mmr_task_inst_params_clone
 *============================================================================
 * Description:     Copy a task inst params structure
 * Parameters:    
 * Returns:  
 */
void mmr_task_inst_params_clone(mmr_task_inst_params_t *p_out,
				mmr_task_inst_params_t *p) {
    if (!p || !p_out) {
	return;
    }

    p_out->args = bitd_nvp_clone(p->args);
    bitd_object_clone(&p_out->input, &p->input);
    p_out->tags = bitd_nvp_clone(p->tags);
} 


/*
 *============================================================================
 *                        mmr_task_inst_params_compare
 *============================================================================
 * Description:     Compare the parameters
 * Parameters:    
 * Returns:  
 *     Zero if p1 is same as p2, >0 if p1 greater than p2, <0 otherwise
 */
int mmr_task_inst_params_compare(mmr_task_inst_params_t *p1,
				 mmr_task_inst_params_t *p2) {
    int ret;

    if (!p1) {
	if (!p2) {
	    return 0;
	}
	return -1;
    } else if (!p2) {
	return 1;
    }

    ret = bitd_nvp_compare(p1->args, p2->args);
    if (ret) {
	return ret;
    }

    ret = bitd_object_compare(&p1->input, &p2->input);
    if (ret) {
	return ret;
    }

    ret = bitd_nvp_compare(p1->tags, p2->tags);
    return ret;
}


/*
 *============================================================================
 *                        mmr_task_inst_results_init
 *============================================================================
 * Description:     Initialize the task inst results structure
 * Parameters:    
 * Returns:  
 */
void mmr_task_inst_results_init(mmr_task_inst_results_t *r) {

    memset(r, 0, sizeof(*r));
} 


/*
 *============================================================================
 *                        mmr_task_inst_results_free
 *============================================================================
 * Description:     Deinitialize the task inst results structure
 * Parameters:    
 * Returns:  
 */
void mmr_task_inst_results_free(mmr_task_inst_results_t *r) {

    if (r) {
	bitd_object_free(&r->output);
	bitd_object_free(&r->error);
	/* Don't free 'r', we're not assuming it is heap allocated */
    }
} 


/*
 *============================================================================
 *                        mmr_task_inst_results_clone
 *============================================================================
 * Description:     Copy a task inst results structure
 * Parameters:    
 * Returns:  
 */
void mmr_task_inst_results_clone(mmr_task_inst_results_t *r_out,
				 mmr_task_inst_results_t *r) {
    if (!r || !r_out) {
	return;
    }

    bitd_object_clone(&r_out->output, &r->output);
    bitd_object_clone(&r_out->error, &r->error);
    r_out->exit_code = r->exit_code;
} 


/*
 *============================================================================
 *                        mmr_task_inst_results_compare
 *============================================================================
 * Description:     Compare the parameters
 * Parameters:    
 * Returns:  
 *     Zero if r1 is same as r2, >0 if r1 greater than r2, <0 otherwise
 */
int mmr_task_inst_results_compare(mmr_task_inst_results_t *r1,
				  mmr_task_inst_results_t *r2) {
    int ret;

    if (!r1) {
	if (!r2) {
	    return 0;
	}
	return -1;
    } else if (!r2) {
	return 1;
    }

    ret = bitd_object_compare(&r1->output, &r2->output);
    if (ret) {
	return ret;
    }

    ret = bitd_object_compare(&r1->error, &r2->error);
    if (ret) {
	return ret;
    }

    if (r1->exit_code < r2->exit_code) {
	return -1;
    } else if (r1->exit_code > r2->exit_code) {
	return 1;
    }

    return 0;
}


/*
 *============================================================================
 *                        mmr_get_raw_results
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
bitd_nvp_t mmr_get_raw_results(mmr_task_inst_t task_inst,
			     mmr_task_inst_results_t *r) {
    bitd_nvp_t nvp;

    nvp = bitd_nvp_alloc(10);
    
    /* Copy the tags */
    nvp->e[nvp->n_elts].name = strdup("tags");
    nvp->e[nvp->n_elts].v.value_nvp = bitd_nvp_clone(task_inst->params.tags);
    nvp->e[nvp->n_elts++].type = bitd_type_nvp;

    /* Copy the run id */
    nvp->e[nvp->n_elts].name = strdup("run-id");
    nvp->e[nvp->n_elts].v.value_uint64 = task_inst->run_id;
    nvp->e[nvp->n_elts++].type = bitd_type_uint64;

    /* Copy the timestamp */
    nvp->e[nvp->n_elts].name = strdup("run-timestamp");
    nvp->e[nvp->n_elts].v.value_uint64 = task_inst->run_tstamp_ns;
    nvp->e[nvp->n_elts++].type = bitd_type_uint64;

    /* Exit code */
    nvp->e[nvp->n_elts].name = strdup("exit-code");
    nvp->e[nvp->n_elts].v.value_int64 = r->exit_code;
    nvp->e[nvp->n_elts++].type = bitd_type_int64;

    if (r->output.type != bitd_type_void) {
	/* Output */
	nvp->e[nvp->n_elts].name = strdup("output");
	bitd_value_clone(&nvp->e[nvp->n_elts].v, &r->output.v, r->output.type);
	nvp->e[nvp->n_elts++].type = r->output.type;
    }

    if (r->error.type != bitd_type_void) {
	/* Error */
	nvp->e[nvp->n_elts].name = strdup("error");
	bitd_value_clone(&nvp->e[nvp->n_elts].v, &r->error.v, r->error.type);
	nvp->e[nvp->n_elts++].type = r->error.type;
    }

    return nvp;
} 
