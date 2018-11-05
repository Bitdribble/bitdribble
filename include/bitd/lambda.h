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

#ifndef _BITD_LAMBDA_H_
#define _BITD_LAMBDA_H_

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

/* Handle passed during function execution */
typedef struct bitd_lambda *bitd_lambda_handle;

/* The prototype of the task function to be executed on a worker thread.
   This function can block. If stopping_p is set, function should exit
   immediately. */
typedef void (bitd_lambda_task_func_t)(void *cookie, bitd_boolean *stopping_p);

/*****************************************************************************
 *                            FUNCTION DEFINITIONS
 *****************************************************************************/

/* Create/destroy the worker thread pool */
bitd_lambda_handle bitd_lambda_init(char *pool_name);
void bitd_lambda_deinit(bitd_lambda_handle lambda);

/* Set the max thread limit. A limit of zero means no limit. */
void bitd_lambda_set_thread_max(bitd_lambda_handle lambda, int thread_max);
    
/* Thread will exit if idle for longer than tmo_msec.
   The default idle timeout is 300,000 msecs (= 5 minutes). */
void bitd_lambda_set_thread_idle_tmo(bitd_lambda_handle lambda, 
				     bitd_uint32 tmo_msec);
    
/* Set the max task limit. A limit of zero means no limit, */
void bitd_lambda_set_task_max(bitd_lambda_handle lambda, int task_max);
    
/* Execute the passed-in routine in the worker thread pool */
bitd_boolean bitd_lambda_exec_task(bitd_lambda_handle lambda,
				   bitd_lambda_task_func_t f,
				   void *cookie);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _BITD_LAMBDA_H_ */
