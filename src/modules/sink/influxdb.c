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
#include "curl/curl.h"

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
    char *url;             /* In http://host:port format */
    char *database;        /* Influxdb database name */
    char *post_url;
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

struct map_tag_cb {
    char *buf;              /* Formatted tag buffer */
    int size;               /* Its size */
    int idx;                /* Write index in the buffer */
};

struct map_output_cb {
    struct map_tag_cb *t;
    bitd_uint64 tstamp_nsecs; /* Result timestamp */
    bitd_msg msg;             /* Formatted result message */
    bitd_uint32 size;         /* Its size */
    bitd_uint32 idx;          /* Write index in the message */
};


struct string {
  char *ptr;
  size_t len;
};

/*****************************************************************************
 *                           FUNCTION DECLARATION
 *****************************************************************************/
static void tcp_background(void *thread_args);
static void reinit_string(struct string *s);
static size_t writefunc(void *ptr, size_t size, size_t nmemb, struct string *s);

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

    s_log_keyid = ttlog_register("bitd-sink-influxdb");

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
    s_task = mmr_task_register(mmr_module, "sink-influxdb", &task_api);

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

    /* Release memory, if already allocated */
    if (p->tcb.url) {
	free(p->tcb.url);
	p->tcb.url = NULL;
    }
    if (p->tcb.post_url) {
	free(p->tcb.post_url);
	p->tcb.post_url = NULL;
    }
    if (p->tcb.database) {
	free(p->tcb.database);
	p->tcb.database = NULL;
    }

    /* Find the url */
    if (bitd_nvp_lookup_elem(args,
			   "url",
			   &idx) &&
	args->e[idx].type == bitd_type_string &&
	args->e[idx].v.value_string) {
	/* Copy the server address */
	p->tcb.url = strdup(args->e[idx].v.value_string);
    }

    /* Find the database name */
    if (bitd_nvp_lookup_elem(args,
			   "database",
			   &idx) &&
	args->e[idx].type == bitd_type_string &&
	args->e[idx].v.value_string) {
	/* Copy the server address */
	p->tcb.database = strdup(args->e[idx].v.value_string);
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
    if (p->tcb.url) {
	free(p->tcb.url);
    }
    if (p->tcb.post_url) {
	free(p->tcb.post_url);
    }
    if (p->tcb.database) {
	free(p->tcb.database);
    }
    free(p);
} 



