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
#include "bitd/log.h"
#include "bitd/msg.h"
#include <time.h>
#include <sys/timeb.h>

/*****************************************************************************
 *                             MANIFEST CONSTANTS
 *****************************************************************************/



/*****************************************************************************
 *                                  MACROS 
 *****************************************************************************/
#define LOG_SIZE_MAX_DEF 1024*1024
#define LOG_COUNT_DEF 3

#define bitd_printf if(0) printf

/*****************************************************************************
 *                                  TYPES
 *****************************************************************************/

#define KEYID_MAGIC 0x2239

struct ttlog_keyid_s {
    int magic;
    struct ttlog_keyid_s *next;
    struct ttlog_keyid_s *prev;
    char *name;
    ttlog_level level;
    int refcount;
};

struct log_cb_s {
    bitd_mutex lock;
    ttlog_level level;
    bitd_thread th;          /* The event loop */
    bitd_queue q;            /* The event loop queue */
    int log_size;
    int log_size_max;
    int log_count;
    FILE *f;               /* Log file handler */
    bitd_boolean force_stop_p; /* Logger is forced to stop immediately */

    /* Data members underneath are lock-protected */
    char *log_name;
    bitd_boolean reopen_log_file_p;
    ttlog_keyid key_head;
    ttlog_keyid key_tail;
};

#define KEY_LIST_HEAD(lcb) \
  ((ttlog_keyid)(((char *)&lcb->key_head) - \
		 offsetof(struct ttlog_keyid_s, next)))


typedef enum {
    ttlog_opcode_log_msg,
    ttlog_opcode_flush,
    ttlog_opcode_exit,
    
    ttlog_opcode_max             /* Sentinel */
} ttlog_opcode;

typedef struct {
    char buf[1024];
} ttlog_msg_buffer;

/*****************************************************************************
 *                           FUNCTION DECLARATION
 *****************************************************************************/



/*****************************************************************************
 *                                VARIABLES
 *****************************************************************************/
struct log_cb_s *g_log_cb;


/*****************************************************************************
 *                          FUNCTION IMPLEMENTATION
 *****************************************************************************/


