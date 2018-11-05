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

#ifndef _MODULE_MGR_H_
#define _MODULE_MGR_H_

/*****************************************************************************
 *                                INCLUDE FILES 
 *****************************************************************************/
#include "bitd/common.h"
#include "bitd/module-api.h"
#include "bitd/mmr-api.h"
#include "bitd/log.h"
#include "bitd/file.h"
#include "bitd/timer-list.h"
#include "bitd/lambda.h"


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

/* Bit ops */
#define SET_BIT(var, bit)        (var) |=  (bit)
#define CLR_BIT(var, bit)        (var) &= ~(bit)
#define TOGGLE_BIT(var, bit)     (var) ^=  (bit)
#define IS_SET(var, bit)         ((var) & (bit))
#define BITVAL(bit)              (1 << (bit))

#define TASK_INST_MAGIC 0xabbacddc

/*****************************************************************************
 *                                  TYPES
 *****************************************************************************/

/* Prototypes for module load & unload symbols */
typedef bitd_boolean (bitd_module_load_t)(mmr_module_t mmr_module,
				      bitd_nvp_t tags,
				      char *module_dir);
typedef void (bitd_module_unload_t)(void);

/* Sanity check for symbols defined in module-api.h */
bitd_module_load_t bitd_module_load;
bitd_module_unload_t bitd_module_unload;

/* The module manager control block */
struct mmr_cb {
    bitd_mutex lock;         /* Protects mmr_cb data and sub-data. */
    bitd_mutex mmr_api_lock; /* Protects mmr-api calls. Can take lock. */
    bitd_mutex results_lock; /* Protects report_results */
    bitd_nvp_t config;  /* The mmr configuration vector */
    bitd_nvp_t tags;    /* The global tags */
    mmr_vlog_func_t *vlog; /* The mmr logger */
    char *module_path;
    struct mmr_module_s *module_head; /* List of modules */
    struct mmr_module_s *module_tail;
    struct mmr_task_inst_s *triggered_head; /* List of triggered task insts */
    struct mmr_task_inst_s *triggered_tail; 
    long input_queue_size;
    long input_queue_max;
    long input_queue_dropped;
    bitd_boolean stopping_p;            /* The module manager is stopping */
    bitd_thread event_loop_th;          /* The event loop thread */
    bitd_event event_loop_ev;           /* Wakes up the event loop */
    bitd_event task_inst_stopped_ev;    /* Set when a task instance has stopped */
    bitd_timer_list timers;
    bitd_lambda_handle lambda;
    mmr_report_results_t *report_results; /* Results reporting callback */
};

#define MMR_MODULE_HEAD(m) \
    ((struct mmr_module_s *)&(m)->module_head)
    
/* The module control block */
struct mmr_module_s {
    struct mmr_module_s *next; /* The list of modules */
    struct mmr_module_s *prev;
    char *name; /* Module name */
    char *dll_name;      /* The dll name */
    char *dll_full_name; /* The dll full path */
    char *dll_dir;       /* The dll directory */
    bitd_dll dll_hdl;      /* The dll handle */    
    int refcount;
    struct mmr_task_s *task_head; /* The list of task types */
    struct mmr_task_s *task_tail;
    bitd_module_load_t *load; /* Module load-unload symbols */
    bitd_module_unload_t *unload; /* Module load-unload symbols */
    bitd_boolean stopping_p;      /* The module is being unloaded */
};

#define MODULE_TASK_HEAD(m) \
    ((struct mmr_task_s *)&(m)->task_head)

/* Task control structure */
struct mmr_task_s {
    struct mmr_task_s *next;     /* The list of tasks owned by a module */
    struct mmr_task_s *prev;
    char *name;
    bitd_task_api_t api;     /* The task image api */
    struct mmr_module_s *module; /* The owning module */
    struct mmr_task_inst_s *task_inst_head; /* The list of task instances */
    struct mmr_task_inst_s *task_inst_tail;
    int refcount;
    bitd_boolean stopping_p;       /* The task is destroyed */
};

/* Task instance input queue - used by triggered tests, which need to serialize
   the input they get from their triggers, since task instances can't
   have the run method called multiple times in parallel */
struct input_queue_s {
    struct input_queue_s *next;
    struct input_queue_s *prev;
    bitd_object_t input;
};

#define TASK_INST_HEAD(t) \
    ((struct mmr_task_inst_s *)&(t)->task_inst_head)

