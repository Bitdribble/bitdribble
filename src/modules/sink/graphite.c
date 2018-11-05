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
#include "bitd/format.h"
#include "bitd/log.h"
#include "bitd/msg.h"
#include "bitd/module-api.h"

#include <ctype.h>

/*****************************************************************************
 *                             MANIFEST CONSTANTS
 *****************************************************************************/



/*****************************************************************************
 *                                  MACROS 
 *****************************************************************************/
#define PLAINTEXT_PORT_DEF 2003
#define QUOTA_DEF 1000

#define SOCK_NOERROR(s, log_keyid)					\
    do {								\
	ret = (s);							\
	if (ret < 0) {							\
	    ttlog(log_level_trace, log_keyid,				\
		  "%s:%d: Socket routine returned %d, (%s %d)\n",	\
		  __FILE__, __LINE__,					\
		  ret, strerror(bitd_socket_errno), bitd_socket_errno);	\
	    goto end;							\
	}								\
    } while (0)

/*****************************************************************************
 *                                  TYPES
 *****************************************************************************/
static bitd_task_inst_create_t task_inst_create;
static bitd_task_inst_update_t task_inst_update;
static bitd_task_inst_destroy_t task_inst_destroy;
static bitd_task_inst_run_t task_inst_run;
static bitd_task_inst_kill_t task_inst_kill;
static ttlog_keyid s_log_keyid;

struct tcp_background_cb {
    ttlog_keyid log_keyid;
    char *name;
    char *server;          /* In host:port format */
    bitd_event stop_ev;      /* The stop event */
    bitd_boolean stopped_p;
    bitd_uint32 quota;       /* Message queue quota */
    bitd_queue queue;        /* The results queue */
};

struct bitd_task_inst_s {
    char *task_name;
    char *task_inst_name;
    mmr_task_inst_t mmr_task_inst_hdl;
    bitd_nvp_t args;
    bitd_thread th;                      /* The background thread */
    struct tcp_background_cb tcb;
};

struct map_output_cb {
    bitd_uint64 tstamp_secs;  /* Result timestamp */
    bitd_msg msg;             /* Formatted result message */
    bitd_uint32 size;         /* Its size */
    bitd_uint32 idx;          /* Write index in the message */
};

/*****************************************************************************
 *                           FUNCTION DECLARATION
 *****************************************************************************/
static void tcp_background(void *thread_args);


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

    s_log_keyid = ttlog_register("bitd-sink-graphite");

    ttlog(log_level_trace, s_log_keyid,
	  "%s:%d:%s() called", __FILE__, __LINE__, __FUNCTION__);

    /* Initialize the task API structure to zero, in case we're not 
       implementing some APIs */
    memset(&task_api, 0, sizeof(task_api));

    /* Set up the task API structure */
    task_api.task_inst_create = task_inst_create;
    task_api.task_inst_update = task_inst_update;
    task_api.task_inst_destroy = task_inst_destroy;
    task_api.task_inst_run = task_inst_run;
    task_api.task_inst_kill = task_inst_kill;

    /* Register the task */
    s_task = mmr_task_register(mmr_module, "sink-graphite", &task_api);

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

    /* Set up the tcp control block */
    p->tcb.name = strdup(task_inst_name);
    p->tcb.log_keyid = s_log_keyid;
    p->tcb.stop_ev = bitd_event_create(BITD_EVENT_FLAG_POLL);

    /* Get the queue quota */
    p->tcb.quota = QUOTA_DEF;

    /* Create the message queue */
    p->tcb.queue = bitd_queue_create("plaintext background", 
				   BITD_QUEUE_FLAG_POLL, 
				   p->tcb.quota);

    /* Call the update routine to set further configuration */
    task_inst_update(p, args, tags);

    return p;
} 


/*
 *============================================================================
 *                        task_inst_update
 *============================================================================
 * Description:     Guaranteed to be called only when run() has stopped
 * Parameters:    
 * Returns:  
 */
