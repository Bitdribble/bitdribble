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
#include "bitd/module-api.h"
#include "bitd/mmr-api.h"
#include "bitd/file.h"
#include "bitd/timer-thread.h"
#include "bitd/log.h"
#include "signal.h" /* Should work for Win32 also */

#ifdef __linux__
# include <pthread.h>
#endif

/*****************************************************************************
 *                             MANIFEST CONSTANTS
 *****************************************************************************/
#define RESULT_FILE_DEF "stdout"
#define LOG_FILE_NAME_DEF "stdout"
#define LOG_FILE_SIZE_DEF 1024*1024*16 /* 16M */
#define LOG_FILE_COUNT_DEF 3

#define YAML_DOC_START "---"
#define YAML_STREAM_END "..."

/*****************************************************************************
 *                                  MACROS 
 *****************************************************************************/
#define MMR_NOERROR(f) do {					\
	mmr_err_t _ret;						\
	_ret = (f);						\
	if (_ret != mmr_err_ok) {				\
	    fprintf(stderr,					\
		    "%s: %s: mmr error: %s.\n",			\
		    g_prog_name,				\
		    #f,						\
		    mmr_get_error_name(_ret));			\
	    fprintf(stderr,					\
		    "At %s:%d.\n",				\
		    __FILE__, __LINE__);			\
	    exit(-1);						\
	}							\
    } while(0)

/*****************************************************************************
 *                                  TYPES
 *****************************************************************************/
struct dll_cb {
    struct dll_cb *next;
    struct dll_cb *prev;
    char *name; /* dll name */
    bitd_dll h;   /* dll handle */
};

struct dll_list {
    struct dll_cb *head;
    struct dll_cb *tail;
};

#define DLL_LIST_HEAD(d) \
    ((struct dll_cb *)&(d)->head)



/*****************************************************************************
 *                           FUNCTION DECLARATION
 *****************************************************************************/


/*****************************************************************************
 *                                VARIABLES
 *****************************************************************************/
static char* g_prog_name = "";
static ttlog_keyid g_log_keyid, g_log_mmr_keyid;

/* The logger function */
static mmr_vlog_func_t mvlog;

/* The config and the result file */
static char* g_config_file = NULL;
static bitd_boolean g_config_xml = FALSE;
static bitd_boolean g_config_yaml = FALSE;

static char* g_result_file = RESULT_FILE_DEF;
static FILE* g_result_fstream;
static bitd_boolean g_xml_results;
static long g_result_count = 0;
static long g_max_result_count = 0;

static bitd_boolean g_got_sigint;
static bitd_boolean g_got_sigterm;
static bitd_boolean g_got_sighup;
static bitd_event g_stop_event;


/*****************************************************************************
 *                          FUNCTION IMPLEMENTATION
 *****************************************************************************/


