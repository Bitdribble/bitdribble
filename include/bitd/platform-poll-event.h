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

#ifndef _BITD_PLATFORM_POLL_EVENT_H_
#define _BITD_PLATFORM_POLL_EVENT_H_

/*****************************************************************************
 *                                INCLUDE FILES
 *****************************************************************************/
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

/* Pollable event handle */
typedef struct bitd_poll_event_s *bitd_poll_event;



/*****************************************************************************
 *                            FUNCTION DEFINITIONS
 *****************************************************************************/


/*
 * Poll event API
 */

/* Create/destroy event */
bitd_poll_event bitd_poll_event_create(void);
void bitd_poll_event_destroy(bitd_poll_event e);

/* Set, clear and wait with timeout for poll event */
void bitd_poll_event_set(bitd_poll_event e);
void bitd_poll_event_clear(bitd_poll_event e);
bitd_boolean bitd_poll_event_wait(bitd_poll_event e, bitd_uint32 timeout);
bitd_socket_t bitd_poll_event_to_fd(bitd_poll_event e);



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _BITD_PLATFORM_POLL_EVENT_H_ */