/*
 *============================================================================
 *                        reinit_string
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
void reinit_string(struct string *s) {
    s->len = 0;
    s->ptr = realloc(s->ptr, s->len+1);
    s->ptr[0] = '\0';
}


/*
 *============================================================================
 *                        writefunc
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
size_t writefunc(void *ptr, size_t size, size_t nmemb, struct string *s) {
    size_t new_len = s->len + size*nmemb;
    s->ptr = realloc(s->ptr, new_len+1);
    memcpy(s->ptr+s->len, ptr, size*nmemb);
    s->ptr[new_len] = '\0';
    s->len = new_len;
    
    return size*nmemb;
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
    struct curl_waitfd cpfd[2];
    int poll_tmo;
    bitd_boolean queue_read_p = FALSE, http_post_p = FALSE;
    bitd_msg msg = NULL; /* Received and partially transmitted message */
    CURL *curl = NULL;
    CURLM *multi_handle = NULL;
    int still_running;
    struct string s = {};
    long response_code;
    ttlog_level log_level;
    CURLMcode mc;
    int numfds;

    ttlog(log_level_trace, s_log_keyid,
	  "%s: %s() started", tcb->name, __FUNCTION__);

    reinit_string(&s);

    /* Check for missing parameters */
    if (!tcb->url) {
	ttlog(log_level_err, s_log_keyid,
	      "%s: No url configured", 
	      tcb->name);
	goto end;	
    }
    if (!tcb->database) {
	ttlog(log_level_err, s_log_keyid,
	      "%s: No database configured", 
	      tcb->name);
	goto end;	
    }

    /* Compute the post URL */
    if (tcb->post_url) {
	free(tcb->post_url);
	tcb->post_url = NULL;
    }
    tcb->post_url = malloc(strlen(tcb->url) + strlen(tcb->database) + 24);
    sprintf(tcb->post_url, "%s/write?db=%s", tcb->url, tcb->database);

    curl = curl_easy_init();
    if (!curl) {
	ttlog(log_level_err, s_log_keyid,
	      "%s: Could not create curl object", 
	      tcb->name);
	goto end;
    }
    multi_handle = curl_multi_init();
    if (!multi_handle) {
	ttlog(log_level_err, s_log_keyid,
	      "%s: Could not create curl multi object", 
	      tcb->name);
	goto end;
    }

    curl_multi_add_handle(multi_handle, curl);

    curl_easy_setopt(curl, CURLOPT_URL, tcb->post_url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writefunc);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);

    while (!tcb->stopped_p) {
	memset(&cpfd, 0, sizeof(cpfd));

	/* Always wait on the stop event */
	cpfd[0].fd = bitd_event_to_fd(tcb->stop_ev);
	cpfd[0].events = CURL_WAIT_POLLIN;	

	if (!queue_read_p) {
	    /* Wait on queue read events */
	    cpfd[1].fd = bitd_queue_fd(tcb->queue);
	    cpfd[1].events = CURL_WAIT_POLLIN;
	} else {
	    cpfd[1].fd = BITD_INVALID_SOCKID;
	}

	mc = curl_multi_wait(multi_handle, cpfd, 2, 2000, &numfds);
	if (mc != CURLM_OK) {
	    ttlog(log_level_err, s_log_keyid,
		  "curl_multi_wait() failed: %s\n",
		  curl_multi_strerror(mc));
	    break;
	}

	if (!numfds) {
	    /* Timeout */
	    continue;
	} else {
#ifdef _XDEBUG
	    ttlog(log_level_trace, s_log_keyid,
		  "%s: bitd_poll() ret %d stop %d queue_read %d", 
		  tcb->name, ret,
		  (cpfd[0].fd != BITD_INVALID_SOCKID ? ((cpfd[0].revents & BITD_POLLIN) ? 1 : 0) : -1),
		  (cpfd[1].fd != BITD_INVALID_SOCKID ? ((cpfd[1].revents & BITD_POLLIN) ? 1 : 0) : -1));
#endif

	    if ((cpfd[0].fd != BITD_INVALID_SOCKID) && 
		(cpfd[0].revents & BITD_POLLIN)) {
		/* Stop event is readable. Clear the stop event. */
	        bitd_event_clear(tcb->stop_ev);
	    } 

	    /* Check the stop bit after checking the stop event */
	    if (tcb->stopped_p) {
		goto end;
	    }

	    if ((cpfd[1].fd != BITD_INVALID_SOCKID) && 
		(cpfd[1].revents & BITD_POLLIN)) {
		/* Can read from the queue */
		queue_read_p = TRUE;

		/* Clear the queue event */
	    } 

	send:
	    /* Do we have a new or partially sent message? */
	    if (msg) {
		if (!http_post_p) {
		    /* A new message - initiate the http post transaction */
		    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, 
				     bitd_msg_get_size(msg));
		    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, msg);
		    
		    /* Reset the curl handle - this is needed each time
		       a http transaction is initialized */
		    curl_multi_remove_handle(multi_handle, curl);
		    curl_multi_add_handle(multi_handle, curl);

		    http_post_p = TRUE;
		}

		/* Perform the http post - this routine must be called until
		   still_running is FALSE, but we'd like to wait for
		   other events in the meanwhile, so we're not
		   calling curl_multi_perform() in a loop. Instead,
		   we go back to the top of the event loop to call poll(). */
		mc = curl_multi_perform(multi_handle, &still_running);
		if (mc != CURLM_OK) {
		    ttlog(log_level_err, s_log_keyid,
			  "curl_easy_perform() failed: %s\n",
			  curl_multi_strerror(mc));
		    break;
		}

		/* Check the stop bit */
		if (tcb->stopped_p) {
		    goto end;
		}

		if (!still_running) {
		    /* Http post is completed, and the response has been
		       received. We can free the message. */
		    http_post_p = FALSE;
		    log_level = log_level_trace;
		    
		    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, 
				      &response_code);
		    
		    if ((response_code - (response_code % 100)) != 200) {
			log_level = log_level_warn;
			/* Need to retransmit message */
		    } else {
			/* Message successfully received */
			bitd_msg_free(msg);
			msg = NULL;
		    }
		    
		    ttlog(log_level, s_log_keyid,
			  "HTTP response status: %ld", response_code);
		    if (s.ptr && s.ptr[0]) {
			ttlog(log_level, s_log_keyid,
			      "HTTP response body: %s",
			      s.ptr);
			reinit_string(&s);
		    }

		    if (msg) {
			int curl_wait_tmo = 0; /* How long to wait before reconnecting */
			bitd_uint32 start_time, current_time;

			/* Check the stop bit */
			if (tcb->stopped_p) {
			    goto end;
			}
			
			start_time = bitd_get_time_msec();
			current_time = start_time;
			curl_wait_tmo = 30000;
			    
			ttlog(log_level_debug, s_log_keyid,
			      "%s: Reconnect to %s in up to %d secs", 
			      tcb->name, tcb->url,
			      curl_wait_tmo/1000);

			while (current_time - start_time < curl_wait_tmo) {
			    struct bitd_pollfd pfd;

			    /* Always wait on the stop event */
			    pfd.fd = bitd_event_to_fd(tcb->stop_ev);
			    pfd.events = CURL_WAIT_POLLIN;
			    pfd.revents = 0;			    
			    
			    /* Estimate how long to wait */
			    poll_tmo = curl_wait_tmo - (current_time - start_time);
			    
			    /* Round up by 20 msecs, so we don't end up in a tight loop at the end */
			    poll_tmo += 20;
			    
			    bitd_poll(&pfd, 1,  poll_tmo);

			    if (pfd.revents & BITD_POLLIN) {
				/* Stop event is readable. Clear the stop event. */
				bitd_event_clear(tcb->stop_ev);
			    } 
			    
			    /* Check the stop bit after clearing the stop event */
			    if (tcb->stopped_p) {
				goto end;
			    }

			    /* Update the current time */
			    current_time = bitd_get_time_msec();
			}

			ttlog(log_level_debug, s_log_keyid,
			      "%s: Reconnecting to %s", 
			      tcb->name, tcb->url);
			goto send;
		    }
		}
	    }

	    /* Can we both read and write? */
	    if (queue_read_p && !http_post_p && !msg) {
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
		    goto end;
		}

		/* Write it through a http post transaction */
		goto send;
	    }
	}
    }

 end:
    ttlog(log_level_trace, s_log_keyid,
	  "%s: %s() stopped", tcb->name, __FUNCTION__);
    
    if (msg) {
	bitd_msg_free(msg);
    }
    if (curl) {
	curl_easy_cleanup(curl);
    }
    if (multi_handle) {
	curl_multi_cleanup(multi_handle);
    }
    free(s.ptr);
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
    if (!name[0]) {
	return;
    }

    for (c = name; *c; c++) {
	if (isspace(*c)) {
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

    /* Skip strings, blobs and nvps. Influxdb accepts only numeric results. */
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
			   "%s%s value=", full_name, m->t->buf);

    /* Format the value */
    switch (type) {
    case bitd_type_boolean:
	snprintf_w_msg_realloc(&m->msg, &m->size, &m->idx, 
			       "%d", 
			       v->value_boolean ? 1 : 0);
	break;
    case bitd_type_int64:
	snprintf_w_msg_realloc(&m->msg, &m->size, &m->idx, 
			       "%lld", v->value_int64);
	break;
    case bitd_type_uint64:
	snprintf_w_msg_realloc(&m->msg, &m->size, &m->idx, 
			       "%llu", v->value_uint64);
	break;
    case bitd_type_double:
	snprintf_w_msg_realloc(&m->msg, &m->size, &m->idx, 
			       "%.*g", 
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
			   " %llu\n", m->tstamp_nsecs);
} 


/*
 *============================================================================
 *                        map_tags
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
static void map_tags(object_node_t *node, void *cookie) {
    struct map_tag_cb *t = (struct map_tag_cb *)cookie;
    char *full_name = node->full_name;
    bitd_value_t *v = &node->a.v;
    bitd_type_t type = node->a.type;
    int idx;
    char *c;

    /* Skip blobs and nvps. */
    switch (type) {
    case bitd_type_void:
    case bitd_type_blob:
    case bitd_type_nvp:
	return;
    default:
	break;
    }

    /* At depth zero, skip nodes named 'task'. The influxdb table will
       contain the task name. */
    if (node->depth == 1) {
	if (node->name && !strcmp(node->name, "task")) {
	    return;
	}
    }

    plaintext_escape(full_name);

    /* Format the name */
    snprintf_w_realloc(&t->buf, &t->size, &t->idx, 
		       ",%s=", full_name);

    /* Format the value */
    switch (type) {
    case bitd_type_boolean:
	snprintf_w_realloc(&t->buf, &t->size, &t->idx, 
			   "%d", 
			   v->value_boolean ? 1 : 0);
	break;
    case bitd_type_int64:
	snprintf_w_realloc(&t->buf, &t->size, &t->idx, 
			   "%lld", v->value_int64);
	break;
    case bitd_type_uint64:
	snprintf_w_realloc(&t->buf, &t->size, &t->idx, 
			   "%llu", v->value_uint64);
	break;
    case bitd_type_double:
	snprintf_w_realloc(&t->buf, &t->size, &t->idx, 
			   "%.*g", 
			   bitd_double_precision(v->value_double), 
			   v->value_double);
	break;
    case bitd_type_string:
	/* Remember the start of the string - so we can escape it */
	idx = t->idx;
	snprintf_w_realloc(&t->buf, &t->size, &t->idx, 
			   "%s", 
			   v->value_string);
	c = &t->buf[idx];
	plaintext_escape(c);
	break;
    default:
	/* Should not happen */
	bitd_assert(0);
	break;	
    }
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
    int task_name_len;
    bitd_int64 exit_code = 0;
    struct map_tag_cb t;
    struct map_output_cb m;
    object_node_t tags_node;

    memset(&t, 0, sizeof(t));
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
    m.tstamp_nsecs = input->v.value_nvp->e[idx].v.value_uint64;

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
    /* No need to escape task_inst_name, we only use it for logging */

    /* Format the tags */
    tags_node.full_name = NULL;
    tags_node.name = NULL;
    tags_node.sep = '.';
    tags_node.depth = 0;
    tags_node.a.type = bitd_type_nvp;
    tags_node.a.v.value_nvp = tags;
	    
    t.size = 1024;
    t.buf = malloc(t.size);
    t.buf[0] = 0;

    bitd_object_node_map(&tags_node, &map_tags, &t);

    /* Allocate the result */
    m.size = 102400;
    m.idx = 0;
    m.msg = bitd_msg_alloc(0, m.size);
    m.t = &t;

    snprintf_w_msg_realloc(&m.msg, &m.size, &m.idx, 
			   "%s.exit_code%s value=%lld %llu\n", 
			   task_name, t.buf,
			   exit_code, 
			   m.tstamp_nsecs);

    task_name_len = strlen(task_name);

    if (bitd_nvp_lookup_elem(input->v.value_nvp, "output", &idx)) {
	bitd_nvp_element_t *e = &input->v.value_nvp->e[idx];
	object_node_t node;
	
	node.full_name = malloc(task_name_len + 24);
	sprintf(node.full_name, "%s.output", task_name);
	
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
	
	node.full_name = malloc(task_name_len + 24);
	sprintf(node.full_name, "%s.error", task_name);
	
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
    if (t.buf) {
	free(t.buf);
    }
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
