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
#define _GNU_SOURCE

#include "bitd/common.h"
#include "bitd/types.h"
#include "bitd/log.h"
#include "bitd/timer-thread.h"
#include "bitd/module-api.h"
#include "bitd/format.h"

#include <ctype.h>
#include <signal.h>
#ifdef BITD_HAVE_SPAWN_H
# include <spawn.h>
#endif
#ifdef BITD_HAVE_SYS_WAIT_H
# include <sys/wait.h>
#endif
#ifdef BITD_HAVE_FCNTL_H
# include <fcntl.h>
#endif

/*****************************************************************************
 *                             MANIFEST CONSTANTS
 *****************************************************************************/



/*****************************************************************************
 *                                  MACROS 
 *****************************************************************************/
#define READFD 0
#define WRITEFD 1

#if !defined(BITD_HAVE_PIPE2) 
# define pipe2(a, b) pipe(a)
#endif

/* Win32 exit code for child process when terminated on timeout */
#define EXEC_TMO_EXIT_CODE -1

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
    bitd_nvp_t args;
    bitd_nvp_t tags;
    bitd_boolean stopped_p;
    char *child_cmd;       /* Command that child executes */
    char **child_env_array;      /* The child environment */
    int child_env_array_count;
    char *child_env_str;
    int child_env_str_size, child_env_str_idx;
    bitd_buffer_type_t input_buffer_type;
    bitd_buffer_type_t output_buffer_type;
    bitd_buffer_type_t error_buffer_type;
    int child_pid;         /* PID of the child */
    bitd_boolean child_timeout;
    bitd_boolean child_killed;
    bitd_boolean child_exited;
#ifdef _WIN32
    HANDLE child_job;
    HANDLE child_process;
    HANDLE child_thread;
#endif
    int child_exit_code;   /* Exit code of the child */
    char *child_stdout;    /* Child standard output */
    char *child_stderr;    /* Child standard error */
    int child_stdout_len, child_stderr_len;
    bitd_double child_tmo;
    tth_timer_t timer;     /* Child run timer */
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

    s_log_keyid = ttlog_register("bitd-exec");

    ttlog(log_level_trace, s_log_keyid,
	  "%s:%d:%s() called", __FILE__, __LINE__, __FUNCTION__);

    tags_str = bitd_nvp_to_string(tags, "  ");
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
    s_task = mmr_task_register(mmr_module, "exec", &task_api);

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

    /* Release the module environment */
    bitd_nvp_free(g_module.tags);
    g_module.tags = NULL;

    ttlog_unregister(s_log_keyid);
} 


/*
 *============================================================================
 *                        env_escape
 *============================================================================
 * Description: Escape an environment variable name    
 * Parameters:    
 * Returns:  
 */