/*
 *============================================================================
 *                        usage
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
void usage() {

    printf("\nUsage: %s [OPTIONS ... ]\n\n", g_prog_name);
    printf("This program tests the module manager.\n\n");

    printf("Options:\n"
	   "  -c config_file.{xml|yml|yaml}\n"
	   "    The module-agent configuration file. Format is determined from\n"
	   "    file suffix. Default format is yaml.\n"
	   "  -cx config_file.xml\n"
	   "    The module-agent configuration file, in xml format.\n"
	   "  -cy config_file.yaml\n"
	   "    The module-agent configuration file, in yaml format.\n"
	   "  -r result_file\n"
	   "    The module-agent result file. Default: %s.\n"
	   "  -rx\n"
	   "    Format the result output as xml.\n"
	   "  -ry\n"
	   "    Format the result output as yaml (default).\n"
	   "  -mrc|--max-result-count count\n"
	   "    Exit after this many results are reported. If set to zero,\n"
	   "    don't exit based on the result count. Default: 0.\n"
	   "  -lp load_path\n"
	   "    DLL load library path.\n"
	   "  --n-worker-threads thread_count\n"
	   "    Set the max number of worker theads.\n"
           "  -l|--log-level none|crit|error|warn|info|debug|trace\n"
           "    Set the log level (default: none).\n"
 	   "  -lk|--log-key-level key_name none|crit|error|warn|info|debug|trace\n"
	   "    Set log key level.\n"
	   "  -lf|--log-file file_name\n"
           "    Save log messages to a file. Default: %s.\n"
	   "  -ls|--log-file-size size\n"
	   "    Set the log file size. Default: %d.\n"
	   "  -lc|--log-file-count count\n"
	   "    Set the log file count. Default: %d.\n"
           "  -h, --help, -?\n"
           "    Show this help.\n"
	   "Environment vaiables (command line options take precedence):\n"
	   "    %s - Dll load library path.\n",
	   RESULT_FILE_DEF,
	   LOG_FILE_NAME_DEF,
	   LOG_FILE_SIZE_DEF,
	   LOG_FILE_COUNT_DEF,
	   bitd_get_dll_path_name());
} 


/*
 *============================================================================
 *                        mvlog
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
int mvlog(ttlog_level level, char *format_string, va_list args) {

    return ttvlog(level, g_log_mmr_keyid, format_string, args);
} 


/*
 *============================================================================
 *                        mlog
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
int mlog(ttlog_level level, char *format_string, ...) {
    va_list args;
    int ret;

    va_start(args, format_string);
    ret = mvlog(level, format_string, args);
    va_end(args);

    return ret;    
} 


/*
 *============================================================================
 *                        parse_config_xml
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
bitd_nvp_t parse_config_xml(FILE *f) {
    int chunk_size = 1024;
    char *buf;
    int idx;
    bitd_xml_stream s = NULL;
    bitd_object_t a;
    char *object_name = NULL;
    bitd_boolean done;

    /* Open the input file */
    ttlog(log_level_trace, g_log_keyid, "Loading config from %s", 
	  g_config_file);

    /* Initialize the nx stream */
    s = bitd_xml_stream_init();

    /* Read the input */
    buf = malloc(chunk_size+1);
    idx = 0;

    done = FALSE;
    while (!done) {
	idx = fread(buf, 1, chunk_size, f);
	if (ferror(f)) {
	    fprintf(stderr, "%s: %s: Read error, errno %d (%s).\n",
		    g_prog_name, g_config_file, 
		    bitd_socket_errno, strerror(bitd_socket_errno));
	    fclose(f);
	    free(buf);
	    bitd_xml_stream_free(s);
	    exit(-1);
	}

	buf[idx] = 0;
	done = feof(f);

	/* Parse the buffer */
	if (!bitd_xml_stream_read(s, buf, idx, done)) {
	    fprintf(stderr, "%s: %s: %s.\n",
		    g_prog_name, g_config_file, bitd_xml_stream_get_error(s));
	    free(buf);
	    bitd_xml_stream_free(s);
	    return NULL;	    
	}
    }

    free(buf);

    /* Get the nvp */
    bitd_object_init(&a);

    /* Get the object */
    bitd_xml_stream_get_object(s, &a, &object_name);

    /* Can close the nx stream now */
    bitd_xml_stream_free(s);

    if (!object_name || strcmp(object_name, "module-config")) {
	fprintf(stderr, 
		"%s: %s: Invalid xml root name '%s', should be 'module-config'.\n",
		g_prog_name, g_config_file, object_name ? object_name: "");
	bitd_object_free(&a);
	if (object_name) {
	    free(object_name);
	}
	return NULL;
    }

    if (object_name) {
	free(object_name);
    }

    if (a.type != bitd_type_nvp) {
	fprintf(stderr, 
		"%s: %s: Invalid xml config, should be nvp format.\n",
		g_prog_name, g_config_file);
	bitd_object_free(&a);
	return NULL;
    }

    return a.v.value_nvp;
} 


/*
 *============================================================================
 *                        parse_config_yaml
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
bitd_nvp_t parse_config_yaml(FILE *f) {
    bitd_nvp_t nvp = NULL;
    bitd_yaml_parser s_yaml = NULL;
    char *buf;
    int idx, size = 10240;
    
    s_yaml = bitd_yaml_parser_init();
    
    /* Read the input */
    buf = malloc(size);
    idx = 0;
    
    /* Read the whole yaml file */
    do {
	if (idx + 1 >= size) {
	    if (size < 4*1024*1024) {
		size *= 2;
	    } else {
		size += 4*1024*1024;
	    }
	    buf = realloc(buf, size);
	}

	idx += fread(buf + idx, 1, size - idx - 1, f);
	if (ferror(f)) {
	    fprintf(stderr, "%s: %s: Read error, errno %d (%s).\n",
		    g_prog_name, g_config_file, errno, strerror(errno));
	    free(buf);
	    bitd_yaml_parser_free(s_yaml);
	    return NULL;
	}
	
	/* Null terminate */
	buf[idx] = 0;
    } while (!feof(f));

    /* Parse the buffer */
    if (!bitd_yaml_parse(s_yaml, buf, idx)) {
	fprintf(stderr, "%s: %s: %s.\n",
		g_prog_name, g_config_file, 
		bitd_yaml_parser_get_error(s_yaml));
	bitd_yaml_parser_free(s_yaml);
	free(buf);
	return NULL;	    
    }

    /* Get the nvp */
    nvp = bitd_yaml_parser_get_document_nvp(s_yaml);
    if (!nvp) {
	fprintf(stderr, 
		"%s: %s: Invalid yaml config, should be nvp format.\n",
		g_prog_name, g_config_file);
	/* Fallthrough */
    }
    
    /* Can close the nx stream now */
    bitd_yaml_parser_free(s_yaml);

    free(buf);

    return nvp;
} 