/*
 *============================================================================
 *                        write_log_msg
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
static int write_log_msg(char *buf) {
    int nbytes = 0, ret;
    struct tm* it;
    struct timeb ibtime;

    if (!g_log_cb->f) {
	return 0;
    }

    ftime(&ibtime);
    if ((it = localtime(&ibtime.time))) {
        ret = fprintf(g_log_cb->f,
		      "%02d-%02d-%4d %02d:%02d:%02d.%03d : ",
		      it->tm_mon+1,it->tm_mday,it->tm_year + 1900,
		      it->tm_hour,it->tm_min,it->tm_sec, ibtime.millitm);
	if (ret < 0) {
	    return ret;
	}
	nbytes += ret;
    }

    ret = fprintf(g_log_cb->f, "%s", buf);
    if (ret < 0) {
	return ret;
    }

    nbytes += ret;

    fflush(g_log_cb->f);

    return nbytes;
} 


/*
 *============================================================================
 *                        ensure_log_file
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
static void ensure_log_file() {
    struct stat s;

    bitd_mutex_lock(g_log_cb->lock);

    if (g_log_cb->reopen_log_file_p) {
	/* Close the file stream */
	if (g_log_cb->f && (g_log_cb->f != stdout)) {
	    fclose(g_log_cb->f);
	    g_log_cb->f = NULL;
	}
	
	/* Reopen the file stream */
	if (!strcmp(g_log_cb->log_name, "stdout") ||
	    !strcmp(g_log_cb->log_name, "/dev/stdout")) {
	    /* Log to standard output */
	    g_log_cb->f = stdout;
	} else if (g_log_cb->log_name) {
	    /* Open the log file */
	    g_log_cb->f = fopen(g_log_cb->log_name, "a");
	    if (!g_log_cb->f) {
		/* Could not open the log file */
		fprintf(stderr, "Failed to open %s, %s (errno %d)\n",
			g_log_cb->log_name, strerror(errno), errno);
	    }
	}
    
	/* Else, we're not outputting to a log file. 
	   Leave the file stream empty. */
    }
    
    g_log_cb->reopen_log_file_p = FALSE;
  
    if (!g_log_cb->f || g_log_cb->f == stdout) {
	/* No archiving needed */
	bitd_mutex_unlock(g_log_cb->lock);
	return;
    }

    /* Archiving might be needed. What is the current file size? */
    if (!g_log_cb->log_size) {
	if (!stat(g_log_cb->log_name, &s)) {
	    g_log_cb->log_size = s.st_size;
	}
    }

    if (g_log_cb->log_size + 2048 > g_log_cb->log_size_max) {
	/* Attempt to archive the file */
	fclose(g_log_cb->f);
	g_log_cb->f = NULL;

	if (g_log_cb->log_count < 2) {
	    unlink(g_log_cb->log_name);
	} else {
	    char *fname1;
	    char *fname2;
	    int i;

	    fname1 = malloc(strlen(g_log_cb->log_name + 24));
	    fname2 = malloc(strlen(g_log_cb->log_name + 24));

	    for (i = g_log_cb->log_count - 2; i >= 0; i--) {
		if (i) {
		    sprintf(fname1, "%s.%u", g_log_cb->log_name, i-1);
		} else {
		    sprintf(fname1, "%s", g_log_cb->log_name);
		}
		sprintf(fname2, "%s.%u", g_log_cb->log_name, i);

		unlink(fname2);
		rename(fname1, fname2);
	    }

	    free(fname1);
	    free(fname2);
	}

	/* Reopen the log file */
	g_log_cb->f = fopen(g_log_cb->log_name, "a");
	if (!g_log_cb->f) {
	    /* Could not open the log file */
	    fprintf(stderr, "Failed to open %s, %s (errno %d)\n",
		    g_log_cb->log_name, strerror(errno), errno);
	}
    }

    bitd_mutex_unlock(g_log_cb->lock);
} 


/*
 *============================================================================
 *                        handle_log_msg
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
static void handle_log_msg(ttlog_msg_buffer *log_msg_buf) {
    int nbytes;

    /* Ensure file is open and archive is rotated */
    ensure_log_file();
    
    /* Log the message */
    nbytes = write_log_msg(log_msg_buf->buf);
    if (nbytes < 0) {
	/* Error writing to file - need to reopen file */
	g_log_cb->reopen_log_file_p = FALSE;
    }
    g_log_cb->log_size += nbytes;
} 

/*
 *============================================================================
 *                        logger_event_loop
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
static void logger_event_loop(void *thread_arg) {
    bitd_queue q;
    bitd_msg m;
    
    while (!g_log_cb->force_stop_p) {
	
	m = bitd_msg_receive(g_log_cb->q);
	bitd_assert(m);

	switch (bitd_msg_get_opcode(m)) {
	case ttlog_opcode_log_msg:
	    /* A log message */
	    handle_log_msg((ttlog_msg_buffer *)m);
	    break;
	case ttlog_opcode_flush:
	    /* Send the flush complete message */
	    q = *(bitd_queue *)m;
	    bitd_msg_send(m, q);
	    continue;
	case ttlog_opcode_exit:
	    /* We're exiting */
	    g_log_cb->force_stop_p = TRUE;
	    break;
	default:
	    bitd_assert(0);
	}

	bitd_msg_free(m);
    }
}