static void env_escape(char *name) {
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
 *                        map_env
 *============================================================================
 * Description:     Called for each node in the tags object to map the 
 *     environment into a string
 * Parameters:    
 *     node - the object node
 *     cookie - pointer to the map output
 * Returns:  
 */
static void map_env(void *cookie, 
		    char *full_name, char *name, int depth,
		    bitd_value_t *v, bitd_type_t t) {
    bitd_task_inst_t p = (bitd_task_inst_t)cookie;
    char *env_var;
    char *value_str;

    if (!full_name || !full_name[0]) {
	return;
    }

    value_str = bitd_value_to_string(v, t);

    env_escape(full_name);

    env_var = malloc(strlen(full_name) + strlen(value_str) + 2);

    /* full_name starts with _, skip that character */
    sprintf(env_var, "%s=%s", full_name+1, value_str);

#ifdef BITD_HAVE_POSIX_SPAWN
    /* Format the environment array */
    p->child_env_array = realloc(p->child_env_array, (p->child_env_array_count + 2)*sizeof(char *));
    p->child_env_array[p->child_env_array_count + 1] = NULL;

    /* Insert the new environment variable */
    p->child_env_array[p->child_env_array_count] = env_var;
    p->child_env_array_count++;
#endif

#ifdef _WIN32
    /* Format the environment string */
    snprintf_w_realloc(&p->child_env_str, 
		       &p->child_env_str_size, 
		       &p->child_env_str_idx,
		       "%s%c", env_var, 0);
#endif

    free(value_str);
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
    char *child_cmd = NULL;
    int idx;

    ttlog(log_level_trace, s_log_keyid,
	  "%s: %s() called", task_inst_name, __FUNCTION__);

    if (!bitd_nvp_lookup_elem(args, "command", &idx) ||
	args->e[idx].type != bitd_type_string) {
	ttlog(log_level_err, s_log_keyid,
	      "%s: No command parameter of type 'string'", task_inst_name);
	return NULL;
    }
    
    child_cmd = args->e[idx].v.value_string;
    if (!child_cmd) {
	child_cmd = "";
    }

    p = calloc(1, sizeof(*p));
    
    p->task_name = task_name;
    p->task_inst_name = task_inst_name;
    p->mmr_task_inst_hdl = mmr_task_inst_hdl;

    /* Save the args, tags */
    p->args = bitd_nvp_clone(args);
    p->tags = bitd_nvp_clone(tags);

    p->child_cmd = strdup(child_cmd);

    /* Log the args, tags, input parameters */
    if (p->args) {
	buf = bitd_nvp_to_string(p->args, "  ");
	ttlog(log_level_trace, s_log_keyid,
	      "%s: Args:\n%s", task_inst_name, buf);
	free(buf);
    }

    if (p->tags) {
	buf = bitd_nvp_to_string(p->tags, "  ");
	ttlog(log_level_trace, s_log_keyid,
	      "%s: Tags:\n%s", task_inst_name, buf);
	free(buf);
    }

    if (bitd_nvp_lookup_elem(args, "command-tmo", &idx)) {
	if (args->e[idx].type == bitd_type_int64) {
	    p->child_tmo = args->e[idx].v.value_int64;
	} else if (args->e[idx].type == bitd_type_uint64) {
	    p->child_tmo = args->e[idx].v.value_uint64;
	} else if (args->e[idx].type == bitd_type_double) {
	    p->child_tmo = args->e[idx].v.value_double;
	} 
    }

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

    if (bitd_nvp_lookup_elem(args, "error-type", &idx) &&
	args->e[idx].type == bitd_type_string  &&
	args->e[idx].v.value_string) {
	if (!strcmp(args->e[idx].v.value_string, "auto")) {
	    p->error_buffer_type = bitd_buffer_type_auto;
	} else if (!strcmp(args->e[idx].v.value_string, "string")) {
	    p->error_buffer_type = bitd_buffer_type_string;
	} else if (!strcmp(args->e[idx].v.value_string, "blob")) {
	    p->error_buffer_type = bitd_buffer_type_blob;
	} else if (!strcmp(args->e[idx].v.value_string, "json")) {
	    p->error_buffer_type = bitd_buffer_type_json;
	} else if (!strcmp(args->e[idx].v.value_string, "xml")) {
	    p->error_buffer_type = bitd_buffer_type_xml;
	} else if (!strcmp(args->e[idx].v.value_string, "yaml")) {
	    p->error_buffer_type = bitd_buffer_type_yaml;
	}
    }

    if (tags) {
	bitd_nvp_map(NULL, '_', 0, tags, &map_env, p);
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
    int i;

    ttlog(log_level_trace, s_log_keyid,
	  "%s: %s() called", p->task_inst_name, __FUNCTION__);

    bitd_nvp_free(p->args);
    bitd_nvp_free(p->tags);

    if (p->child_cmd) {
	free(p->child_cmd);
    }

    if (p->child_env_array) {
        for (i = 0; i < p->child_env_array_count; i++) {
            if (p->child_env_array[i]) {
                free(p->child_env_array[i]);
            }
        }
        free(p->child_env_array);
    }
    if (p->child_env_str) {
	free(p->child_env_str);
    }

    free(p);
} 


/*
 *============================================================================
 *                        strerror_last_error
 *============================================================================
 * Description:     Format the Win32 last error as string
 * Parameters:    
 *     err_buf [OUT] - where to print the error
 *     err_buf_len - the length of the passed-in buffer
 * Returns:  
 *     Pointer to the error string.
 */
#ifdef _WIN32
static char *strerror_last_error(char *err_buf, int err_buf_len) {
    DWORD errCode = GetLastError();
    char *serr, *c;

    /* Parameter check */
    if (!err_buf || !err_buf_len) {
	return "";
    }
    
    /* Initialize the OUT parameter */
    err_buf[0] = 0;

    if (!FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER|
		       FORMAT_MESSAGE_FROM_SYSTEM,
		       NULL,
		       errCode,
		       MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		       (LPTSTR)&serr,
		       0,
		       NULL)) {
	return "";
    }
    
    /* Remove any trailing newline */
    if (serr) {
	c = strrchr(serr, '\n');
	if (c && !c[1]) {
	    *c = 0;
	}
	c = strrchr(serr, '\r');
	if (c && !c[1]) {
	    *c = 0;
	}
	c = strrchr(serr, '.');
	if (c && !c[1]) {
	    *c = 0;
	}
    }

    snprintf(err_buf, err_buf_len, "%s", serr);
    LocalFree(serr);

    return err_buf;
}
#endif

#ifdef BITD_HAVE_POSIX_SPAWN
/*
 *============================================================================
 *                        cmd_spawn
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
static pid_t cmd_spawn(char *cmd, FILE **infp, FILE **outfp, FILE **errfp,
		       char **envp) {
    int p_stdin[2] = {-1, -1};
    int p_stdout[2] = {-1, -1};
    int p_stderr[2] = {-1, -1};
    pid_t pid = -1;
    posix_spawn_file_actions_t factions;
    posix_spawnattr_t fattr;
    int i;

    /* Initialize actions */
    posix_spawn_file_actions_init(&factions);

    /* Initialize attributes */
    posix_spawnattr_init(&fattr);

    if (pipe2(p_stdin, O_CLOEXEC) || 
        pipe2(p_stdout, O_CLOEXEC) || 
        pipe2(p_stderr, O_CLOEXEC)) {
        ttlog(log_level_err, s_log_keyid, 
	      "pipe2(): %s (errno %d)", 
	      strerror(errno), errno);
        goto end;
    }
    
    /* Close-on-exec flag should not be set for file descriptors
       that will be used by child process */
    fcntl(p_stdin[READFD], F_SETFD, 0);
    fcntl(p_stdout[WRITEFD], F_SETFD, 0);
    fcntl(p_stderr[WRITEFD], F_SETFD, 0);

    if (posix_spawn_file_actions_adddup2(&factions, p_stdin[READFD], 
                                         STDIN_FILENO) ||
        posix_spawn_file_actions_adddup2(&factions, p_stdout[WRITEFD], 
					 STDOUT_FILENO) ||
        posix_spawn_file_actions_adddup2(&factions, p_stderr[WRITEFD], 
					 STDERR_FILENO)) {
        ttlog(log_level_err, s_log_keyid, 
	      "Set spawn file actions: %s (errno %d)",
              strerror(errno), errno);
        goto end;
    }

    /* Set up the spawn attributes */    
    if (posix_spawnattr_setflags(&fattr, POSIX_SPAWN_SETPGROUP)) {
	ttlog(log_level_err, s_log_keyid, 
	      "spawnattr setflags: %s (errno %d)",
	      strerror(errno), errno);
	goto end;
    }
    
    /* Spawn the child */
    if (posix_spawn(&pid, "/bin/sh", &factions, &fattr,
                    (char *[]){ "sh", "-c", cmd, 0 },
                    envp) ||
        pid <= 0) {
        ttlog(log_level_err, s_log_keyid, 
	      "Child process spawn: %s (errno %d)",
              strerror(errno), errno);
        goto end;
    }

    /* Parent process continues here */
    
    ttlog(log_level_trace, s_log_keyid, "Spawned child pid %d", pid);

    /* Set up the parent end of the pipes */
    close(p_stdin[READFD]);
    if (!infp) {
        close(p_stdin[WRITEFD]);
    } else {
        *infp = fdopen(p_stdin[WRITEFD], "w");
    }

    close(p_stdout[WRITEFD]);
    if (!outfp) {
        close(p_stdout[READFD]);
    } else {
        *outfp = fdopen(p_stdout[READFD], "r");
    }

    close(p_stderr[WRITEFD]);
    if (!errfp) {
        close(p_stderr[READFD]);
    } else {
        *errfp = fdopen(p_stderr[READFD], "r");
    }

    /* Wait for child to acquire own process group ID. */
    for (i = 0; i < 10; i++) {
        bitd_sleep(10);
        if (getpgid(pid) != getpid()) {
            if (i >= 1) {
		ttlog(log_level_info, s_log_keyid, 
		      "Process group ID changed after %d cycles", i);
            }
            break;
        }
    }
    
    if (getpgid(pid) == getpid()) {
        ttlog(log_level_err, s_log_keyid, 
	      "Process group ID still not changed after 100 millisecs");
        kill(pid, SIGKILL);
        pid = 0;
    }

end:    
    if (pid <= 0) {
        if (p_stdin[READFD] != -1) {
            close(p_stdin[READFD]);
        }
        if (p_stdin[WRITEFD] != -1) {
            close(p_stdin[WRITEFD]);
        }
        if (p_stdout[READFD] != -1) {
            close(p_stdout[READFD]);
        }
        if (p_stdout[WRITEFD] != -1) {
            close(p_stdout[WRITEFD]);
        }
        if (p_stderr[READFD] != -1) {
            close(p_stderr[READFD]);
        }
        if (p_stderr[WRITEFD] != -1) {
            close(p_stderr[WRITEFD]);
        }
    }

    posix_spawn_file_actions_destroy(&factions);
    posix_spawnattr_destroy(&fattr);

    return pid;
}
#endif /* BITD_HAVE_POSIX_SPAWN */