/*
 *============================================================================
 *                        set_task_inst_config
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
bitd_boolean set_task_inst_config(bitd_nvp_t config_nvp) {
    bitd_nvp_t modules_nvp = NULL;
    bitd_nvp_t task_inst_nvp = NULL;
    int i, idx;
    mmr_err_t ret;

    /* Find the tags */
    if (bitd_nvp_lookup_elem(config_nvp, "tags", &idx) &&
	config_nvp->e[idx].type == bitd_type_nvp) {
	bitd_nvp_t tags;
	
	/* Set the tags */
	tags = config_nvp->e[idx].v.value_nvp;
	MMR_NOERROR(mmr_set_tags(tags));
    }

    /* Find the list of modules */
    if (bitd_nvp_lookup_elem(config_nvp, "modules", &idx) &&
	config_nvp->e[idx].type == bitd_type_nvp) {
	char *elem_names[] = {"module-name"};
	modules_nvp = bitd_nvp_trim_bytype(config_nvp->e[idx].v.value_nvp,
					 elem_names, 1, bitd_type_string);
    }

    /* Find the list of task instances */
    {
	char *elem_names[] = {"task-inst"};
	task_inst_nvp = bitd_nvp_trim_bytype(config_nvp, elem_names, 1,
					   bitd_type_nvp);
    }
    
    /* Load the modules */
    if (modules_nvp) {
	for (i = 0; i < modules_nvp->n_elts; i++) {
	    mmr_err_t ret;

	    ret = mmr_load_module(modules_nvp->e[i].v.value_string);
	    if (ret != mmr_err_ok) {
		/* Sleep a bit to let log messages come through */
		bitd_sleep(500);

		fprintf(stderr,
			"%s: mmr_load_module(): %s: %s.\n",
			g_prog_name, 
			modules_nvp->e[i].v.value_string,
			mmr_get_error_name(ret));
		
		if (ret == mmr_err_module_not_found) {
		    char *name;

		    name = bitd_get_dll_name(modules_nvp->e[i].v.value_string);
		    fprintf(stderr,
			    "%s is not in the path.\n", 
			    name);
		    free(name);

		    fprintf(stderr,
			    "The module path is configured through the '-lp' parameter or the %s environment variable.\n", bitd_get_dll_path_name());
		}
		exit(-1);
	    }
	}
    }

    /* Create the task instances */
    if (task_inst_nvp) {
	for (i = 0; i < task_inst_nvp->n_elts; i++) {
	    char *task_name = NULL;
	    char *task_inst_name = NULL;
	    bitd_nvp_t schedule_nvp = NULL;
	    mmr_task_inst_params_t params;
	    bitd_nvp_t nvp;

	    mmr_task_inst_params_init(&params);

	    nvp = task_inst_nvp->e[i].v.value_nvp;

	    if (bitd_nvp_lookup_elem(nvp, "task-name", &idx) &&
		nvp->e[idx].type == bitd_type_string) {
		task_name = nvp->e[idx].v.value_string;
	    }
	    if (bitd_nvp_lookup_elem(nvp, "task-inst-name", &idx) &&
		nvp->e[idx].type == bitd_type_string) {
		task_inst_name = nvp->e[idx].v.value_string;
	    }
	    if (bitd_nvp_lookup_elem(nvp, "schedule", &idx) &&
		nvp->e[idx].type == bitd_type_nvp) {
		schedule_nvp = nvp->e[idx].v.value_nvp;
	    }
	    if (bitd_nvp_lookup_elem(nvp, "args", &idx) &&
		nvp->e[idx].type == bitd_type_nvp) {
		params.args = nvp->e[idx].v.value_nvp;
	    }
	    if (bitd_nvp_lookup_elem(nvp, "tags", &idx) &&
		nvp->e[idx].type == bitd_type_nvp) {
		params.tags = nvp->e[idx].v.value_nvp;
	    }
	    if (bitd_nvp_lookup_elem(nvp, "input", &idx)) {
		params.input.v = nvp->e[idx].v;
		params.input.type = nvp->e[idx].type;
	    }

	    ret = mmr_task_inst_create(task_name,
				       task_inst_name,
				       schedule_nvp,
				       &params);
	    if (ret == mmr_err_no_task) {
		fprintf(stderr,
			"%s: Task '%s' not found in a loaded module.\n",
			g_prog_name,
			task_name);
		fprintf(stderr,
			"Loaded modules:");
		if (!modules_nvp || !modules_nvp->n_elts) {
		    fprintf(stderr,
			    " none.\n");
		} else {
		    fprintf(stderr,
			    "\n");
		    for (i = 0; i < modules_nvp->n_elts; i++) {
			fprintf(stderr,
				"  %s\n", modules_nvp->e[i].v.value_string);
		    }
		}
		exit(-1);
	    } else {
		MMR_NOERROR(ret);
	    }
	}
    }

    bitd_nvp_free(modules_nvp);
    bitd_nvp_free(task_inst_nvp);

    return TRUE;
} 


