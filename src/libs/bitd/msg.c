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

/*****************************************************************************
 *                                INCLUDE FILES
 *****************************************************************************/
#include "bitd/msg.h"


/*****************************************************************************
 *                             MANIFEST CONSTANTS
 *****************************************************************************/



/*****************************************************************************
 *                                  MACROS
 *****************************************************************************/



/*****************************************************************************
 *                                  TYPES
 *****************************************************************************/

/* The queue control block */
struct bitd_queue_s {
    bitd_uint32 magic;
    char *name;
    struct bitd_msg_s *head, *tail; /* The list of enqueued messages */
    bitd_uint32 count;              /* Count of messages in the queue */
    bitd_uint64 size;               /* Total size of messages in the queue */
    bitd_uint64 quota;              /* Size quota */
    bitd_mutex lock;
    bitd_event receive_ev;
    bitd_event quota_ev;
    int ev_flags;
    int refcount;
};

#define QUEUE_MAGIC 0xabcdaaaa
#define ASSERT_QUEUE(q) bitd_assert((q) && (q)->magic == QUEUE_MAGIC)


/* The message control block */
struct bitd_msg_s {
    bitd_uint32 magic;
    struct bitd_msg_s *next; /* The list of enqueued messages */
    bitd_uint32 opcode;
    bitd_uint32 size;
};

#define MSG_MAGIC 0xabcdbbbb
#define ASSERT_MSG(m) bitd_assert((m) && (m)->magic == MSG_MAGIC)

/* Translate between user and system message headers */
#define PUBLIC_MSG(m)  ((m) ? (m) + 1 : NULL)
#define PRIVATE_MSG(m) ((m) ? (m) - 1 : NULL)

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
 *                        bitd_queue_create
 *============================================================================
 * Description: Create a message queue
 * Parameters:
 *     name - The name of the queue. May be NULL.
 *     queue_flags - The following flag is supported:
 *         BITD_QUEUE_FLAG_POLL - Queue event is pollable
 *     size_quota - If non-zero, this is the max number of bytes permitted
 *         in the queue. If zero, queue is infinite.
 * Returns:
 */
bitd_queue bitd_queue_create(char *name,              
			     bitd_uint32 queue_flags, 
			     bitd_uint64 size_quota) {
    bitd_queue q;
    int ret = -1;

    /* Allocate the queue control block */
    q = malloc(sizeof(*q));
    q->magic = QUEUE_MAGIC;
    if (name) {
        q->name = strdup(name);
    } else {
        q->name = NULL;
    }
    q->head = NULL;
    q->tail = NULL;
    q->size = 0;
    q->count = 0;
    q->quota = size_quota;
    q->lock = bitd_mutex_create();
    q->ev_flags = 0;
    if (queue_flags & BITD_QUEUE_FLAG_POLL) {
        q->ev_flags |= BITD_EVENT_FLAG_POLL;
    }
    q->receive_ev = bitd_event_create(q->ev_flags);
    q->refcount = 1;

    if (!q->lock || !q->receive_ev) {
        goto end;
    }

    /* We'll need a send event only if there's a queue quota */
    if (q->quota) {
        q->quota_ev = bitd_event_create(q->ev_flags);
        if (!q->quota_ev) {
            goto end;
        }
    } else {
        q->quota_ev = NULL;
    }

    /* Queue successfully created */
    ret = 0;
end:
    if (ret < 0) {
        bitd_queue_destroy(q);
        q = NULL;
    }

    return q;
}


/*
 *============================================================================
 *                        bitd_queue_destroy
 *============================================================================
 * Description: Destroy a message queue
 * Parameters:
 * Returns:
 */
void bitd_queue_destroy(bitd_queue q) {
    bitd_msg m, m_next;
    int refcount;

    if (!q) {
	return;
    }

    /* Sanity checks */
    ASSERT_QUEUE(q);

    if (q->lock) {
        bitd_mutex_lock(q->lock);
	refcount = (--q->refcount);
        bitd_mutex_unlock(q->lock);
	if (refcount > 0) {
	    return;
	}
    }

    /* Release all queue buffers */
    m = q->head;
    while (m) {
        ASSERT_MSG(m);
        m_next = m->next;
        free(m);
        m = m_next;
    }

    /* Destroy the mutex and the events */
    if (q->lock) {
        bitd_mutex_destroy(q->lock);
    }
    if (q->receive_ev) {
        bitd_event_destroy(q->receive_ev);
    }
    if (q->quota_ev) {
        bitd_event_destroy(q->quota_ev);
    }
    if (q->name) {
        free(q->name);
    }

    /* Clear the magic field */
    q->magic = 0;
    
    /* Deallocate the queue control block */
    free(q);
}