/*
 *============================================================================
 *                        child_kill
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
static void child_kill(bitd_task_inst_t p) {
    int i;

    if (!p->child_timeout) {
	p->child_killed = TRUE;
    }
    
    /* Wait until the pid is set */
    for (i = 0; i < 100; i++) {
	if (!p->child_pid) {
	    bitd_sleep(10);
	} else {
	    break;
	}
    }
    
    if (!p->child_pid) {
	ttlog(log_level_trace, s_log_keyid,
		  "%s:%s(): Child pid is 0, giving up", 
	      p->task_inst_name, __FUNCTION__);
	return;
    }

#ifdef BITD_HAVE_POSIX_SPAWN
    {
	pid_t pgrp;
	int ret = -1;

	/* Kill the process group of the child - which should kill
	   all subbprocesses spawned except if they left the 
	   group.  */
	pgrp = getpgid(p->child_pid);
	if (pgrp > 0) {
	    ret = kill(-pgrp, SIGKILL);
	}
	if (!ret) {
	    ttlog(log_level_trace, s_log_keyid,
		  "%s: Killed child %d", p->task_inst_name, p->child_pid);
	} else {
	    ttlog(log_level_trace, s_log_keyid,
		  "%s: Could not kill child %d, %s (errno %d)", 
		  p->task_inst_name, p->child_pid, strerror(errno), errno);
	}
    }    