/*
 *============================================================================
 *                        clear_test_inst_config
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
bitd_boolean clear_test_inst_config(bitd_nvp_t config_nvp) {
    bitd_nvp_t modules_nvp = NULL;
    bitd_nvp_t task_inst_nvp = NULL;
    int i, idx;

    /* Find the list of modules */
    if (bitd_nvp_lookup_elem(config_nvp, "modules", &idx) &&
	config_nvp->e[idx].type == bitd_type_nvp) {
	char *elem_names[] = {"module-name"};
	modules_nvp = bitd_nvp_trim_bytype(config_nvp->e[idx].v.value_nvp,
					 elem_names, 1, bitd_type_string);
    }

    /* Find the list of task instances */
    {
	char *elem_names[] = {"task-inst"};
	task_inst_nvp = bitd_nvp_trim_bytype(config_nvp, elem_names, 1,
					   bitd_type_nvp);
    }

    /* Destroy the task instances */
    if (task_inst_nvp) {
	/* Prepare to destroy the task instances */
	for (i = 0; i < task_inst_nvp->n_elts; i++) {
	    char *task_name = NULL;
	    char *task_inst_name = NULL;
	    bitd_nvp_t nvp;

	    nvp = task_inst_nvp->e[i].v.value_nvp;

	    if (bitd_nvp_lookup_elem(nvp, "task-name", &idx) &&
		nvp->e[idx].type == bitd_type_string) {
		task_name = nvp->e[idx].v.value_string;
	    }
	    if (bitd_nvp_lookup_elem(nvp, "task-inst-name", &idx) &&
		nvp->e[idx].type == bitd_type_string) {
		task_inst_name = nvp->e[idx].v.value_string;
	    }

	    MMR_NOERROR(mmr_task_inst_prepare_destroy(task_name,
						      task_inst_name));
	}

	/* Destroy the task instances */
	for (i = 0; i < task_inst_nvp->n_elts; i++) {
	    char *task_name = NULL;
	    char *task_inst_name = NULL;
	    bitd_nvp_t nvp;

	    nvp = task_inst_nvp->e[i].v.value_nvp;

	    if (bitd_nvp_lookup_elem(nvp, "task-name", &idx) &&
		nvp->e[idx].type == bitd_type_string) {
		task_name = nvp->e[idx].v.value_string;
	    }
	    if (bitd_nvp_lookup_elem(nvp, "task-inst-name", &idx) &&
		nvp->e[idx].type == bitd_type_string) {
		task_inst_name = nvp->e[idx].v.value_string;
	    }

	    MMR_NOERROR(mmr_task_inst_destroy(task_name,
					      task_inst_name));
	}
    }

    /* Unload the modules */
    if (modules_nvp) {
	for (i = 0; i < modules_nvp->n_elts; i++) {
	    if (modules_nvp->e[i].type == bitd_type_string) {
		MMR_NOERROR(mmr_unload_module(modules_nvp->e[i].v.value_string));
	    }
	}
    }

    bitd_nvp_free(modules_nvp);
    bitd_nvp_free(task_inst_nvp);

    return TRUE;
} 