void task_inst_update(bitd_task_inst_t p,
		      bitd_nvp_t args,
		      bitd_nvp_t tags) {
    char *buf = NULL;
    int idx;

    ttlog(log_level_trace, s_log_keyid,
	  "%s: %s() called", p->task_inst_name, __FUNCTION__);

    if (p->th) {
	/* Stop the instance */
	p->tcb.stopped_p = TRUE;
	
	/* Set the stop event */
	bitd_event_set(p->tcb.stop_ev);

	/* Wait for the thread to exit */
	bitd_join_thread(p->th);
	p->th = NULL;
	p->tcb.stopped_p = FALSE;
    }

    /* Save the args and tags */
    bitd_nvp_free(p->args);
    p->args = bitd_nvp_clone(args);

    /* Log the args */
    if (p->args) {
	buf = bitd_nvp_to_yaml(p->args, FALSE, FALSE);
	ttlog(log_level_trace, s_log_keyid,
	      "%s: Args:\n%s", p->task_inst_name, buf);
	free(buf);
    }

    /* Find the server address */
    if (p->tcb.server) {
	free(p->tcb.server);
	p->tcb.server = NULL;
    }
    if (bitd_nvp_lookup_elem(args,
			   "server",
			   &idx) &&
	args->e[idx].type == bitd_type_string &&
	args->e[idx].v.value_string) {
	/* Copy the server address */
	p->tcb.server = strdup(args->e[idx].v.value_string);
    }

    /* Get the queue quota */
    p->tcb.quota = QUOTA_DEF;
    if (bitd_nvp_lookup_elem(p->args,
			   "queue-size",
			   &idx)) {
	if (p->args->e[idx].type == bitd_type_int64 &&
	    p->args->e[idx].v.value_int64 > 0) {
	    p->tcb.quota = (bitd_uint32)p->args->e[idx].v.value_int64;
	} else {
	    ttlog(log_level_warn, s_log_keyid,
		  "%s: Invalid queue-size, using default of %d\n", 
		  p->task_inst_name, QUOTA_DEF);
	}
    }

    /* Change the queue quota */
    bitd_queue_set_quota(p->tcb.queue, p->tcb.quota);

    /* (Re)create the background thread */
    p->th = bitd_create_thread("plaintext tcp background", 
			     tcp_background,
			     0, 128000, (void *)&p->tcb);
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
    bitd_uint32 count;

    ttlog(log_level_trace, s_log_keyid,
	  "%s: %s() called", p->task_inst_name, __FUNCTION__);

    /* Stop the instance */
    p->tcb.stopped_p = TRUE;

    /* Set the stop event */
    bitd_event_set(p->tcb.stop_ev);

    /* Wait for the thread to exit */
    bitd_join_thread(p->th);

    count = bitd_queue_count(p->tcb.queue);
    if (count) {
	ttlog(log_level_warn, s_log_keyid,
	      "%s: %s(): Dropping %u results", 
	      p->task_inst_name, __FUNCTION__, count);
    }

    /* Destroy the queue */
    bitd_queue_destroy(p->tcb.queue);

    bitd_nvp_free(p->args);
    bitd_event_destroy(p->tcb.stop_ev);

    if (p->tcb.name) {
	free(p->tcb.name);
    }
    if (p->tcb.server) {
	free(p->tcb.server);
    }
    free(p);
} 


/*
 *============================================================================
 *                        tcp_background
 *============================================================================
 * Description:     Background thread that reads messages from queue
 *     and writes contents of messages to tcp socket
 * Parameters:    
 * Returns:  
 */