#endif

#ifdef _WIN32
    if (p->child_process) {
	TerminateProcess(p->child_process, EXEC_TMO_EXIT_CODE);
    }
    if (p->child_job) {
	TerminateJobObject(p->child_job, EXEC_TMO_EXIT_CODE);
    }
#endif
} 


/*
 *============================================================================
 *                        cmd_timeout_handler
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
static void cmd_timeout_handler(void *cookie) {
    bitd_task_inst_t p = (bitd_task_inst_t)cookie;

    p->child_timeout = TRUE;

    child_kill(p);
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
    mmr_task_inst_results_t results;
    char *input_buf = NULL;
    int input_buf_len = 0;
    char *output_buf = NULL;
    int output_buf_len = 10240, len;
#ifdef BITD_HAVE_POSIX_SPAWN
    FILE *f_in = NULL, *f_out = NULL, *f_err = NULL;
    int child_exit_status = 0;
    int ret;
#endif
#ifdef _WIN32
    HANDLE g_hChildStd_IN_Rd = NULL;
    HANDLE g_hChildStd_IN_Wr = NULL;
    HANDLE g_hChildStd_OUT_Rd = NULL;
    HANDLE g_hChildStd_OUT_Wr = NULL;
    HANDLE g_hChildStd_ERR_Rd = NULL;
    HANDLE g_hChildStd_ERR_Wr = NULL;
    SECURITY_ATTRIBUTES saAttr; 
    PROCESS_INFORMATION piProcInfo; 
    STARTUPINFO siStartInfo;
    BOOL bSuccess = FALSE; 
    char *szCmdline = NULL;
    DWORD dwWritten, dwRead, dwExitCode;
    int err_buf_len = 1024;
    char *err_buf = malloc(err_buf_len);
#endif

    ttlog(log_level_trace, s_log_keyid,
	  "%s: %s() called", p->task_inst_name, __FUNCTION__);

    bitd_object_to_buffer(&input_buf, &input_buf_len,
			  input,
			  NULL,
			  p->input_buffer_type);

    if (input_buf && input_buf_len) {
	register char *c;
	register int len = 0;

	for (c = input_buf; *c; c++) {
	    len++;
	    if (len >= input_buf_len) {
		break;
	    }
	}

	if (len == input_buf_len) {
	    ttlog(log_level_trace, s_log_keyid,
		  "%s: Input:\n%s", p->task_inst_name, input_buf);
	} else {
	    ttlog(log_level_trace, s_log_keyid,
		  "%s: Raw input", p->task_inst_name);
	}
    }

    memset(&results, 0, sizeof(results));
    output_buf = malloc(output_buf_len);

    /* Initialize task control block */
    p->child_pid = 0;
    p->child_timeout = FALSE;
    p->child_exited = FALSE;
    p->child_exit_code = 0;