/*
 *============================================================================
 *                        ttlog_init
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
bitd_boolean ttlog_init(void) {
    
    if (!g_log_cb) {
	g_log_cb = calloc(1, sizeof(*g_log_cb));

	/* Initialize the key list */
	g_log_cb->key_head = KEY_LIST_HEAD(g_log_cb);
	g_log_cb->key_tail = KEY_LIST_HEAD(g_log_cb);

	g_log_cb->log_size_max = LOG_SIZE_MAX_DEF;
	g_log_cb->log_count = LOG_COUNT_DEF;

	/* The default level */
	g_log_cb->level = log_level_info;

	/* Create the mutex */
	g_log_cb->lock = bitd_mutex_create();

	/* Create the logger queue, with a quota of 4M  */
	g_log_cb->q = bitd_queue_create("logger", 0, 4*1024*1024);

	/* Create the event loop */
	g_log_cb->th = bitd_create_thread("logger", logger_event_loop, 
					0, 24576, NULL);
    }

    return TRUE;
}


/*
 *============================================================================
 *                        ttlog_deinit
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
void ttlog_deinit(bitd_boolean force_stop) {
    bitd_msg m;
    ttlog_keyid key;

    if (g_log_cb) {
	/* We're about to stop the logger */
	g_log_cb->force_stop_p = force_stop;

	/* Send a stop message to the queue */
	m = bitd_msg_alloc(ttlog_opcode_exit, 0);
        bitd_msg_send(m, g_log_cb->q);
	
	/* Wait for the event loop to exit */
        bitd_join_thread(g_log_cb->th);

	/* Close the queue */
	bitd_queue_destroy(g_log_cb->q);

	/* Release the file name */
	if (g_log_cb->log_name) {
	    free(g_log_cb->log_name);
	}

	/* Close the file stream */
	if (g_log_cb->f && (g_log_cb->f != stdout)) {
	    fclose(g_log_cb->f);
	    g_log_cb->f = NULL;
	}	

	/* Release the keys */
	while (g_log_cb->key_head != KEY_LIST_HEAD(g_log_cb)) { 

	    key = g_log_cb->key_head;
	    
	    /* Unchain the key */
	    key->next->prev = key->prev;
	    key->prev->next = key->next;
	    
	    /* Release the key */
	    if (key->name) {
		free(key->name);
	    }
	    free(key);
	}

	bitd_mutex_destroy(g_log_cb->lock);
	free(g_log_cb);
    }
}


/*
 *============================================================================
 *                        ttlog_set_log_file_name
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
bitd_boolean ttlog_set_log_file_name(char *log_name) {

    if (!g_log_cb) {
	return FALSE;
    }

    /* Do this inside the mutex */
    bitd_mutex_lock(g_log_cb->lock);
    
    if (g_log_cb->log_name) {
	free(g_log_cb->log_name);	
	g_log_cb->log_name = NULL;
    }
    
    if (log_name) {
	g_log_cb->log_name = strdup(log_name);
    } else {
	g_log_cb->log_name = NULL;
    }
    
    g_log_cb->reopen_log_file_p = TRUE;
    
    bitd_mutex_unlock(g_log_cb->lock);

    return TRUE;
} 


/*
 *============================================================================
 *                        ttlog_set_log_file_size
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
bitd_boolean ttlog_set_log_file_size(int log_size_max) {

    /* Parameter check */
    if (log_size_max <= 0 || !g_log_cb) {
	return FALSE;
    }

    g_log_cb->log_size_max = log_size_max;
    return TRUE;
} 


/*
 *============================================================================
 *                        ttlog_set_log_file_count
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
bitd_boolean ttlog_set_log_file_count(int log_count) {

    /* Parameter check */
    if (log_count <= 0 || !g_log_cb) {
	return FALSE;
    }

    g_log_cb->log_count = log_count;
    return TRUE;
} 


/*
 *============================================================================
 *                        ttlog_register
 *============================================================================
 * Description:     Register a key
 * Parameters:    
 * Returns:  
 */