/*
 *============================================================================
 *                        bitd_queue_addref
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
void bitd_queue_addref(bitd_queue q) {
    /* Sanity checks */
    ASSERT_QUEUE(q);

    bitd_mutex_lock(q->lock);
    ++q->refcount;
    bitd_mutex_unlock(q->lock);
} 


/*
 *============================================================================
 *                        bitd_queue_delref
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
void bitd_queue_delref(bitd_queue q) {
    bitd_queue_destroy(q);
} 


/*
 *============================================================================
 *                        bitd_queue_set_quota
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
void bitd_queue_set_quota(bitd_queue q, bitd_uint64 size_quota) {

    if (size_quota && !q->quota_ev) {
        q->quota_ev = bitd_event_create(q->ev_flags);
    }
    q->quota = size_quota;
} 


/*
 *============================================================================
 *                        bitd_queue_get_name
 *============================================================================
 * Description: Returns the queue name, or NULL if queue has no name
 * Parameters:
 * Returns:
 */
char *bitd_queue_get_name(bitd_queue q) {

    /* Sanity checks */
    ASSERT_QUEUE(q);

    return q->name;
}


/*
 *============================================================================
 *                        bitd_queue_fd
 *============================================================================
 * Description: Report the queue's file descriptor, if any
 * Parameters:
 * Returns:
 */
bitd_socket_t bitd_queue_fd(bitd_queue q) {
    /* Sanity checks */
    ASSERT_QUEUE(q);

    if (q->receive_ev) {
        return bitd_event_to_fd(q->receive_ev);
    }

    return BITD_INVALID_SOCKID;
}


/*
 *============================================================================
 *                        bitd_queue_quota_fd
 *============================================================================
 * Description: Report the queue's quota file descriptor, if any
 * Parameters:
 * Returns:
 */
bitd_socket_t bitd_queue_quota_fd(bitd_queue q) {
    /* Sanity checks */
    ASSERT_QUEUE(q);

    if (q->quota_ev) {
        return bitd_event_to_fd(q->quota_ev);
    }

    return BITD_INVALID_SOCKID;
}


/*
 *============================================================================
 *                        bitd_queue_count
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
bitd_uint32 bitd_queue_count(bitd_queue q) {
    return q ? q->count : 0;
} 


/*
 *============================================================================
 *                        bitd_queue_size
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
bitd_uint64 bitd_queue_size(bitd_queue q) {
    return q ? q->size : 0;
} 


/*
 *============================================================================
 *                        bitd_msg_alloc
 *============================================================================
 * Description:  Allocate a message
 * Parameters:
 * Returns:
 */
bitd_msg bitd_msg_alloc(bitd_uint32 opcode, bitd_uint32 msg_size) {
    bitd_msg m;

    m = malloc(sizeof(*m) + msg_size);
    m->magic = MSG_MAGIC;
    m->opcode = opcode;
    m->next = NULL;
    m->size = msg_size;

    return PUBLIC_MSG(m);
}


/*
 *============================================================================
 *                        bitd_msg_realloc
 *============================================================================
 * Description:  Reallocate a message (similar to realloc)
 * Parameters:
 * Returns:
 */
bitd_msg bitd_msg_realloc(bitd_msg m, bitd_uint32 msg_size) {
    
    if (!m) {
	return bitd_msg_alloc(0, msg_size);
    }

    /* Get the internal message header */
    m = PRIVATE_MSG(m);

    /* Sanity check */
    ASSERT_MSG(m);
    
    m = realloc(m, sizeof(*m) + msg_size);
    m->magic = MSG_MAGIC;
    m->size = msg_size;

    return PUBLIC_MSG(m);
}


/*
 *============================================================================
 *                        bitd_msg_free
 *============================================================================
 * Description:  Free a message
 * Parameters:
 * Returns:
 */