#ifdef _WIN32
    p->child_job = NULL;
    p->child_process = NULL;
    p->child_thread = NULL;
#endif

    if (p->child_tmo) {
	/* Set up an execution timer */
	p->timer = tth_timer_set_nsec((bitd_uint64)(p->child_tmo * 1000000000ULL),
				      cmd_timeout_handler,
				      p,
				      FALSE);
    }

#ifdef BITD_HAVE_POSIX_SPAWN
    p->child_pid = cmd_spawn(p->child_cmd, 
			     &f_in, &f_out, &f_err, 
			     p->child_env_array);

    if (p->child_pid < 0) {
	ttlog(log_level_err, s_log_keyid,
	      "Failed to spawn child process");
	goto end;
    }

    /* Write the standard input, if any. Note that SIGPIPE is ignored by
       the module-testapp, else we would have been interrupted 
       in this call if the child exited from under us. */
    if (input_buf) {
	fwrite(input_buf, input_buf_len, 1, f_in);
    }
    
    /* Close the f_in end of the pipe, so child gets EOF */
    fclose(f_in);
    f_in = NULL;

    /* Parse the child stdout */
    if (f_out) {
	for (;;) {
	    len = fread(output_buf, 1, output_buf_len - 1, f_out);
	    if (!len) {
		break;
	    }
	    p->child_stdout = realloc(p->child_stdout, 
				      p->child_stdout_len + len + 1);
	    memcpy(p->child_stdout + p->child_stdout_len, output_buf, len);
	    p->child_stdout_len += len;
	    p->child_stdout[p->child_stdout_len] = 0;
	}
    }

    if (p->child_stdout_len) {
	ttlog(log_level_trace, s_log_keyid,
	      "child stdout len: %d",
	      p->child_stdout_len);
    }

    /* Parse the child stderr */
    if (f_err) {
	for (;;) {
	    len = fread(output_buf, 1, output_buf_len - 1, f_err);
	    if (!len) {
		break;
	    }
	    p->child_stderr = realloc(p->child_stderr, 
				      p->child_stderr_len + len + 1);
	    memcpy(p->child_stderr + p->child_stderr_len, output_buf, len);
	    p->child_stderr_len += len;
	    p->child_stderr[p->child_stderr_len] = 0;
	}
    }

    if (p->child_stderr_len) {
	ttlog(log_level_trace, s_log_keyid,
	      "child stderr len: %d",
	      p->child_stderr_len);
    }

    /* Wait for child to exit, and save the exit code of the child */
    ret = waitpid(p->child_pid, &child_exit_status, 0);
    ttlog(log_level_trace, s_log_keyid,
	  "waitpid() return code %d, child exit status 0x%x, %sexit code %d",
	  ret, child_exit_status, 
	  (WIFSIGNALED(child_exit_status) ? "killed by signal, ": ""),
	  WEXITSTATUS(child_exit_status));
    
    p->child_exit_code = WEXITSTATUS(child_exit_status);
    p->child_exited = TRUE;
    p->child_pid = 0;
#endif /* BITD_HAVE_POSIX_SPAWN */