void tcp_background(void *thread_arg) {
    struct tcp_background_cb *tcb = (struct tcp_background_cb *)thread_arg;
    int ret, port;
    struct bitd_pollfd pfd[3];
    int poll_tmo;
    struct sockaddr_storage sock_addr;
    bitd_socket_t sock = BITD_INVALID_SOCKID;
    int sock_wait_tmo = 0;   /* How long to wait before reconnecting socket */
    char *addr_str = NULL;
    int addr_str_len;
    bitd_boolean queue_read_p = FALSE, sock_write_p = FALSE;
    bitd_msg msg = NULL; /* Received and partially transmitted message */
    bitd_uint32 msg_size = 0; /* Size of the message */
    bitd_uint32 msg_idx = 0;  /* Index of where we stopped transmitting inside
			       the message */

    ttlog(log_level_trace, s_log_keyid,
	  "%s: %s() started", tcb->name, __FUNCTION__);

    /* Resolve the server address */
    if (!tcb->server) {
	ttlog(log_level_err, s_log_keyid,
	      "%s: No server configured", 
	      tcb->name);
	goto end;	
    }

    ret = bitd_resolve_hostport(&sock_addr, sizeof(sock_addr), tcb->server);
    if (ret) {
	ttlog(log_level_err, s_log_keyid,
	      "%s: Could not resolve %s: ret %d, %s", 
	      tcb->name, tcb->server, ret, bitd_gai_strerror(ret));
	goto end;
    }

    /* Set the default port, if not already set */
    port = ntohs(*bitd_sin_port(&sock_addr));
    if (!port) {
	port = PLAINTEXT_PORT_DEF;
	*bitd_sin_port(&sock_addr) = htons(port);
    }
    
    addr_str_len = 1024;
    addr_str = malloc(addr_str_len);

    inet_ntop(bitd_sin_family(&sock_addr), 
	      bitd_sin_addr(&sock_addr),
	      addr_str, addr_str_len);
    
    ttlog(log_level_trace, s_log_keyid,
	  "%s: Resolved %s as [%s]:%d", 
	  tcb->name, tcb->server, addr_str, port);

    while (!tcb->stopped_p) {
	if (sock == BITD_INVALID_SOCKID && sock_wait_tmo <= 0) {
	    /* (Re)create the socket */
	    sock = bitd_socket(bitd_sin_family(&sock_addr), 
			     SOCK_STREAM, IPPROTO_TCP);
	    if (sock == BITD_INVALID_SOCKID) {
		ttlog(log_level_trace, s_log_keyid,
		      "%s: Failed to create tcp socket, %s (errno %d)", 
		      tcb->name, strerror(bitd_socket_errno), bitd_socket_errno);
		break;
	    }

	    /* Make the socket non-blocking */
	    SOCK_NOERROR(bitd_set_blocking(sock, FALSE), tcb->log_keyid);

	    /* Connect the socket */
	    ret = bitd_connect(sock, &sock_addr, 
			     bitd_sockaddrlen(&sock_addr));
	    if (ret < 0) {
		if (bitd_socket_errno != BITD_EINPROGRESS &&
		    bitd_socket_errno != BITD_EWOULDBLOCK) {
		    SOCK_NOERROR(ret, tcb->log_keyid);
		}
	    }
	}	

	memset(&pfd, 0, sizeof(pfd));

	/* Always wait on the stop event */
	pfd[0].fd = bitd_event_to_fd(tcb->stop_ev);
	pfd[0].events = BITD_POLLIN;	

	if (!queue_read_p) {
	    /* Wait on queue read events */
	    pfd[1].fd = bitd_queue_fd(tcb->queue);
	    pfd[1].events = BITD_POLLIN;
	} else {
	    pfd[1].fd = BITD_INVALID_SOCKID;
	}

	if (!sock_write_p && (sock != BITD_INVALID_SOCKID)) {
	    /* Wait on socket write events*/
	    pfd[2].fd = sock;
	    pfd[2].events = BITD_POLLOUT;
	} else {
	    pfd[2].fd = BITD_INVALID_SOCKID;
	}

	if (sock_wait_tmo > 0) {
	    poll_tmo = sock_wait_tmo;
	} else {
	    poll_tmo = -1;
	}
	sock_wait_tmo = 0;

	ret = bitd_poll(pfd, 3, poll_tmo);
	
	/* Check the poll() return code */
	if (ret < 0) {
	    ttlog(log_level_err, s_log_keyid,
		  "%s: bitd_poll() error %s (errno %d)", 
		  tcb->name, 
		  strerror(bitd_socket_errno), bitd_socket_errno);
	    break;
	} else if (ret == 0) {
	    /* Timeout */
	    continue;
	} else {
#ifdef _XDEBUG
	    ttlog(log_level_trace, s_log_keyid,
		  "%s: bitd_poll() ret %d stop %d queue_read %d sock_write %d", 
		  tcb->name, ret,
		  (pfd[0].fd != BITD_INVALID_SOCKID ? ((pfd[0].revents & BITD_POLLIN) ? 1 : 0) : -1),
		  (pfd[1].fd != BITD_INVALID_SOCKID ? ((pfd[1].revents & BITD_POLLIN) ? 1 : 0) : -1),
		  (pfd[2].fd != BITD_INVALID_SOCKID ? ((pfd[2].revents & BITD_POLLOUT) ? 1 : 0) : -1));
#endif

	    if ((pfd[0].fd != BITD_INVALID_SOCKID) && (pfd[0].revents & BITD_POLLIN)) {
		/* Stop event is readable. Clear the stop event. */
	        bitd_event_clear(tcb->stop_ev);
	    } 

	    /* Check the stop bit after checking the stop event */
	    if (tcb->stopped_p) {
		break;
	    }

	    if ((pfd[1].fd != BITD_INVALID_SOCKID) && (pfd[1].revents & BITD_POLLIN)) {
		/* Can read from the queue */
		queue_read_p = TRUE;

		/* Clear the queue event */
	    } 
	    if ((pfd[2].fd != BITD_INVALID_SOCKID) && (pfd[2].revents & BITD_POLLOUT)) {
		/* Can write to the socket */
		sock_write_p = TRUE;
	    }
	    
	send:
	    /* Do we have a partially sent message, and can we write more? */
	    if (msg && sock_write_p) {
		while (sock_write_p && (sock != BITD_INVALID_SOCKID)) {
		    ret = bitd_send(sock, 
				  (char *)msg + msg_idx, 
				  msg_size - msg_idx, 
				  0);
		    if (ret < 0) {
			if (bitd_socket_errno == BITD_EAGAIN ||
			    bitd_socket_errno == BITD_EWOULDBLOCK) {
			    ttlog(log_level_trace, s_log_keyid,
				  "%s: sock_write TRUE, socket would block", 
				  tcb->name, ret);
			    
			    sock_write_p = FALSE;
			} else {
			    /* How long to wait before reconnecting */
			    sock_wait_tmo = 30000;
			    
			    ttlog(log_level_debug, s_log_keyid,
				  "%s: Connect to %s error (%s %d), retry in up to %d secs", 
				  tcb->name, tcb->server,
				  strerror(bitd_socket_errno), 
				  bitd_socket_errno,
				  sock_wait_tmo/1000);
			    
			    /* Close the socket */
			    bitd_close(sock);
			    sock = BITD_INVALID_SOCKID;
			    sock_write_p = FALSE;
			    
			    /* Go back to top of the loop, socket needs to be
			       re-created */
			    continue;
			}
		    } else {
			ttlog(log_level_trace, s_log_keyid,
			      "%s: sock_write TRUE, wrote %d bytes", 
			      tcb->name, ret);

			msg_idx += ret;
			if (msg_idx == msg_size) {
			    /* Done sending, go receive */
			    bitd_msg_free(msg);
			    msg = NULL;
			    goto recv;
			}
		    }
		}
	    }

	recv:
	    /* Can we both read and write? */
	    if (queue_read_p && sock_write_p) {
		/* Dequeue a result */
		msg = bitd_msg_receive_w_tmo(tcb->queue, 0);
		if (!msg) {
		    /* Queue is empty */
		    queue_read_p = FALSE;
		    ttlog(log_level_trace, s_log_keyid,
			  "%s: queue_read TRUE and no message in queue", 
			  tcb->name);
		    continue;
		}
	    
		ttlog(log_level_trace, s_log_keyid,
		      "%s: queue_read TRUE, message received", 
		      tcb->name);
		
		/* Check the stop bit - and empty message will signal us
		   to stop the background thread */
		if (tcb->stopped_p) {
		    break;
		}

		/* Got a message */
		msg_idx = 0;
		msg_size = bitd_msg_get_size(msg);

		/* Write it to the socket */
		goto send;
	    }
	}
    }

 end:
    ttlog(log_level_trace, s_log_keyid,
	  "%s: %s() stopped", tcb->name, __FUNCTION__);
    
    if (addr_str) {
	free(addr_str);
    }
    if (sock != BITD_INVALID_SOCKID) {
	bitd_close(sock);
    }
    if (msg) {
	bitd_msg_free(msg);
    }
} 