/*
 *============================================================================
 *                        report_results
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
static void report_results(char *task_name,
			   char *task_inst_name,
			   bitd_nvp_t tags,
			   bitd_uint64 run_id,
			   bitd_uint64 tstamp_ns,
			   mmr_task_inst_results_t *r) {
    bitd_nvp_t nvp;
    char *buf;

    ttlog(log_level_debug, g_log_keyid, 
	  "%s: %s: Results",
	  task_name,
	  task_inst_name);

    g_result_count++;

    /* Do we have a file stream open? */
    if (!g_result_fstream) {
	goto end;
    }

    nvp = bitd_nvp_alloc(10);
    
    /* Copy the tags */
    nvp->e[nvp->n_elts].name = "tags";
    nvp->e[nvp->n_elts].v.value_nvp = tags;
    nvp->e[nvp->n_elts++].type = bitd_type_nvp;

    /* Copy the run id */
    nvp->e[nvp->n_elts].name = "run-id";
    nvp->e[nvp->n_elts].v.value_uint64 = run_id;
    nvp->e[nvp->n_elts++].type = bitd_type_uint64;

    /* Copy the timestamp */
    nvp->e[nvp->n_elts].name = "run-timestamp";
    nvp->e[nvp->n_elts].v.value_uint64 = tstamp_ns;
    nvp->e[nvp->n_elts++].type = bitd_type_uint64;

    /* Exit code */
    nvp->e[nvp->n_elts].name = "exit-code";
    nvp->e[nvp->n_elts].v.value_int64 = r->exit_code;
    nvp->e[nvp->n_elts++].type = bitd_type_int64;

    if (r->output.type != bitd_type_void) {
	/* Output */
	nvp->e[nvp->n_elts].name = "output";
	nvp->e[nvp->n_elts].v = r->output.v;
	nvp->e[nvp->n_elts++].type = r->output.type;
    }

    if (r->error.type != bitd_type_void) {
	/* Error */
	nvp->e[nvp->n_elts].name = "error";
	nvp->e[nvp->n_elts].v = r->error.v;
	nvp->e[nvp->n_elts++].type = r->error.type;
    }

    /* Write the results */
    if (g_xml_results) {
	buf = bitd_nvp_to_xml_elem(nvp, "task-inst-result", 2, FALSE);
	fprintf(g_result_fstream, "%s", buf);
	free(buf);
    } else {
	buf = bitd_nvp_to_yaml(nvp, FALSE, FALSE);
	bitd_assert(buf);
	if (buf) {
	    fprintf(g_result_fstream, "%s", buf);
	    free(buf);
	}
	fprintf(g_result_fstream, "%s\n", YAML_DOC_START);
    }

    /* Reset the number of elements to zero */
    nvp->n_elts = 0;

    bitd_nvp_free(nvp);

 end:
    if (g_max_result_count && (g_result_count >= g_max_result_count)) {
	/* Kick the event, which will stop the agent */
	bitd_event_set(g_stop_event);
    }
} 


/*
 *============================================================================
 *                        sighandler
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
static void sighandler(int signo) {

    switch (signo) {
    case SIGINT:
	ttlog(log_level_trace, g_log_keyid, "Got SIGINT\n");
	g_got_sigint = TRUE;
	break;
    case SIGTERM:
	ttlog(log_level_trace, g_log_keyid, "Got SIGTERM\n");
	g_got_sigterm = TRUE;
	break;
#ifdef SIGHUP
    case SIGHUP:
	ttlog(log_level_trace, g_log_keyid, "Got SIGHUP\n");
	g_got_sighup = TRUE;
	break;
#endif
    default:
	ttlog(log_level_trace, g_log_keyid, "Got signal %d, ignored\n", signo);
	return;
    }

    /* Kick the event */
    bitd_event_set(g_stop_event);
} 


/*
 *============================================================================
 *                        register_sig_handler
 *============================================================================
 * Description:     Register a signal handler function
 * Parameters:    
 * Returns:  
 */