#ifdef _WIN32
    /* Set the bInheritHandle flag so pipe handles are inherited */
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES); 
    saAttr.bInheritHandle = TRUE; 
    saAttr.lpSecurityDescriptor = NULL; 

    /* Create anonymous stdin pipe */
    if (!CreatePipe(&g_hChildStd_IN_Rd, &g_hChildStd_IN_Wr, &saAttr, 0))  {
	ttlog(log_level_err, s_log_keyid,
	      "CreatePipe() error %d (%s)", 
	      GetLastError(), strerror_last_error(err_buf, err_buf_len));
	goto end;
    }

    /* Ensure the write handle to the pipe for STDIN is not inherited. */
    if (!SetHandleInformation(g_hChildStd_IN_Wr, HANDLE_FLAG_INHERIT, 0)) {
	ttlog(log_level_err, s_log_keyid,
	      "SetHandleInformation() error %d (%s)", 
	      GetLastError(), strerror_last_error(err_buf, err_buf_len));
	goto end;
    }

    /* Create anonymous stdout pipe */
    if (!CreatePipe(&g_hChildStd_OUT_Rd, &g_hChildStd_OUT_Wr, &saAttr, 0))  {
	ttlog(log_level_err, s_log_keyid,
	      "CreatePipe() error %d (%s)", 
	      GetLastError(), strerror_last_error(err_buf, err_buf_len));
	goto end;
    }

    /* Ensure the read handle to the pipe for STDOUT is not inherited. */
    if (!SetHandleInformation(g_hChildStd_OUT_Rd, HANDLE_FLAG_INHERIT, 0)) {
	ttlog(log_level_err, s_log_keyid,
	      "SetHandleInformation() error %d (%s)", 
	      GetLastError(), strerror_last_error(err_buf, err_buf_len));
	goto end;
    }

    /* Create anonymous stderr pipe */
    if (!CreatePipe(&g_hChildStd_ERR_Rd, &g_hChildStd_ERR_Wr, &saAttr, 0))  {
	ttlog(log_level_err, s_log_keyid,
	      "CreatePipe() error %d (%s)", 
	      GetLastError(), strerror_last_error(err_buf, err_buf_len));
	goto end;
    }

    /* Ensure the read handle to the pipe for STDERR is not inherited. */
    if (!SetHandleInformation(g_hChildStd_ERR_Rd, HANDLE_FLAG_INHERIT, 0)) {
	ttlog(log_level_err, s_log_keyid,
	      "SetHandleInformation() error %d (%s)", 
	      GetLastError(), strerror_last_error(err_buf, err_buf_len));
	goto end;
    }

    /* Create child process job */
    p->child_job = CreateJobObject(&saAttr, NULL);
    if (!p->child_job) {
	ttlog(log_level_err, s_log_keyid,
	      "CreateJobObject() error %d (%s)", 
	      GetLastError(), strerror_last_error(err_buf, err_buf_len));
	goto end;
    }

    /* Set up the command line */
    szCmdline = malloc(strlen(p->child_cmd) + 24);
    sprintf(szCmdline, "cmd /C \"%s\"", p->child_cmd);

    memset(&piProcInfo, 0, sizeof(piProcInfo));
    memset(&siStartInfo, 0, sizeof(siStartInfo));

    siStartInfo.cb = sizeof(STARTUPINFO); 
    siStartInfo.hStdError = g_hChildStd_ERR_Wr;
    siStartInfo.hStdOutput = g_hChildStd_OUT_Wr;
    siStartInfo.hStdInput = g_hChildStd_IN_Rd;
    siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

    /* Create the child process. */
    bSuccess = CreateProcess(NULL, 
			     szCmdline,     /* command line */
			     NULL,          /* proc security attr */
			     NULL,          /* primary thread security attr */ 
			     TRUE,          /* handles are inherited */
			     CREATE_SUSPENDED, /* creation flags */
			     p->child_env_str, /* child environment */
			     NULL,          /* use parent's cwd */
			     &siStartInfo,  /* STARTUPINFO pointer */
			     &piProcInfo);  /* receives PROCESS_INFORMATION */
    
    if (!bSuccess) {
	ttlog(log_level_err, s_log_keyid,
	      "CreateProcess() error %d (%s)", 
	      GetLastError(), strerror_last_error(err_buf, err_buf_len));
	goto end;
    }

    /* Save the child process and child thread handles */
    p->child_process = piProcInfo.hProcess;
    p->child_thread = piProcInfo.hThread;

    /* Get the child process id */
    p->child_pid = (int)GetProcessId(p->child_process);

    /* Assign the child process to its job */
    if (!AssignProcessToJobObject(p->child_job, p->child_process)) {
	ttlog(log_level_err, s_log_keyid,
	      "AssignProcessToJobObject() error %d (%s)", 
	      GetLastError(), strerror_last_error(err_buf, err_buf_len));
	goto end;
    }

    /* Start the child process thread */
    if (ResumeThread(p->child_thread) == -1) {
	ttlog(log_level_err, s_log_keyid,
	      "ResumeThread() error %d (%s)", 
	      GetLastError(), strerror_last_error(err_buf, err_buf_len));
	goto end;
    }

    /* Close the parent end of the pipes */
    CloseHandle(g_hChildStd_IN_Rd);
    g_hChildStd_IN_Rd = NULL;
    CloseHandle(g_hChildStd_OUT_Wr);
    g_hChildStd_OUT_Wr = NULL;
    CloseHandle(g_hChildStd_ERR_Wr);
    g_hChildStd_ERR_Wr = NULL;

    /* Write the stdin */
    if (input_buf) {
	bSuccess = WriteFile(g_hChildStd_IN_Wr, 
			     input_buf, input_buf_len, 
			     &dwWritten, NULL);
	if (!bSuccess) {
	    ttlog(log_level_trace, s_log_keyid,
		  "WriteFile() returns %d (%s)", 
		  GetLastError(), strerror_last_error(err_buf, err_buf_len));
	} else {
	    ttlog(log_level_trace, s_log_keyid,
		  "WriteFile() wrote %d bytes", dwWritten);
	}
    }

    /* Close the stdin write end of the pipe, so child stops reading */
    CloseHandle(g_hChildStd_IN_Wr);
    g_hChildStd_IN_Wr = NULL;

    /* Read the stdout */
    SetLastError(0);
    for (;;) {
	bSuccess = PeekNamedPipe(g_hChildStd_OUT_Rd, NULL, 0, NULL, &dwRead, NULL);
	if (!bSuccess) {
	    ttlog(log_level_trace, s_log_keyid,
		  "PeekNamedPipe(g_hChildStd_OUT_Rd) last error %d (%s)", 
		  GetLastError(), strerror_last_error(err_buf, err_buf_len));
	    break; 
	}
	
	ttlog(log_level_trace, s_log_keyid,
	      "PeekNamedPipe(g_hChildStd_OUT_Rd) dwRead %d", 
	      dwRead);

	if (!dwRead) {
	    bitd_sleep(20);
	    continue;
	}

	bSuccess = ReadFile(g_hChildStd_OUT_Rd, 
			    output_buf, output_buf_len, 
			    &dwRead, NULL);
	if (!bSuccess || !dwRead) {
	    break; 
	}

	ttlog(log_level_trace, s_log_keyid,
	      "ReadFile(g_hChildStd_OUT_Rd) dwRead %d", 
	      dwRead);

	len = dwRead;

	p->child_stdout = realloc(p->child_stdout, 
				  p->child_stdout_len + len + 1);
	memcpy(p->child_stdout + p->child_stdout_len, output_buf, len);
	p->child_stdout_len += len;
	p->child_stdout[p->child_stdout_len] = 0;	
    }
    
    /* Read the stderr */
    SetLastError(0);
    for (;;) {
	bSuccess = PeekNamedPipe(g_hChildStd_ERR_Rd, NULL, 0, NULL, &dwRead, NULL);
	if (!bSuccess) {
	    ttlog(log_level_trace, s_log_keyid,
		  "PeekNamedPipe(g_hChildStd_ERR_Rd) last error %d (%s)", 
		  GetLastError(), strerror_last_error(err_buf, err_buf_len));
	    break; 
	}
	
	ttlog(log_level_trace, s_log_keyid,
	      "PeekNamedPipe(g_hChildStd_ERR_Rd) dwRead %d", 
	      dwRead);

	if (!dwRead) {
	    bitd_sleep(20);
	    continue;
	}

	bSuccess = ReadFile(g_hChildStd_ERR_Rd, 
			    output_buf, output_buf_len, 
			    &dwRead, NULL);
	if (!bSuccess || !dwRead) {
	    break; 
	}

	ttlog(log_level_trace, s_log_keyid,
	      "ReadFile(g_hChildStd_ERR_Rd) dwRead %d", 
	      dwRead);

	len = dwRead;

	p->child_stderr = realloc(p->child_stderr, 
				  p->child_stderr_len + len + 1);
	memcpy(p->child_stderr + p->child_stderr_len, output_buf, len);
	p->child_stderr_len += len;
	p->child_stderr[p->child_stderr_len] = 0;	
    }

    /* Wait for the child to exit */
    WaitForSingleObject(p->child_process, 0);

    /* Get the child process exit code */
    dwExitCode = 0;
    if (!GetExitCodeProcess(p->child_process, &dwExitCode)) {
	ttlog(log_level_trace, s_log_keyid,
	      "GetExitCodeProcess() last error %d (%s)", 
	      GetLastError(), strerror_last_error(err_buf, err_buf_len));
	goto end; 
    }

    p->child_exit_code = (int)dwExitCode;

    /* Close the handles of the child process and 
       its primary thread */
    CloseHandle(p->child_process);
    p->child_process = NULL;
    CloseHandle(p->child_thread);
    p->child_thread = NULL;