/*
 *============================================================================
 *                        plaintext_escape
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
static void plaintext_escape(char *name) {
    register char *c;

    if (!name) {
	name = "";
    }

    /* Replace spaces, semicolons, equal sign with '_' */
    for (c = name; *c; c++) {
	if (isspace(*c) || *c == ';' || *c == '=') {
	    *c = '_';
	}
    }
} 


/*
 *============================================================================
 *                        map_output
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
static void map_output(object_node_t *node, void *cookie) {
    struct map_output_cb *m = (struct map_output_cb *)cookie;
    char *full_name = node->full_name;
    bitd_value_t *v = &node->a.v;
    bitd_type_t type = node->a.type;

    /* Skip strings, blobs and nvps. Graphite accepts only numeric results. */
    switch (type) {
    case bitd_type_void:
    case bitd_type_string:
    case bitd_type_blob:
    case bitd_type_nvp:
	return;
    default:
	break;
    }

    plaintext_escape(full_name);

    /* Format the name */
    snprintf_w_msg_realloc(&m->msg, &m->size, &m->idx, 
			   "%s", full_name);

    /* Format the value */
    switch (type) {
    case bitd_type_boolean:
	snprintf_w_msg_realloc(&m->msg, &m->size, &m->idx, 
			       " %d", 
			       v->value_boolean ? 1 : 0);
	break;
    case bitd_type_int64:
	snprintf_w_msg_realloc(&m->msg, &m->size, &m->idx, 
			       " %lld", v->value_int64);
	break;
    case bitd_type_uint64:
	snprintf_w_msg_realloc(&m->msg, &m->size, &m->idx, 
			       " %llu", v->value_uint64);
	break;
    case bitd_type_double:
	snprintf_w_msg_realloc(&m->msg, &m->size, &m->idx, 
			       " %.*g", 
			       bitd_double_precision(v->value_double), 
			       v->value_double);
	break;
    default:
	/* Should not happen */
	bitd_assert(0);
	break;	
    }

    /* Format the timestamp */
    snprintf_w_msg_realloc(&m->msg, &m->size, &m->idx, 
			   " %llu\n", m->tstamp_secs);
} 


