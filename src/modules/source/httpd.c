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
#include "bitd/log.h"
#include "bitd/msg.h"
#include "bitd/module-api.h"

#include <microhttpd.h>

/*****************************************************************************
 *                             MANIFEST CONSTANTS
 *****************************************************************************/



/*****************************************************************************
 *                                  MACROS 
 *****************************************************************************/
#define PORT_DEF 8080
#define QUOTA_DEF 1000


/*****************************************************************************
 *                                  TYPES
 *****************************************************************************/
static bitd_task_inst_create_t task_inst_create;
static bitd_task_inst_destroy_t task_inst_destroy;
static bitd_task_inst_run_t task_inst_run;
static bitd_task_inst_kill_t task_inst_kill;
static ttlog_keyid s_log_keyid;

/* The module control block */
struct module_s {
    bitd_nvp_t tags;
} g_module;

struct bitd_task_inst_s {
    char *task_name;
    char *task_inst_name;
    mmr_task_inst_t mmr_task_inst_hdl;
    bitd_nvp_t args;     /* Task instance arguments at creation */
    bitd_nvp_t tags;     /* Task instance tags at creation */
    bitd_buffer_type_t input_buffer_type;
    bitd_buffer_type_t output_buffer_type;
    bitd_event stop_ev;  /* The stop event */
    bitd_boolean stopped_p;
    struct MHD_Daemon *daemon;
    bitd_uint32 quota;   /* Outgoing queue quota */
    bitd_queue queue;    /* The outgoing queue */
};

/*****************************************************************************
 *                           FUNCTION DECLARATION
 *****************************************************************************/



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
    char *tags_str = NULL;

    s_log_keyid = ttlog_register("bitd-httpd");

    ttlog(log_level_trace, s_log_keyid,
	  "%s:%d:%s() called", __FILE__, __LINE__, __FUNCTION__);

    tags_str = bitd_nvp_to_yaml(tags, FALSE, FALSE);
    if (tags_str) {
	if (tags_str[0]) {
	    ttlog(log_level_trace, s_log_keyid,
		  "Module tags\n%s", tags_str);
	}
	free(tags_str);
    }

    ttlog(log_level_trace, s_log_keyid,
	  "Module dir: %s", module_dir);

    g_module.tags = bitd_nvp_clone(tags);

    /* Initialize the task API structure to zero, in case we're not 
       implementing some APIs */
    memset(&task_api, 0, sizeof(task_api));

    /* Set up the task API structure */
    task_api.task_inst_create = task_inst_create;
    task_api.task_inst_destroy = task_inst_destroy;
    task_api.task_inst_run = task_inst_run;
    task_api.task_inst_kill = task_inst_kill;

    /* Register the task */
    s_task = mmr_task_register(mmr_module, "httpd", &task_api);

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

    /* Release the module tags */
    bitd_nvp_free(g_module.tags);
    g_module.tags = NULL;

    ttlog_unregister(s_log_keyid);
} 


/*
 *============================================================================
 *                        answer_to_connection
 *============================================================================
 * Description:     Connection handler
 * Parameters:    
 * Returns:  
 */