#endif

    /* Destroy the timer */
    tth_timer_destroy(p->timer);
    p->timer = NULL;

    if (!p->child_killed) {
	/* 
	 * Format the results
	 */

	/* Get the child exit code */
	results.exit_code = p->child_exit_code;

	if (p->child_timeout) {
	    ttlog(log_level_warn, s_log_keyid,
		  "%s: Child timed out after %*g second(s)", 
		  p->task_inst_name, 
		  bitd_double_precision(p->child_tmo),
		  p->child_tmo);

	    if (!results.exit_code) {
		/* Change the exit code to error */
		results.exit_code = -1;
	    }
	}
	
	if (p->child_stdout_len) {
	    /* We have standard output. */
	    bitd_buffer_to_object(&results.output, NULL, 
				  p->child_stdout, p->child_stdout_len,
				  p->output_buffer_type);
	}
	
	if (p->child_stderr_len) {
	    /* We have standard error. */
	    bitd_buffer_to_object(&results.error, NULL, 
				  p->child_stderr, p->child_stderr_len,
				  p->error_buffer_type);
	}

	/* Report the results */
	mmr_task_inst_report_results(p->mmr_task_inst_hdl, &results);    

	bitd_object_free(&results.output);
	bitd_object_free(&results.error);
    }

 end:
    if (p->child_stdout) {
	free(p->child_stdout);
	p->child_stdout = NULL;
	p->child_stdout_len = 0;
    }
    if (p->child_stderr) {
	free(p->child_stderr);
	p->child_stderr = NULL;
	p->child_stderr_len = 0;
    }

    p->child_pid = 0;
    p->child_killed = FALSE;
    p->child_timeout = FALSE;    

    if (input_buf) {
	free(input_buf);
    }

    if (output_buf) {
	free(output_buf);
    }