ttlog_keyid ttlog_register(char *key_name) {
    ttlog_keyid keyid;

    if (!g_log_cb || !key_name) {
	return NULL;
    }

    /* Do this inside the mutex */
    bitd_mutex_lock(g_log_cb->lock);
    
    /* Look up the key */
    keyid = ttlog_get_keyid(key_name);

    if (keyid) {
	/* Bump the refcount and return duplicate key */
	keyid->refcount++;
	bitd_mutex_unlock(g_log_cb->lock);
	return keyid;
    }
    
    /* Else, create a new key */
    keyid = malloc(sizeof(*keyid));
    keyid->name = strdup(key_name);
    keyid->level = log_level_none;
    keyid->refcount = 1;
    keyid->magic = KEYID_MAGIC;

    /* Chain the new key to tail of list */
    keyid->prev = g_log_cb->key_tail;
    keyid->next = g_log_cb->key_tail->next;

    keyid->prev->next = keyid;
    keyid->next->prev = keyid;
        
    bitd_mutex_unlock(g_log_cb->lock);
    
    return keyid;
} 


/*
 *============================================================================
 *                        ttlog_unregister
 *============================================================================
 * Description:     Unregister a key
 * Parameters:    
 * Returns:  
 */
void ttlog_unregister(ttlog_keyid keyid) {
    
    if (!g_log_cb || !keyid) {
	return;
    }

    if (keyid->magic != KEYID_MAGIC) {
	bitd_assert(0);
	return;
    }

    if (keyid->refcount < 1) {
	bitd_assert(0);
	return;
    }

    /* Do this inside the mutex */
    bitd_mutex_lock(g_log_cb->lock);

    keyid->refcount--;
    if (!keyid->refcount) {
	/* Unchain key */
	keyid->next->prev = keyid->prev;
	keyid->prev->next = keyid->next;
	
	/* Clear the magic */
	keyid->magic = 0;

	/* Release key */
	free(keyid->name);
	free(keyid);
    }

    bitd_mutex_unlock(g_log_cb->lock);
} 


/*
 *============================================================================
 *                        ttlog_get_keyid
 *============================================================================
 * Description:     Get the keyid for a key
 * Parameters:    
 * Returns:  
 */
ttlog_keyid ttlog_get_keyid(char *key_name) {
    ttlog_keyid keyid;
    
    if (!g_log_cb || !key_name) {
	return NULL;
    }

    /* Do this inside the mutex */
    bitd_mutex_lock(g_log_cb->lock);

    /* Look up the key */
    for (keyid = g_log_cb->key_head;
	 keyid != KEY_LIST_HEAD(g_log_cb);
	 keyid = keyid->next) {
	if (!strcmp(key_name, keyid->name)) {
	    /* Bump the refcount and return duplicate key */
	    bitd_mutex_unlock(g_log_cb->lock);
	    return keyid;
	}
    }

    bitd_mutex_unlock(g_log_cb->lock);
 
    return NULL;
}


/*
 *============================================================================
 *                        ttlog_get_level
 *============================================================================
 * Description:     Get the log level for a key
 * Parameters:    
 * Returns:  
 */
ttlog_level ttlog_get_level(char *key_name) {
    ttlog_keyid keyid;

    if (!g_log_cb) {
	return 0;
    }

    keyid = ttlog_get_keyid(key_name);
    if (!keyid) {
	return g_log_cb->level;
    }

    return keyid->level;
}


/*
 *============================================================================
 *                        ttlog_set_level
 *============================================================================
 * Description:     Set the log level for a key
 * Parameters:    
 * Returns:  
 */
bitd_boolean ttlog_set_level(char *key_name, ttlog_level level) {
    ttlog_keyid keyid;

    if (!g_log_cb) {
	return FALSE;
    }

    if ((int)level < 0 || level >= log_level_max) {
	return FALSE;
    }

    if (!key_name) {
	g_log_cb->level = level;
    } else {
	keyid = ttlog_get_keyid(key_name);
	if (keyid) {
	    keyid->level = level;
	}
    }

    return TRUE;
}


/*
 *============================================================================
 *                        ttlog_flush
 *============================================================================
 * Description:     Wait for all pending log messages to be written to the log
 * Parameters:    
 * Returns:  
 */