void register_sig_handler(void (*f)(int)) {

    signal(SIGINT, f);
    signal(SIGTERM, f);
#ifdef SIGHUP
    signal(SIGHUP, f);
#endif

#ifdef SIGPIPE
    /* Ignore SIGPIPE. This is needed by script module. */
    signal(SIGPIPE, SIG_IGN);
#endif
} 



/*
 *============================================================================
 *                        init_load_path
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
static char *init_load_path(char *prog_name) {
    char *load_path;

    /* Set up the initial dll load path */
    load_path = bitd_get_path_dir(prog_name);
    
#if !defined(_WIN32) && !defined(__CYGWIN__)
    {
	char *load_path_base;
	char *c;

	/* The load path base is the load path minus the leaf directory name */
	load_path_base = load_path;
	
	c = strrchr(load_path_base, '/');
	if (c) {
	    *(c+1) = 0;
	} else if (!load_path_base[0] || !strcmp(load_path_base, ".")) {
	    free(load_path_base);
	    load_path_base = strdup("../");
	} else {
	    load_path_base[0] = 0;
	}

	load_path = malloc(2*strlen(load_path_base) + 
			   2*sizeof("lib") + 
			   24);

#if defined(__linux__)
	sprintf(load_path, "%slib64:%slib", load_path_base, load_path_base);
#else
	sprintf(load_path, "%slib", load_path_base);
#endif
	free(load_path_base);
    }
#endif

    return load_path;
} 