/*
 *============================================================================
 *                        format_plaintext_result
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
static bitd_msg format_plaintext_result(bitd_object_t *input) {
    int idx;
    bitd_nvp_t tags = NULL;
    char *task_name = NULL, *task_inst_name = NULL;
    int task_name_len, task_inst_name_len;
    bitd_int64 exit_code = 0;
    struct map_output_cb m;

    memset(&m, 0, sizeof(m));

    if (input->type != bitd_type_nvp) {
	goto end;
    }

    /* Parse the timestamp */
    if (!bitd_nvp_lookup_elem(input->v.value_nvp, "run-timestamp", &idx) ||
	input->v.value_nvp->e[idx].type != bitd_type_uint64) {
	goto end;
    }

    /* Convert from nanosecs to secs */
    m.tstamp_secs = input->v.value_nvp->e[idx].v.value_uint64 / 1000000000ULL;

    /* Parse the exit code */
    if (!bitd_nvp_lookup_elem(input->v.value_nvp, "exit-code", &idx) ||
	input->v.value_nvp->e[idx].type != bitd_type_int64) {
	goto end;
    }
    exit_code = input->v.value_nvp->e[idx].v.value_int64;

    /* Get the tags */
    if (!bitd_nvp_lookup_elem(input->v.value_nvp, "tags", &idx) ||
	input->v.value_nvp->e[idx].type != bitd_type_nvp) {
	goto end;
    }
    tags = input->v.value_nvp->e[idx].v.value_nvp;

    /* Parse the task name */
    if (!bitd_nvp_lookup_elem(tags, "task", &idx) ||
	tags->e[idx].type != bitd_type_string) {
	goto end;
    }
    
    if (!tags->e[idx].v.value_string ||
	!tags->e[idx].v.value_string[0]) {
	goto end;
    }
    task_name = strdup(tags->e[idx].v.value_string);
    plaintext_escape(task_name);

    /* Parse the task inst name */
    if (!bitd_nvp_lookup_elem(tags, "task-instance", &idx) ||
	tags->e[idx].type != bitd_type_string) {
	goto end;
    }
    if (!tags->e[idx].v.value_string ||
	!tags->e[idx].v.value_string[0]) {
	goto end;
    }
    task_inst_name = strdup(tags->e[idx].v.value_string);
    plaintext_escape(task_inst_name);
	    
    /* Allocate the result */
    m.size = 102400;
    m.idx = 0;
    m.msg = bitd_msg_alloc(0, m.size);

    snprintf_w_msg_realloc(&m.msg, &m.size, &m.idx, 
			   "%s.%s.exit_code %lld %llu\n", 
			   task_name, task_inst_name,
			   exit_code, 
			   m.tstamp_secs);

    task_name_len = strlen(task_name);
    task_inst_name_len = strlen(task_inst_name);

    if (bitd_nvp_lookup_elem(input->v.value_nvp, "output", &idx)) {
	bitd_nvp_element_t *e = &input->v.value_nvp->e[idx];
	object_node_t node;
	
	node.full_name = malloc(task_name_len + task_inst_name_len + 24);
	sprintf(node.full_name, "%s.%s.output", task_name, task_inst_name);
	
	node.name = NULL;
	node.sep = '.';
	node.depth = 0;
	node.a.type = e->type;
	node.a.v = e->v;
	    
	bitd_object_node_map(&node, &map_output, &m);
	
	free(node.full_name);
    }
    
    if (bitd_nvp_lookup_elem(input->v.value_nvp, "error", &idx)) {
	bitd_nvp_element_t *e = &input->v.value_nvp->e[idx];
	object_node_t node;
	
	node.full_name = malloc(task_name_len + task_inst_name_len + 24);
	sprintf(node.full_name, "%s.%s.error", task_name, task_inst_name);
	
	node.name = NULL;
	node.sep = '.';
	node.depth = 0;
	node.a.type = e->type;
	node.a.v = e->v;
	    
	bitd_object_node_map(&node, &map_output, &m);
	
	free(node.full_name);
    }
    
    /* Set the message size */
    bitd_msg_set_size(m.msg, m.idx);

 end:
    if (!m.msg) {
	ttlog(log_level_trace, s_log_keyid,
	      "%s(): Dropping incorrectly formatted result from %s:%s", 
	      __FUNCTION__,
	      task_name ? task_name : "unknown",
	      task_inst_name ? task_inst_name : "unknown");
    }

    if (task_name) {
	free(task_name);
    }
    if (task_inst_name) {
	free(task_inst_name);
    }

    return m.msg;
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
    bitd_msg msg;

    ttlog(log_level_trace, s_log_keyid,
	  "%s: %s() called", p->task_inst_name, __FUNCTION__);

#ifdef _XDEBUG
    if (input->type != bitd_type_void) {
	char *buf = bitd_object_to_yaml(input, FALSE);
	ttlog(log_level_trace, s_log_keyid,
	      "%s: Input:\n%s", p->task_inst_name, buf);
	free(buf);
    }
#endif

    /* Format the result */
    msg = format_plaintext_result(input);
    if (msg) {
	/* Send the result message with a timeout, so we don't block 
	   on the quota */
	while (bitd_msg_send_w_tmo(msg, p->tcb.queue, 250) == bitd_msgerr_timeout) {
	    if (p->tcb.stopped_p) {
		ttlog(log_level_warn, s_log_keyid,
		      "%s: %s(): Dropping result", 
		      p->task_inst_name, __FUNCTION__);
		bitd_msg_free(msg);
		break;
	    }
	}
    }

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

    p->tcb.stopped_p = TRUE;
    bitd_event_set(p->tcb.stop_ev);
} 
