/*****************************************************************************
 *
 * Original Author: Andrei Radulescu-Banu
 * Creation Date:   
 * Description: 
 * 
 * Copyright 2018 by Andrei Radulescu-Banu.  All Rights Reserved.
 * Unauthorized reproduction, modification, distribution, transmission, 
 * republication, display or performance are strictly prohibited.
 ****************************************************************************/

#ifndef _BITD_PLATFORM_ARCH_THREAD_H_
#define _BITD_PLATFORM_ARCH_THREAD_H_

/*****************************************************************************
 *                                INCLUDE FILES 
 *****************************************************************************/

#include "bitd/platform-types.h"

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

/* Thread, mutex and event handles */
typedef struct bitd_arch_thread_s *bitd_arch_thread;
typedef struct bitd_arch_mutex_s *bitd_arch_mutex;
typedef struct bitd_arch_event_s *bitd_arch_event;
struct bitd_thread_s;

/* Thread entrypoint prototype */
typedef void (bitd_arch_thread_entrypoint_t)(void *thread_arg);



/*****************************************************************************
 *                            FUNCTION DEFINITIONS
 *****************************************************************************/

/* Init/deinit the thread subsystem */
bitd_boolean bitd_sys_arch_thread_init(void);
void bitd_sys_arch_thread_deinit(void);


/*
 * Thread API
 */

/* Create and start the thread */
bitd_arch_thread bitd_arch_create_thread(bitd_arch_thread_entrypoint_t *entry, 
					 bitd_int32 priority, 
					 bitd_uint32 stack_nbytes,
					 struct bitd_thread_s *th);

/* Wait for the thread to terminate.  Deletes the thread object. */
void bitd_arch_join_thread(bitd_arch_thread ath);

/* Get the current thread id */
bitd_arch_thread bitd_arch_get_current_thread(void);

/* Ask the user data of the passed thread. */
struct bitd_thread_s *bitd_arch_get_platform_thread(bitd_arch_thread ath);


/*
 * Mutex API
 */

/* Create/destroy mutex */
bitd_arch_mutex bitd_arch_mutex_create(void);
void bitd_arch_mutex_destroy(bitd_arch_mutex m);

/* Lock/unlock mutex. A thread may recursively lock 
   the mutex multiple times. */
void bitd_arch_mutex_lock(bitd_arch_mutex m);
void bitd_arch_mutex_unlock(bitd_arch_mutex m);

/*
 * Event API
 */

/* Create/destroy event */
bitd_arch_event bitd_arch_event_create(void);
void bitd_arch_event_destroy(bitd_arch_event e);

/* Set, clear and wait with timeout for event */
void bitd_arch_event_set(bitd_arch_event e);
void bitd_arch_event_clear(bitd_arch_event e);
bitd_boolean bitd_arch_event_wait(bitd_arch_event e, bitd_uint32 timeout);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _BITD_PLATFORM_ARCH_THREAD_H_ */