void bitd_msg_free(bitd_msg m) {

    /* Get the internal message header */
    m = PRIVATE_MSG(m);

    /* Sanity checks */
    ASSERT_MSG(m);
    bitd_assert(!m->next);

    /* Clear the magic field */
    m->magic = 0;
    
    /* Free the message */
    free(m);
}


/*
 *============================================================================
 *                        bitd_msg_get_opcode
 *============================================================================
 * Description: Gets the message opcode
 * Parameters:
 * Returns:
 */
bitd_uint32 bitd_msg_get_opcode(bitd_msg m) {

    /* Get the internal message header */
    m = PRIVATE_MSG(m);

    /* Sanity checks */
    ASSERT_MSG(m);

    return m->opcode;
}


/*
 *============================================================================
 *                        bitd_msg_set_opcode
 *============================================================================
 * Description: Sets the message opcode
 * Parameters:
 * Returns:
 */
void bitd_msg_set_opcode(bitd_msg m, bitd_uint32 op) {

    /* Get the internal message header */
    m = PRIVATE_MSG(m);

    /* Sanity checks */
    ASSERT_MSG(m);

    m->opcode = op;
}


/*
 *============================================================================
 *                        bitd_msg_get_size
 *============================================================================
 * Description: Gets the message size
 * Parameters:
 * Returns:
 */
bitd_uint32 bitd_msg_get_size(bitd_msg m) {

    /* Get the internal message header */
    m = PRIVATE_MSG(m);

    /* Sanity checks */
    ASSERT_MSG(m);

    return m->size;
}


/*
 *============================================================================
 *                        bitd_msg_set_size
 *============================================================================
 * Description: Gets the message size
 * Parameters:
 * Returns:
 */
bitd_boolean bitd_msg_set_size(bitd_msg m, bitd_uint32 size) {

    /* Get the internal message header */
    m = PRIVATE_MSG(m);

    /* Sanity checks */
    ASSERT_MSG(m);

    if (size > m->size) {
	return FALSE;
    }

    /* Decrease the size */
    m->size = size;
    return TRUE;
}


/*
 *============================================================================
 *                        bitd_msg_send
 *============================================================================
 * Description:
 *     Sends a message to a queue. This may fail only if the queue has a quota,
 *     and the quota has been reached (i.e. the queue is full). If the
 *     call fails, the caller still owns the message and is in charge of
 *     freeing it. 
 * Parameters:
 * Returns:
 */
bitd_msgerr bitd_msg_send(bitd_msg m, bitd_queue q) {
    return bitd_msg_send_w_tmo(m, q, BITD_FOREVER);
}


/*
 *============================================================================
 *                        bitd_msg_send_w_tmo
 *============================================================================
 * Description:  Same as the above, with a timeout
 * Parameters:
 * Returns:
 */
bitd_msgerr bitd_msg_send_w_tmo(bitd_msg m, bitd_queue q, bitd_uint32 tmo) {
    bitd_msgerr ret = bitd_msgerr_timeout;
    bitd_uint32 current_time, start_time, tmo_left;
    bitd_boolean start_time_set = FALSE;

    /* Get the internal message header */
    m = PRIVATE_MSG(m);
    
    /* Sanity checks */
    ASSERT_MSG(m);
    ASSERT_QUEUE(q);

    /* Enter the lock */
    bitd_mutex_lock(q->lock);

    if (q->quota) {
	while (q->quota <= (q->size + sizeof(*m) + m->size)) {
            /* Quota is full - wait for more space */
            if (tmo != BITD_FOREVER) {
                if (!start_time_set) {
                    /* Remember when we started waiting */
                    start_time = bitd_get_time_msec();
                    current_time = start_time;
                    start_time_set = TRUE;
                } else {
                    current_time = bitd_get_time_msec();                
                }
                if (current_time - start_time >= tmo) {
                    /* No timeout left */
                    goto end;
                } else {
                    tmo_left = tmo - current_time + start_time;
                }
            } else {
                tmo_left = BITD_FOREVER;
            }
            
            /* But wait at least 1 msec to avoid tight loops */
            tmo_left = MAX(tmo_left, 1);

            /* Wait on the quota event */
            bitd_mutex_unlock(q->lock);
            bitd_event_wait(q->quota_ev, tmo_left);
            bitd_mutex_lock(q->lock);
        }
    }

    /* Enqueue the message */
    if (!q->tail) {
        bitd_assert(!q->head);
        q->head = m;
    } else {
        q->tail->next = m;
    }

    q->tail = m;
    m->next = NULL;

    /* Update the queue count and size */
    q->count++;
    q->size += (sizeof(struct bitd_msg_s) + m->size);

    /* Wake up the receivers */
    bitd_event_set(q->receive_ev);

    /* Success */
    ret = bitd_msgerr_ok;
end:
    /* Leave the lock */
    bitd_mutex_unlock(q->lock);

    return ret;
}