/*
 *============================================================================
 *                        main
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
int main(int argc, char **argv) {
    ttlog_level level;
    bitd_boolean init_load_path_p = TRUE;
    bitd_nvp_t config_nvp = NULL;
    bitd_nvp_t old_config_nvp = NULL;
    bitd_nvp_t mmr_config_nvp = NULL;
    FILE *f = NULL;
    bitd_value_t v;
    char *load_path = NULL; /* The dll load path */


    bitd_sys_init();
    tth_init();

    /* Parse program name argument */
    g_prog_name = bitd_get_leaf_filename(argv[0]);

    /* Initialize logger */
    ttlog_init();

    ttlog_set_log_file_name(LOG_FILE_NAME_DEF);
    ttlog_set_log_file_size(LOG_FILE_SIZE_DEF);
    ttlog_set_log_file_count(LOG_FILE_COUNT_DEF);
    
    /* Set the default log level */
    ttlog_set_level(NULL, log_level_none);

    /* Register log keys */
    g_log_keyid = ttlog_register("module-agent");
    g_log_mmr_keyid = ttlog_register("module-mgr");

    /* Create the stop event */
    g_stop_event = bitd_event_create(0);

    /* Register the signal handler */
    register_sig_handler(sighandler);

    /* Set up the initial dll load path */
    load_path = init_load_path(argv[0]);

    /* Skip to next parameter */
    argc--;
    argv++;

    /* Parse vector size and help */
    while (argc) {
	if (!strcmp(argv[0], "-lp")) {

            /* Skip to next parameter */
            argc--;
            argv++;
            
            if (!argc) {
                usage();
		exit(-1);
            }

	    if (init_load_path_p) {
		init_load_path_p = FALSE;
		if (load_path) {
		    free(load_path);
		    load_path = NULL;
		}
	    }

            /* Concatenate the path */
            load_path = bitd_envpath_add(load_path, argv[0]);

	    ttlog(log_level_trace,
		  g_log_keyid, 
		  "Dll path from command line: %s\n", load_path);

	} else if (!strcmp(argv[0], "--n-worker-threads")) {

            /* Skip to next parameter */
            argc--;
            argv++;
            
            if (!argc) {
                usage();
		exit(-1);
            }

	    /* Remove the number of threads from the config */
	    bitd_nvp_delete_elem(mmr_config_nvp, "n-worker-threads");
	    v.value_int64 = (bitd_int64)atoi(argv[0]);

	    bitd_nvp_add_elem(&mmr_config_nvp, 
			    "n-worker-threads",
			    &v,
			    bitd_type_int64);

	} else if (!strcmp(argv[0], "-c") ||
		   !strcmp(argv[0], "-cx") ||
		   !strcmp(argv[0], "-cy")) {
	    char *opt = argv[0];

            /* Skip to next parameter */
            argc--;
            argv++;
            
            if (!argc) {
                usage();
		exit(-1);
            }

	    g_config_file = argv[0];
	    
	    if (!strcmp(opt, "-cx")) {
		g_config_xml = TRUE;
		g_config_yaml = FALSE;
	    } else if (!strcmp(opt, "-cy")) {
		g_config_xml = FALSE;
		g_config_yaml = TRUE;
	    } else {
		/* Determine the config based on suffix */
		char *suffix = bitd_get_filename_suffix(g_config_file);
		
		if (!suffix || !strcmp(suffix, "yaml")) {
		    g_config_xml = FALSE;
		    g_config_yaml = TRUE;
		} else {
		    g_config_xml = TRUE;
		    g_config_yaml = FALSE;
		}
	    }

	} else if (!strcmp(argv[0], "-r")) {

            /* Skip to next parameter */
            argc--;
            argv++;
            
            if (!argc) {
                usage();
		exit(-1);
            }

	    g_result_file = argv[0];

	} else if (!strcmp(argv[0], "-rx")) {
	    g_xml_results = TRUE;
	} else if (!strcmp(argv[0], "-ry")) {
	    g_xml_results = FALSE;
	} else if (!strcmp(argv[0], "-mrc") ||
		   !strcmp(argv[0], "--max-result-count")) {

            /* Skip to next parameter */
            argc--;
            argv++;
            
            if (!argc) {
                usage();
		exit(-1);
            }

	    g_max_result_count = atoll(argv[0]);

	} else if (!strcmp(argv[0], "-l") ||
		   !strcmp(argv[0], "--log-level")) {

            /* Skip to next parameter */
            argc--;
            argv++;
            
            if (!argc) {
                usage();
		exit(-1);
            }

	    level = ttlog_get_level_from_name(argv[0]);
	    if (!level) {
		fprintf(stderr, 
			"%s: Invalid log level %s.\n", 
			g_prog_name, argv[0]);
	    }

            if (!ttlog_set_level(NULL, level)) {
		fprintf(stderr, 
			"%s: Failed to set log file level %s.\n", 
			g_prog_name, argv[0]);
		return -1;
	    }

	    if (init_load_path_p) {
		ttlog(log_level_trace,
		      g_log_keyid, 
		      "Initial dll path: %s\n", 
		      load_path);
	    }

	} else if (!strcmp(argv[0], "-lk") ||
		   !strcmp(argv[0], "--log-key-level")) {

            /* Skip to next parameter */
            argc--;
            argv++;
            
            if (argc < 2) {
                usage();
		exit(-1);
            }

	    level = ttlog_get_level_from_name(argv[1]);
	    if (!level) {
		fprintf(stderr, 
			"%s: Invalid log level %s.\n", g_prog_name, argv[1]);
	    }

	    /* Ensure the log key is registered. Registrations are reference
	       counted, so it's safe to register multiple times. Registrations
	       are freed when ttlog_deinit() is called. */
	    ttlog_register(argv[0]);

	    /* Set the log level for the key */
	    if (!ttlog_set_level(argv[0], level)) {
		fprintf(stderr, 
			"%s: Failed to set log level %s for key '%s'.\n", 
			g_prog_name, argv[1], argv[0]);
		return -1;
	    }

            /* Skip to next parameter */
            argc--;
            argv++;

	} else if (!strcmp(argv[0], "-lf") ||
		   !strcmp(argv[0], "--log-file")) {

            /* Skip to next parameter */
            argc--;
            argv++;
            
            if (!argc) {
                usage();
		exit(-1);
            }

	    ttlog_set_log_file_name(argv[0]);
	    
	} else if (!strcmp(argv[0], "-ls") ||
		   !strcmp(argv[0], "--log-file-size")) {

            /* Skip to next parameter */
            argc--;
            argv++;
            
            if (!argc) {
                usage();
		exit(-1);
            }

	    ttlog_set_log_file_size(atoi(argv[0]));
	    
	} else if (!strcmp(argv[0], "-lc") ||
		   !strcmp(argv[0], "--log-file-count")) {

            /* Skip to next parameter */
            argc--;
            argv++;
            
            if (!argc) {
                usage();
		exit(-1);
            }

	    ttlog_set_log_file_count(atoi(argv[0]));
	    
        } else if (!strcmp(argv[0], "-h") ||
                   !strcmp(argv[0], "--help") ||
                   !strcmp(argv[0], "-?")) {
            usage();
	    exit(0);
        } else {
            fprintf(stderr, "%s: Skipping invalid parameter %s\n", 
		    g_prog_name, argv[0]);
        }

        /* Skip to next argument */
        argc--;
        argv++;        
    }

    if (!g_config_file) {
	fprintf(stderr, "%s: Use -c to pass a configuration file.\n",
		g_prog_name);
	exit(-1);
    }

    /* Open the file stream */
    if (g_result_file) {
	if (!strcmp(g_result_file, "stdout") ||
	    !strcmp(g_result_file, "/dev/stdout")) {
	    g_result_fstream = stdout;
	} else {
	    g_result_fstream = fopen(g_result_file, "w");
	}
	if (!g_result_fstream) {
	    fprintf(stderr, "Could not open %s for writing: %s (errno %d)\n",
		    g_result_file, 
		    strerror(bitd_socket_errno), bitd_socket_errno);
	    exit(-1);
	}	
    }

    if (g_result_fstream) {
	if (g_xml_results) {
	    /* Write the xml header */
	    fprintf(g_result_fstream, "<?xml version='1.0'?>\n");
	    fprintf(g_result_fstream, "<nvp>\n");
	} else {
	    fprintf(g_result_fstream, "%s\n", YAML_DOC_START);
	}
    }
    
    /* Initialize the module manager */
    MMR_NOERROR(mmr_init());

    /* Set the mmr logger */
    MMR_NOERROR(mmr_set_vlog_func(&mvlog));

    /* Set the results report callback */
    MMR_NOERROR(mmr_results_register(&report_results));

    /* Set the config */
    MMR_NOERROR(mmr_set_config(mmr_config_nvp));
    
    /* Set the mmr path */
    MMR_NOERROR(mmr_set_module_path(load_path));

 reload_config:
    /* Open the config file stream, or use stdin */
    if (!strcmp(g_config_file, "stdin")) {
	f = stdin;
    } else {
	f = fopen(g_config_file, "r");
    }
    if (!f) {
	fprintf(stderr, "%s: Could not open %s: errno %d (%s).\n",
		g_prog_name, g_config_file, 
		bitd_socket_errno, strerror(bitd_socket_errno));
	exit(-1);
    }

    /* Move the old config into config_nvp_old */
    old_config_nvp = config_nvp;
	
    /* Parse the configuration */
    if (g_config_xml) {
	config_nvp = parse_config_xml(f);
    } else if (g_config_yaml) {
	config_nvp = parse_config_yaml(f);
    }

    /* Close the config file stream, if it's not stdin */
    if (f != stdin) {
	fclose(f);
    }

    if (!config_nvp) {
	exit(-1);	    
    }

    /* Set the test instance config */
    set_task_inst_config(config_nvp);

    /* Clear the old config, if any */
    clear_test_inst_config(old_config_nvp);
    bitd_nvp_free(old_config_nvp);
    old_config_nvp = NULL;

    /* Wait for the stop event */
    bitd_event_wait(g_stop_event, BITD_FOREVER);
    if (g_got_sighup) {
	/* Reload the config */
	ttlog(log_level_info,
	      g_log_keyid, 
	      "Reloading the configuration\n");
	g_got_sighup = FALSE;
	goto reload_config;
    }

    /* Unset the test instance config */
    clear_test_inst_config(config_nvp);

    /* Deinitialize the mmr */
    mmr_deinit();

    /* Unregister the signal handler */
    register_sig_handler(NULL);
    
    /* Destroy the stop event */
    bitd_event_destroy(g_stop_event);

    /* Unregister log keys */
    ttlog_unregister(g_log_keyid);
    ttlog_unregister(g_log_mmr_keyid);

    /* Deinitialize logger */
    ttlog_deinit(FALSE);

    tth_deinit();
    bitd_sys_deinit();

    if (load_path) {
	free(load_path);
    }    

    /* Release the config nvp */
    bitd_nvp_free(config_nvp);
    bitd_nvp_free(old_config_nvp);
    bitd_nvp_free(mmr_config_nvp);

    if (g_result_fstream) {
	if (g_xml_results) {
	    /* Write the xml trailer */
	    fprintf(g_result_fstream, "</nvp>\n");
	} else {
	    fprintf(g_result_fstream, "%s\n", YAML_STREAM_END);
	}
    }

    if (g_result_fstream != stdout) {
	fclose(g_result_fstream);
    }

#ifdef __linux__
    /* Clean up the main thread state so we don't see valgrind leaks.
       We don't move this to bitd_sys_deinit() because it actually causes
       the main thread to exit. */
    pthread_exit(NULL);
#endif

    return 0;
} 