void ttlog_flush(void) {
    bitd_queue q;
    bitd_msg m;

    q = bitd_queue_create("logger-flush", 0, 0);
    m = bitd_msg_alloc(ttlog_opcode_flush, sizeof(bitd_queue));

    /* Copy the queue id inside the message */
    *(bitd_queue *)m = q;

    /* Send the flush message */
    bitd_msg_send(m, g_log_cb->q);

    /* Wait for the flush complete message */
    m = bitd_msg_receive(q);

    /* Release allocated memory */
    bitd_msg_free(m);
    bitd_queue_destroy(q);
} 


/*
 *============================================================================
 *                        ttlog
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
int ttlog(ttlog_level level, ttlog_keyid keyid, char *format_string, ...) {
    va_list args;
    int ret;

    va_start(args, format_string);
    ret = ttvlog(level, keyid, format_string, args);
    va_end(args);

    return ret;
} 


/*
 *============================================================================
 *                        ttvlog
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
int ttvlog(ttlog_level level, ttlog_keyid keyid, char *format_string, 
	   va_list args) {
    bitd_msg m = NULL;
    int msg_size = sizeof(ttlog_msg_buffer);
    ttlog_msg_buffer *log_msg_buf;
    int size, idx;
    bitd_boolean opcode_set = FALSE;

    if (!g_log_cb || !keyid || keyid->magic != KEYID_MAGIC) {
	return -1;
    }

    /* Filter the message at the global level */
    if (!level) {
	/* Drop message without returning error */
	return 0;
    }

    if (level > g_log_cb->level) {
	if (!keyid) {
	    /* Drop message without returning error */
	    return 0;
	}
	/* Filter the message at the key level */
	if (level > keyid->level) {
	    /* Drop message without returning error */
	    return 0;
	}
    }
    
 again:
    /* (Re)allocate and send the message */
    m = bitd_msg_realloc(m, msg_size);
    if (!opcode_set) {
	bitd_msg_set_opcode(m, ttlog_opcode_log_msg);
	opcode_set = TRUE;
    }
    log_msg_buf = (ttlog_msg_buffer *)m;

    /* We might pad with "\n\0" so leave 2 bytes at the end */
    size = msg_size - 2;
    idx = 0;
    
    /* Inside the lock, because we access the key name */
    bitd_mutex_lock(g_log_cb->lock);
    idx += snprintf(log_msg_buf->buf + idx, size - idx,
		    "%s: %s: ", 
		    ttlog_get_level_name(level),
		    keyid->magic == KEYID_MAGIC ? keyid->name : "");
    bitd_mutex_unlock(g_log_cb->lock);
    
    if (idx < 0 || idx > size) {
	if (msg_size < 1024*1024) {
	    msg_size *= 2;
	} else {
	    msg_size += 1024*1024;
	}
	goto again;
    }

    /* Do this inside the mutex */
    idx += vsnprintf(log_msg_buf->buf + idx, size - idx,
		     format_string, args);
    if (idx > size) {
	msg_size *= 2;
	goto again;
    }
    
    /* Ensure newline termination */
    if (log_msg_buf->buf[idx-1] != '\n') {
	log_msg_buf->buf[idx++] = '\n';
    }
    
    /* Ensure NULL termination */
    log_msg_buf->buf[idx++] = 0;

    bitd_printf("%s", log_msg_buf->buf);

    /* Send the message */
    bitd_msg_send(m, g_log_cb->q);

    return idx;
} 