/*
 *============================================================================
 *                        bitd_msg_receive
 *============================================================================
 * Description: Receive any message from the queue. Blocks until a message is
 *      received.
 * Parameters:
 * Returns:
 */
bitd_msg bitd_msg_receive(bitd_queue q) {
    return bitd_msg_receive_selective(q, 0, NULL, BITD_FOREVER);
}


/*
 *============================================================================
 *                        bitd_msg_receive_w_tmo
 *============================================================================
 * Description: Same as the above, with a timeout. Returns NULL if no message
 *      is available.
 * Parameters:
 * Returns:
 */
bitd_msg bitd_msg_receive_w_tmo(bitd_queue q, bitd_uint32 tmo) {
    return bitd_msg_receive_selective(q, 0, NULL, tmo);
}


/*
 *============================================================================
 *                        bitd_msg_receive_selective
 *============================================================================
 * Description: Wait for any message in the list of opcodes
 * Parameters:
 * Returns:
 */
bitd_msg bitd_msg_receive_selective(bitd_queue q, 
				    bitd_uint32 n_opcodes,
				    bitd_uint32 *opcodes,
				    bitd_uint32 tmo) {
    bitd_msg m = NULL;
    bitd_msg m_prev;
    bitd_uint32 current_time, start_time, tmo_left;
    bitd_boolean start_time_set = FALSE;
    bitd_uint32 i;

    /* Sanity checks */
    ASSERT_QUEUE(q);

    /* Normalize the parameters */
    if (n_opcodes && !opcodes) {
        n_opcodes = 0;
    }
    
    /* Enter the mutex */
    bitd_mutex_lock(q->lock);

    for(;;) {        
        /* Look for a matching message */
        for (m = q->head, m_prev = NULL; m;
             m_prev = m, m = m->next) {

            /* Is this an opcode match? */
            if (!n_opcodes) {
                break;
            }

            for (i = 0; i < n_opcodes; i++) {
                if (opcodes[i] == m->opcode) {
                    break;
                }
            }
            
            if (i < n_opcodes) {
                /* Found our match */
                break;
            }
        }
        
        if (m) {
            /* Dequeue the message */
            if (m == q->head) {
                q->head = m->next;
            }
            if (m == q->tail) {
                q->tail = m_prev;
            }
            if (m_prev) {
                m_prev->next = m->next;
            }
            m->next = NULL;

	    /* Update the queue count */
            bitd_assert(q->count > 0);
            q->count--;

	    /* Update the queue size */
	    q->size -= (sizeof(struct bitd_msg_s) + m->size);
	    q->size = MAX(q->size, 0);

            /* Wake up queue writers, if any are waiting */
            if (q->quota_ev) {
                bitd_event_set(q->quota_ev);
            }
            break;
        }

        /* No message has been received. How much longer should we wait? */
        if (tmo != BITD_FOREVER) {
            
            if (!start_time_set) {
                /* Remember when we started waiting */
                start_time = bitd_get_time_msec();
                current_time = start_time;
                start_time_set = TRUE;
            } else {
                current_time = bitd_get_time_msec();                
            }
            if (current_time - start_time >= tmo) {
                /* No timeout left */
                break;
            } else {
                tmo_left = tmo - current_time + start_time;
            }
        } else {
            tmo_left = BITD_FOREVER;
        }

        /* But wait at least 1 msec to avoid tight loops */
        tmo_left = MAX(tmo_left, 1);

        /* Wait on the receive event */
        bitd_mutex_unlock(q->lock);
        bitd_event_wait(q->receive_ev, tmo_left);
        bitd_mutex_lock(q->lock);
    }

    if (!q->count) {
        /* For an empty queue, always clear the receive event */
        bitd_event_clear(q->receive_ev);
    }

    /* Leave the mutex */
    bitd_mutex_unlock(q->lock);

    return m ? PUBLIC_MSG(m) : NULL;
}