#define TRIGGERED_HEAD(t) \
    ((struct mmr_task_inst_s *)((char *)&(t)->triggered_head - offsetof(struct mmr_task_inst_s, triggered_next)))

#define INPUT_QUEUE_HEAD(t) \
    ((struct input_queue_s *)&(t)->input_queue_head)

#define TASK_INST_SCHEDULED 0x01      /* timer installed or expired */
#define TASK_INST_PENDING_RUN 0x02    /* lambda function called for run */
#define TASK_INST_RUNNING 0x04        /* run method executing */
#define TASK_INST_PARAMS_CHANGED 0x08
#define TASK_INST_SCHED_CHANGED 0x10

typedef enum {
    task_inst_sched_none_t,
    task_inst_sched_periodic_t,
    task_inst_sched_random_t,
    task_inst_sched_once_t,
    task_inst_sched_config_t,
    task_inst_sched_triggered_t,
    task_inst_sched_triggered_raw_t
} mmr_task_inst_sched_type;

/* Task inst control structure */
struct mmr_task_inst_s {
    struct mmr_task_inst_s *next; /* The list of task types for a module */
    struct mmr_task_inst_s *prev;
    int magic;                    /* Set to TASK_INST_MAGIC */
    char *name;
    struct mmr_task_s *task;      /* The owning task */
    bitd_task_inst_t user_task_inst;
    bitd_nvp_t sched;
    mmr_task_inst_params_t params;
    int refcount;
    int state;
    bitd_timer run_timer;
    bitd_uint64 run_interval_nsec;
    bitd_uint64 next_run_nsec;      /* Next run time slot (non-randomized) */
    mmr_task_inst_sched_type sched_type; /* Periodic, random, once, ... */
    struct mmr_task_inst_s *triggered_next; /* List of triggered task inst */
    struct mmr_task_inst_s *triggered_prev;
    struct input_queue_s *input_queue_head; /* Serialized input for */
    struct input_queue_s *input_queue_tail; /* triggered task instances */
    bitd_uint64 run_id;          /* Counter for task instance runs */
    bitd_uint64 run_tstamp_ns;   /* Result timestamp for last results */
    bitd_boolean stopping_p;     /* The task instance is destroyed */
};

/* The global mmr control block pointer */
struct mmr_cb *g_mmr_cb;

/*****************************************************************************
 *                            FUNCTION DEFINITIONS
 *****************************************************************************/

/* Lock/unlock the g_mmr_cb->mmr_api_lock and, inside that, the 
   g_mmr_cb->lock. Used by all mmr apis. */
void mmr_api_lock();
void mmr_api_unlock();

void mmr_event_loop(void *thread_arg);
struct mmr_module_s *mmr_module_get(char *module_name,
				    mmr_err_t *err);
struct mmr_module_s *mmr_module_find(char *module_name);
void mmr_module_stop(struct mmr_module_s *module);
bitd_boolean mmr_module_is_stopped(struct mmr_module_s *module);
void mmr_module_release(struct mmr_module_s *module);

struct mmr_task_s *mmr_task_get(mmr_module_t module,
				char *task_name,
				bitd_task_api_t *task_api);
struct mmr_task_s *mmr_task_find(char *task_name);
void mmr_task_stop(struct mmr_task_s *task);
bitd_boolean mmr_task_is_stopped(struct mmr_task_s *task);
void mmr_task_release(struct mmr_task_s *task);

struct mmr_task_inst_s *mmr_task_inst_get(struct mmr_task_s *task,
					  char *task_inst_name);
struct mmr_task_inst_s *mmr_task_inst_find(struct mmr_task_s *task,
					   char *task_inst_name);
mmr_err_t mmr_task_inst_update(struct mmr_task_inst_s *task_inst);
void mmr_task_inst_stop(struct mmr_task_inst_s *task_inst);
bitd_boolean mmr_task_inst_is_stopped(struct mmr_task_inst_s *task_inst);
void mmr_task_inst_release(struct mmr_task_inst_s *task_inst);

void mmr_schedule_task_inst(struct mmr_task_inst_s *task_inst);
bitd_boolean mmr_schedule_triggers(mmr_task_inst_t task_inst,
				   mmr_task_inst_results_t *r);
void mmr_task_inst_run_timer_expired(bitd_timer t, void *cookie);
void mmr_task_inst_run(void *cookie, bitd_boolean *stopping_p);

int mmr_log(ttlog_level level, char *format_string, ...);
int mmr_vlog(ttlog_level level, char *format_string, va_list args);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _MODULE_MGR_H_ */