static int answer_to_connection(void *cls, 
				struct MHD_Connection *connection,
				const char *url,
				const char *method, 
				const char *version,
				const char *upload_data,
				size_t *upload_data_size, 
				void **con_cls) {

    bitd_task_inst_t p = (bitd_task_inst_t)cls;
    struct MHD_Response *response = NULL;
    int ret;
    bitd_msg m = NULL;
    
    ttlog(log_level_trace, s_log_keyid,
	  "%s: %s() called", p->task_inst_name, __FUNCTION__);
    
    if (strcmp(method, "GET")) {
	return MHD_NO;              /* unexpected method */
    }

    if (!*con_cls) {
	/* Allocate the context */
	*con_cls = malloc(1);
	return MHD_YES;
    }

    m = bitd_msg_receive_w_tmo(p->queue, 0);
    if (!m) {
	response = MHD_create_response_from_buffer(0, NULL,
						   MHD_RESPMEM_PERSISTENT);
	if (response) {
	    ret = MHD_queue_response(connection, MHD_HTTP_NOT_FOUND,
				     response);
        } else {
	    ret = MHD_NO;
	}
	goto end;
    }

    response = MHD_create_response_from_buffer(bitd_msg_get_size(m),
					       (void*)m, 
					       MHD_RESPMEM_MUST_COPY);
    
    ret = MHD_queue_response(connection, MHD_HTTP_OK, response);

 end:
    if (response) {
	MHD_destroy_response(response);
    }
    if (m) {
	bitd_msg_free(m);
    }

    if (*con_cls) {
	free(*con_cls);
    }

    return ret;
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
    char *buf = NULL;
    int idx;
    int port = PORT_DEF;

    ttlog(log_level_trace, s_log_keyid,
	  "%s: %s() called", task_inst_name, __FUNCTION__);

    p = calloc(1, sizeof(*p));
    
    p->task_name = task_name;
    p->task_inst_name = task_inst_name;
    p->mmr_task_inst_hdl = mmr_task_inst_hdl;
    p->stop_ev = bitd_event_create(0);

    /* Save the args and tags */
    p->args = bitd_nvp_clone(args);
    p->tags = bitd_nvp_clone(tags);

    /* Log the args and tags */
    if (p->args) {
	buf = bitd_nvp_to_yaml(p->args, FALSE, FALSE);
	ttlog(log_level_trace, s_log_keyid,
	      "%s: Args:\n%s", task_inst_name, buf);
	free(buf);
    }

    if (p->tags) {
	buf = bitd_nvp_to_yaml(p->tags, FALSE, FALSE);
	ttlog(log_level_trace, s_log_keyid,
	      "%s: Tags:\n%s", task_inst_name, buf);
	free(buf);
    }

    /* Parse the input-type parameter */
    if (bitd_nvp_lookup_elem(args, "input-type", &idx) &&
	args->e[idx].type == bitd_type_string &&
	args->e[idx].v.value_string) {
	if (!strcmp(args->e[idx].v.value_string, "auto")) {
	    p->input_buffer_type = bitd_buffer_type_auto;
	} else if (!strcmp(args->e[idx].v.value_string, "string")) {
	    p->input_buffer_type = bitd_buffer_type_string;
	} else if (!strcmp(args->e[idx].v.value_string, "blob")) {
	    p->input_buffer_type = bitd_buffer_type_blob;
	} else if (!strcmp(args->e[idx].v.value_string, "json")) {
	    p->input_buffer_type = bitd_buffer_type_json;
	} else if (!strcmp(args->e[idx].v.value_string, "xml")) {
	    p->input_buffer_type = bitd_buffer_type_xml;
	} else if (!strcmp(args->e[idx].v.value_string, "yaml")) {
	    p->input_buffer_type = bitd_buffer_type_yaml;
	}
    }

    /* Parse the output-type parameter */
    if (bitd_nvp_lookup_elem(args, "output-type", &idx) &&
	args->e[idx].type == bitd_type_string &&
	args->e[idx].v.value_string) {
	if (!strcmp(args->e[idx].v.value_string, "auto")) {
	    p->output_buffer_type = bitd_buffer_type_auto;
	} else if (!strcmp(args->e[idx].v.value_string, "string")) {
	    p->output_buffer_type = bitd_buffer_type_string;
	} else if (!strcmp(args->e[idx].v.value_string, "blob")) {
	    p->output_buffer_type = bitd_buffer_type_blob;
	} else if (!strcmp(args->e[idx].v.value_string, "json")) {
	    p->output_buffer_type = bitd_buffer_type_json;
	} else if (!strcmp(args->e[idx].v.value_string, "xml")) {
	    p->output_buffer_type = bitd_buffer_type_xml;
	} else if (!strcmp(args->e[idx].v.value_string, "yaml")) {
	    p->output_buffer_type = bitd_buffer_type_yaml;
	}
    }

    /* Look for the port argument */
    if (bitd_nvp_lookup_elem(p->args, "port", &idx) &&
	p->args->e[idx].type == bitd_type_int64) {
	port = p->args->e[idx].v.value_int64;
    }

    /* Get the queue quota */
    p->quota = QUOTA_DEF;

    /* Create the message queue */
    p->queue = bitd_queue_create("plaintext background", 
				 BITD_QUEUE_FLAG_POLL, 
				 p->quota);

    p->daemon = MHD_start_daemon(MHD_USE_SELECT_INTERNALLY, 
				 port, 
				 NULL, 
				 NULL,
				 &answer_to_connection, 
				 p, 
				 MHD_OPTION_END);
    if (!p->daemon) {
	task_inst_destroy(p);
	return NULL;
    }

    return p;
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

    /* Stop the daemon */
    if (p->daemon) {
	MHD_stop_daemon(p->daemon);
    }

    count = bitd_queue_count(p->queue);
    if (count) {
	ttlog(log_level_warn, s_log_keyid,
	      "%s: %s(): Dropping %u result(s)", 
	      p->task_inst_name, __FUNCTION__, count);
    }

    /* Destroy the queue */
    bitd_queue_destroy(p->queue);

    bitd_nvp_free(p->args);
    bitd_nvp_free(p->tags);
    bitd_event_destroy(p->stop_ev);
    free(p);
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
    char *input_buf = NULL;
    int input_buf_len = 0;
    bitd_msg m;
    char *buf;

    ttlog(log_level_trace, s_log_keyid,
	  "%s: %s() called", p->task_inst_name, __FUNCTION__);

    if (input->type != bitd_type_void) {
	buf = bitd_object_to_string(input);
	ttlog(log_level_trace, s_log_keyid,
	      "%s: Input:\n%s", p->task_inst_name, buf);
	free(buf);
    }

    /* Convert input according to input_buffer_type */
    bitd_object_to_buffer(&input_buf, &input_buf_len,
			  input,
			  NULL,
			  p->output_buffer_type);

    /* Enqueue input */
    m = bitd_msg_alloc(0, input_buf_len);
    memcpy(m, input_buf, input_buf_len);

    if (bitd_msg_send(m, p->queue) != bitd_msgerr_ok) {
	ttlog(log_level_warn, s_log_keyid,
	      "%s: Queue full, dropping message", p->task_inst_name);
	bitd_msg_free(m);
    }

    if (input_buf) {
	free(input_buf);
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

    p->stopped_p = TRUE;
    bitd_event_set(p->stop_ev);
} 