/*
 *============================================================================
 *                        ttlog_raw
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
int ttlog_raw(char *format_string, ...) {
    va_list args;
    int ret;

    va_start(args, format_string);
    ret = ttvlog_raw(format_string, args);
    va_end(args);

    return ret;
} 


/*
 *============================================================================
 *                        ttvlog_raw
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
int ttvlog_raw(char *format_string, va_list args) {
    bitd_msg m;
    ttlog_msg_buffer *log_msg_buf;
    int size, idx;

    if (!g_log_cb) {
	return -1;
    }

    /* Allocate and send the message */
    m = bitd_msg_alloc(ttlog_opcode_log_msg, sizeof(ttlog_msg_buffer));
    log_msg_buf = (ttlog_msg_buffer *)m;

    /* We might pad with "\n\0" so leave 2 bytes at the end */
    size = sizeof(log_msg_buf->buf) - 2;
    idx = 0;
    
    idx += vsnprintf(log_msg_buf->buf + idx, size - idx,
		     format_string, args);
    
    /* Ensure newline termination */
    if (log_msg_buf->buf[idx-1] != '\n') {
	log_msg_buf->buf[idx++] = '\n';
    }
    
    /* Ensure NULL termination */
    log_msg_buf->buf[idx++] = 0;

    bitd_printf("%s", log_msg_buf->buf);

    /* Send the message */
    bitd_msg_send(m, g_log_cb->q);

    return idx;
} 


/*
 *============================================================================
 *                        
 *============================================================================
 * Description:     ttlog_get_level_name
 * Parameters:    
 * Returns:  
 */
char *ttlog_get_level_name(ttlog_level level) {
    static char *s_level_name[] = {
	"NONE",
	"CRIT",
	"ERROR",
	"WARN",
	"INFO",
	"DEBUG",
	"TRACE"
    };

    if (level < sizeof(s_level_name)/sizeof(s_level_name[0])) {
	return s_level_name[level];
    }

    return NULL;
} 


/*
 *============================================================================
 *                        
 *============================================================================
 * Description:     ttlog_get_level_from_name
 * Parameters:    
 * Returns:  
 */
ttlog_level ttlog_get_level_from_name(char *level_name) {

    if (!level_name ||
	!strcasecmp(level_name, "none")) {
	return log_level_none;
    }
    if (!strcasecmp(level_name, "crit") ||
	!strcasecmp(level_name, "critical")) {
	return log_level_crit;
    }
    if (!strcasecmp(level_name, "err") ||
	!strcasecmp(level_name, "error")) {
	return log_level_err;
    }
    if (!strcasecmp(level_name, "warn") ||
	!strcasecmp(level_name, "warning")) {
	return log_level_warn;
    }
    if (!strcasecmp(level_name, "info")) {
	return log_level_info;
    }
    if (!strcasecmp(level_name, "debug")) {
	return log_level_debug;
    }
    if (!strcasecmp(level_name, "trace")) {
	return log_level_trace;
    }

    return log_level_none;
} 


/*
 *============================================================================
 *                        ttlog_get_keys
 *============================================================================
 * Description:     Get the configured filter keys
 * Parameters:    
 * Returns:  
 */
void ttlog_get_keys(char ***key_names, int *n_keys) {
    ttlog_keyid keyid;
    int i;

    if (!key_names || !n_keys) {
	return;
    }

    /* Initialize the key names array */
    *key_names = NULL;
    *n_keys = 0;

    /* Are we initialized? */
    if (!g_log_cb) {
	return;
    }
        
    bitd_mutex_lock(g_log_cb->lock);

    /* Find the number of keys */
    for (keyid = g_log_cb->key_head;
	 keyid != KEY_LIST_HEAD(g_log_cb);
	 keyid = keyid->next) {
	(*n_keys)++;
    }

    /* Construct the keys array  */
    (*key_names) = calloc(*n_keys, sizeof(char *));

    i = 0;
    for (keyid = g_log_cb->key_head;
	 keyid != KEY_LIST_HEAD(g_log_cb);
	 keyid = keyid->next) {

	(*key_names)[i] = strdup(keyid->name);
	i++;
    }

    bitd_assert(i == *n_keys);

    bitd_mutex_unlock(g_log_cb->lock);
}


/*
 *============================================================================
 *                        ttlog_free_keys
 *============================================================================
 * Description:     Free an array of key names
 * Parameters:    
 * Returns:  
 */
void ttlog_free_keys(char **key_names, int n_keys) {
    int i;

    if (!key_names) {
	return;
    }

    for (i = 0; i < n_keys; i++) {
	if (key_names[i]) {
	    free(key_names[i]);
	}
    }

    free(key_names);
}
