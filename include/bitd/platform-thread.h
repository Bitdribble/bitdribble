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

#ifndef _BITD_PLATFORM_THREAD_H_
#define _BITD_PLATFORM_THREAD_H_

/*****************************************************************************
 *                                INCLUDE FILES 
 *****************************************************************************/

#include "bitd/arch/arch-thread.h"
#include "bitd/platform-socket.h"

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
typedef struct bitd_thread_s *bitd_thread;
typedef struct bitd_mutex_s *bitd_mutex;
typedef struct bitd_event_s *bitd_event;

/* Thread entrypoint prototype */
typedef void (bitd_thread_entrypoint_t)(void *thread_arg);



/*****************************************************************************
 *                            FUNCTION DEFINITIONS
 *****************************************************************************/

/* Init/deinit the thread subsystem */
bitd_boolean bitd_sys_thread_init(void);
void bitd_sys_thread_deinit(void);


/*
 * Thread API
 */

#define BITD_DEFAULT_STACK_SIZE 8192
#define BITD_DEFAULT_PRIORITY   0

/* Create and start the thread */
bitd_thread bitd_create_thread(const char* name,
			       bitd_thread_entrypoint_t *entry, 
			       bitd_int32 priority, bitd_uint32 stack_nbytes,
			       void *thread_arg);

/* Wait for the thread to terminate.  Deletes the thread object. */
void bitd_join_thread(bitd_thread th);

/* Get the current thread id */
bitd_thread bitd_get_current_thread(void);

/* Get the name of the passed thread */
const char *bitd_get_thread_name(bitd_thread th);

/* Get the thread argument (called from arch-thread layer) */
void *bitd_get_thread_arg(bitd_thread th);


/*
 * Mutex API
 */

/* Create/destroy mutex */
bitd_mutex bitd_mutex_create(void);
void bitd_mutex_destroy(bitd_mutex m);

/* Lock/unlock mutex. A thread may recursively lock 
   the mutex multiple times. */
void bitd_mutex_lock(bitd_mutex m);
void bitd_mutex_unlock(bitd_mutex m);

/*
 * Event API
 */

/* Set this flag to create a pollable event */
#define BITD_EVENT_FLAG_POLL 0x1

/* 
 * Create event. Flags may be set to  for a normal event,
 * or BITD_EVENT_FLAG_POLL for a pollable event - in which case
 * the bitd_event_fd() api may be called to obtain the pollable
 * event file descriptor
 */
bitd_event bitd_event_create(bitd_uint32 flags);

/* Destroy event */
void bitd_event_destroy(bitd_event e);

/* Set, clear and wait with timeout for event */
void bitd_event_set(bitd_event e);
void bitd_event_clear(bitd_event e);
bitd_boolean bitd_event_wait(bitd_event e, bitd_uint32 timeout);

/* Return the pollable vent file descriptor, if so equipped */
bitd_socket_t bitd_event_to_fd(bitd_event e);

/*
 * Other APIs
 */
void bitd_sleep(bitd_uint32 msec);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _BITD_PLATFORM_THREAD_H_ */

