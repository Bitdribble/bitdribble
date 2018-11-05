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

#ifndef _BITD_MSG_H_
#define _BITD_MSG_H_

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

/* Messages and queue opaque types */
typedef struct bitd_msg_s *bitd_msg;
typedef struct bitd_queue_s *bitd_queue;

typedef enum {
    bitd_msgerr_ok,
    bitd_msgerr_timeout
} bitd_msgerr;

/*****************************************************************************
 *                            FUNCTION DEFINITIONS
 *****************************************************************************/

#define BITD_QUEUE_FLAG_POLL 0x1 /* Allocate a pollable file descriptor
                                  for the queue */

/* Create/destroy a message queue */
bitd_queue bitd_queue_create(char *name,              /* May be NULL */ 
			     bitd_uint32 flags,       /* BITD_QUEUE_FLAG_POLL */
			     bitd_uint64 size_quota); /* If 0, infinite queue */
void bitd_queue_destroy(bitd_queue q);

/* Change the queue quota - if 0, queue is infinite */
void bitd_queue_set_quota(bitd_queue q, bitd_uint64 size_quota);

/* Returns the queue name, or NULL if queue has no name */
char *bitd_queue_get_name(bitd_queue q);

/* Report the queue's file descriptor, if any */
bitd_socket_t bitd_queue_fd(bitd_queue q);

/* Report the queue's quota file descriptor, if any */
bitd_socket_t bitd_queue_quota_fd(bitd_queue q);

/* Get queue count & size */
bitd_uint32 bitd_queue_count(bitd_queue q);
bitd_uint64 bitd_queue_size(bitd_queue q);

/* Allocate/free a message */
bitd_msg bitd_msg_alloc(bitd_uint32 opcode, bitd_uint32 msg_size);
bitd_msg bitd_msg_realloc(bitd_msg m, bitd_uint32 msg_size);
void bitd_msg_free(bitd_msg m);

/* Get/set the message opcode */
bitd_uint32 bitd_msg_get_opcode(bitd_msg m);
void bitd_msg_set_opcode(bitd_msg m, bitd_uint32 op);

/* Get the message size */
bitd_uint32 bitd_msg_get_size(bitd_msg m);
bitd_boolean bitd_msg_set_size(bitd_msg m, bitd_uint32 size);

/* Send a message to a queue. This may fail only if the queue has a quota,
   and the quota has been reached (i.e. the queue is full). If the
   call fails, the caller still owns the message and is in charge of
   freeing it. */
bitd_msgerr bitd_msg_send(bitd_msg m, bitd_queue q);

/* Same as the above, with a timeout */
bitd_msgerr bitd_msg_send_w_tmo(bitd_msg m, bitd_queue q, bitd_uint32 tmo);

/* Receive any message from the queue. Blocks until a message is
   received. */
bitd_msg bitd_msg_receive(bitd_queue q);

/* Same as the above, with a timeout. Returns NULL if no message
   is available. */
bitd_msg bitd_msg_receive_w_tmo(bitd_queue q, bitd_uint32 tmo);

/* Wait for any message in the list of opcodes */
bitd_msg bitd_msg_receive_selective(bitd_queue q, 
				    bitd_uint32 n_opcodes,
				    bitd_uint32 *opcodes,
				    bitd_uint32 tmo);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _BITD_MSG_H_ */