#ifdef BITD_HAVE_POSIX_SPAWN
    /* Close the in, out, err streams */
    if (f_in) {
	fclose(f_in);
    } 
    if (f_out) {
	fclose(f_out);
    } 
    if (f_err) {
	fclose(f_err);
    }
#endif

#ifdef _WIN32
    if (szCmdline) {
	free(szCmdline);
    }
    if (g_hChildStd_IN_Rd) {
	CloseHandle(g_hChildStd_IN_Rd);
    }
    if (g_hChildStd_IN_Wr) {
	CloseHandle(g_hChildStd_IN_Wr);
    }
    if (g_hChildStd_OUT_Rd) {
	CloseHandle(g_hChildStd_OUT_Rd);
    }
    if (g_hChildStd_OUT_Wr) {
	CloseHandle(g_hChildStd_OUT_Wr);
    }
    if (g_hChildStd_ERR_Rd) {
	CloseHandle(g_hChildStd_ERR_Rd);
    }
    if (g_hChildStd_ERR_Wr) {
	CloseHandle(g_hChildStd_ERR_Wr);
    }
    if (p->child_process) {
	TerminateProcess(p->child_process, 0);
	CloseHandle(p->child_process);
	p->child_process = NULL;
    }
    if (p->child_thread) {
	CloseHandle(p->child_thread);
	p->child_thread = NULL;
    }
    if (p->child_job) {
	TerminateJobObject(p->child_job, 0);
	CloseHandle(p->child_job);
	p->child_job = NULL;
    }
    
    if (err_buf) {
	free(err_buf);
    }
#endif

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
    child_kill(p);
} 
