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
#include "mmr.h"


/*****************************************************************************
 *                             MANIFEST CONSTANTS
 *****************************************************************************/



/*****************************************************************************
 *                                  MACROS 
 *****************************************************************************/



/*****************************************************************************
 *                                  TYPES
 *****************************************************************************/



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
 *                        mmr_schedule_task_inst
 *============================================================================
 * Description:     Set the run interval and the next start, based on
 *     the task instance schedule
 * Parameters:    
 * Returns:  
 */
void mmr_schedule_task_inst(struct mmr_task_inst_s *ti) {
    int idx;
    char *time_str;
    bitd_boolean sched_changed_p = FALSE;
    bitd_boolean timer_add_p = FALSE;
    bitd_boolean wake_up_event_loop_p = FALSE;
    bitd_uint64 current_time;
    bitd_int64 tmo = 0, tmo_min = 0, tmo_list = 0;

    /* If task instance is already scheduled, don't double schedule it.
       This is an issue for triggered tasks, which can be triggered
       while scheduled (or running). These tasks will be rescheduled
       when the run completes. */
    if (IS_SET(ti->state, TASK_INST_SCHEDULED)) {
	return;
    }

    /* If the task instance is stopping, don't reschedule it, and kick
       the task_inst_stopped event */
    if (ti->stopping_p) {
	bitd_event_set(g_mmr_cb->task_inst_stopped_ev);
	return;
    }

    if (IS_SET(ti->state, TASK_INST_SCHED_CHANGED)) {
	CLR_BIT(ti->state, TASK_INST_SCHED_CHANGED);
	sched_changed_p = TRUE;
    }

    /* Has the schedule changed? */
    if (sched_changed_p) {
	/* Parse the schedule type */
	if (bitd_nvp_lookup_elem(ti->sched, "type", &idx) &&
	    ti->sched->e[idx].type == bitd_type_string &&
	    ti->sched->e[idx].v.value_string) {
	    if (!strcmp(ti->sched->e[idx].v.value_string, "periodic")) {
		ti->sched_type = task_inst_sched_periodic_t;
	    } else if (!strcmp(ti->sched->e[idx].v.value_string,
			       "random")) {
		ti->sched_type = task_inst_sched_random_t;
	    } else if (!strcmp(ti->sched->e[idx].v.value_string,
			       "once")) {
		ti->sched_type = task_inst_sched_once_t;
	    } else if (!strcmp(ti->sched->e[idx].v.value_string,
			       "config")) {
		ti->sched_type = task_inst_sched_config_t;
	    } else if (!strcmp(ti->sched->e[idx].v.value_string,
			       "triggered")) {
		ti->sched_type = task_inst_sched_triggered_t;
	    } else if (!strcmp(ti->sched->e[idx].v.value_string,
			       "triggered-raw")) {
		ti->sched_type = task_inst_sched_triggered_raw_t;
	    } else {
		/* Not scheduled */
		ti->sched_type = task_inst_sched_none_t;
	    }
	}

	/* For periodic or random task instances, parse the run interval */
	if (ti->sched_type == task_inst_sched_periodic_t ||
	    ti->sched_type == task_inst_sched_random_t) {
	    
	    /* Do we have a run-interval? */
	    if (bitd_nvp_lookup_elem(ti->sched, "interval", &idx) &&
		ti->sched->e[idx].type == bitd_type_string) {
		
		/* Parse the run-interval*/
		time_str = ti->sched->e[idx].v.value_string;
		ti->run_interval_nsec = atoll(time_str);

		if (strstr(time_str, "ns")) {
		    /* No-op */
		} else if (strstr(time_str, "us")) {
		    ti->run_interval_nsec *= 1000ULL;
		} else if (strstr(time_str, "ms")) {
		    ti->run_interval_nsec *= 1000000ULL;
		} else if (strchr(time_str, 's')) {
		    ti->run_interval_nsec *= 1000000000ULL;
		} else if (strchr(time_str, 'm')) {
		    ti->run_interval_nsec *= (1000000000ULL*60);
		} else if (strchr(time_str, 'h')) {
		    ti->run_interval_nsec *= (1000000000ULL*3600);
		} else if (strchr(time_str, 'd')) {
		    ti->run_interval_nsec *= (1000000000ULL*3600*24);
		}
	    } else {
		/* Change schedule type to none */
		ti->sched_type = task_inst_sched_none_t;
	    }

	    /* Compute the next run time */
	    if (ti->run_interval_nsec) {
		/* Pick a random time less than run_interval but not
		   longer than 30 secs */
		bitd_uint64 next_run_nsec = MIN(ti->run_interval_nsec,
					      30000000000ULL);
		
		ti->next_run_nsec = bitd_random64() % next_run_nsec;
	    }
	}

	/* Add or remove to the triggered list */
	if (ti->sched_type == task_inst_sched_triggered_t ||
	    ti->sched_type == task_inst_sched_triggered_raw_t) {
	    if (!ti->triggered_next) {
		/* Insert in triggered list */
		ti->triggered_prev = g_mmr_cb->triggered_tail;
		ti->triggered_next = TRIGGERED_HEAD(g_mmr_cb);
		ti->triggered_prev->triggered_next = ti;
		ti->triggered_next->triggered_prev = ti;
	    }
	} else {
	    if (ti->triggered_next) {
		/* Remove from triggered list */
		ti->triggered_prev->triggered_next = ti->triggered_next;
		ti->triggered_next->triggered_prev = ti->triggered_prev;
		ti->triggered_next = NULL;
		ti->triggered_prev = NULL;
	    }
	}
    }

    switch (ti->sched_type) {
    case task_inst_sched_once_t:
	/* 'Once' task instances should run only once */
	if (!ti->run_id) {
	    /* Task inst has not run yet. 
	       Set the timer to expire immediately. */
	    timer_add_p = TRUE;
	    wake_up_event_loop_p = TRUE;
	} else {
	    /* Task inst has already run. Deinitialize the task instance */
	    mmr_task_inst_release(ti);
	    return;
	}
	break;
    case task_inst_sched_config_t:
	/* 'Config' task instances should re-run when schedule is changed */
	if (sched_changed_p) {
	    /* Task inst has not run yet. 
	       Set the timer to expire immediately. */
	    timer_add_p = TRUE;
	    wake_up_event_loop_p = TRUE;
	}
	break;

    case task_inst_sched_periodic_t:
    case task_inst_sched_random_t:
	/* If task instance not running, schedule its run timer */
	if (!IS_SET(ti->state, TASK_INST_RUNNING) &&
	    ti->run_interval_nsec) {
	    
	    current_time = bitd_get_time_nsec();
	    tmo = ti->next_run_nsec - current_time;
	    if (tmo <= 0) {
		/* We are falling behind - need to execute rightaway. 
		   Also reset the next_run based on the current time */
		tmo = 0;
		ti->next_run_nsec = current_time + 
		    ti->run_interval_nsec;
	    } else {
		if (ti->sched_type == task_inst_sched_random_t) {
		    /* Pick a random time less than tmo but 
		       more than the previous next_run */
		    tmo_min = ti->next_run_nsec - ti->run_interval_nsec - current_time;
		    tmo_min = MAX(tmo_min, 0);
		    tmo_min = MIN(tmo_min, tmo - 1);
		    tmo = tmo_min + (bitd_random64() % (tmo-tmo_min));
		}
		
		/* Push the next_run by the run_interval */
		ti->next_run_nsec += ti->run_interval_nsec;
	    }
	    
	    timer_add_p = TRUE;

	    /* We should wake up the event loop if the timer is shorter than
	       all other timers */
	    tmo_list = bitd_timer_list_get_timeout_msec(g_mmr_cb->timers);
	    if (tmo / 1000000ULL < tmo_list) {
		wake_up_event_loop_p = TRUE;
	    }
	}	
	break;

    case task_inst_sched_triggered_t:
    case task_inst_sched_triggered_raw_t:
	
	if (ti->input_queue_head != INPUT_QUEUE_HEAD(ti)) {
	    timer_add_p = TRUE;
	    wake_up_event_loop_p = TRUE;
	}
	break;

    default:
	break;
    }
    

    if (timer_add_p) {
	mmr_log(log_level_trace,
		"%s: %s: Timer add, tmo %u msecs, sched %d, run interval %llu msecs%s",
		ti->task->name,
		ti->name,
		tmo / 1000000ULL,
		ti->sched_type,
		ti->run_interval_nsec / 1000000ULL,
		wake_up_event_loop_p ? ", wake up event loop" : "");
	
	/* Input is queued up. Schedule the timer to expire immediately. */
	bitd_timer_list_add_nsec(g_mmr_cb->timers,
				 ti->run_timer,
				 tmo,
				 mmr_task_inst_run_timer_expired,
				 ti);

	SET_BIT(ti->state, TASK_INST_SCHEDULED);

	if (wake_up_event_loop_p) {
	    bitd_event_set(g_mmr_cb->event_loop_ev);
	}
    }
} 


/*
 *============================================================================
 *                        mmr_schedule_triggers
 *============================================================================
 * Description:     Pass results to trigger tests
 * Parameters:    
 * Returns:  
 *     TRUE if results are consumed, and should not be reported elsewhere
 *     FALSE if results are not consumed
 */
bitd_boolean mmr_schedule_triggers(mmr_task_inst_t ti_trigger,
				   mmr_task_inst_results_t *r) {
    bitd_boolean ret = FALSE;
    mmr_task_inst_t ti;
    int idx, i, j;
    struct input_queue_s *iq;

    bitd_mutex_lock(g_mmr_cb->lock);

    /* Should we exit on non-zero exit code? */
    if (r->exit_code &&
	bitd_nvp_lookup_elem(ti_trigger->sched, "exit-on-error", &idx) &&
	ti_trigger->sched->e[idx].type == bitd_type_boolean &&
	ti_trigger->sched->e[idx].v.value_boolean) {

	mmr_log(log_level_err, "%s: %s: Non-zero exit code %d", 
		ti_trigger->task->name, ti_trigger->name, r->exit_code);

	/* Exit the application after leaving a bit of time for log messages
	   to flush */
	bitd_sleep(250);
	exit(r->exit_code);	
    }

    /* TO DO: optimize with a hash mechanism */
    for (ti = g_mmr_cb->triggered_head;
	 ti != TRIGGERED_HEAD(g_mmr_cb);
	 ti = ti->triggered_next) {

	/* Is this trigger a match? TO DO: this is inefficient. */
	if (bitd_nvp_lookup_elem(ti->sched, "task-name", &idx) &&
	    ti->sched->e[idx].type == bitd_type_string &&
	    ti->sched->e[idx].v.value_string) {
	    /* Task name should match */
	    if (strcmp(ti->sched->e[idx].v.value_string,
		       ti_trigger->task->name)) {
		continue;
	    }
	}
	if (bitd_nvp_lookup_elem(ti->sched, "task-inst-name", &idx) &&
	    ti->sched->e[idx].type == bitd_type_string &&
	    ti->sched->e[idx].v.value_string) {
	    /* Task inst name should match */
	    if (strcmp(ti->sched->e[idx].v.value_string,
		       ti_trigger->name)) {
		continue;
	    }
	}
	if (bitd_nvp_lookup_elem(ti->sched, "tags", &idx) &&
	    ti->sched->e[idx].type == bitd_type_nvp &&
	    ti->sched->e[idx].v.value_nvp) {
	    bitd_boolean match = TRUE;
	    bitd_nvp_t tags1 = ti->sched->e[idx].v.value_nvp;
	    bitd_nvp_t tags2 = ti_trigger->params.tags;

	    /* Each tag should match */
	    for (i = 0; i < tags1->n_elts; i++) {
		char *name = tags1->e[i].name;
		bitd_type_t type = tags1->e[i].type;
		bitd_value_t *v = &tags1->e[i].v;

		if (!bitd_nvp_lookup_elem(tags2, name, &j) ||
		    tags2->e[j].type != type ||
		    bitd_value_compare(&tags2->e[j].v, v, type)) {
		    match = FALSE;
		    break;
		}
	    }
	    if (!match) {
		continue;
	    }
	}

	if (ti->sched_type == task_inst_sched_triggered_t) {
	    /* Only trigger when the exit code was zero, signifying that
	       the previous task instance had no error */
	    if (r->exit_code != 0) {
		continue;
	    }
	}

	if (g_mmr_cb->input_queue_max <= g_mmr_cb->input_queue_size) {
	    /* Queue full */
	    g_mmr_cb->input_queue_dropped++;

	    mmr_log(log_level_warn, "%s: %s: Dropping trigger for %s: %s, result queue size full (%d/%d)",
		    ti_trigger->task->name,
		    ti_trigger->name,
		    ti->task->name,
		    ti->name,
		    g_mmr_cb->input_queue_size, g_mmr_cb->input_queue_max);
	    continue;
	}

	/* Enqueue input for the triggered task instance */
	iq = malloc(sizeof(*iq));
	iq->next = ti->input_queue_tail;
	iq->prev = ti->input_queue_tail->prev;
	iq->next->prev = iq;
	iq->prev->next = iq;
	iq->input.type = bitd_type_void;

	g_mmr_cb->input_queue_size++;
	if (g_mmr_cb->input_queue_size > g_mmr_cb->input_queue_max / 2) {
	    mmr_log(log_level_warn, "Input queue size incremented to %d/%d, above half",
		    g_mmr_cb->input_queue_size, g_mmr_cb->input_queue_max);
	}

	if (ti->sched_type == task_inst_sched_triggered_t) {
	    /* Copy the previous task instance output as the input
	       of this task instance */
	    bitd_object_clone(&iq->input, &r->output);

	    mmr_log(log_level_trace, "%s: %s: Triggering %s: %s",
		    ti_trigger->task->name,
		    ti_trigger->name,
		    ti->task->name,
		    ti->name);
	} else if (ti->sched_type == task_inst_sched_triggered_raw_t) {
	    /* Copy the previous task instance tags, run-id, run-timestamp.
	       exit-code, output and error */
	    iq->input.type = bitd_type_nvp;
	    iq->input.v.value_nvp = mmr_get_raw_results(ti_trigger, r);

	    mmr_log(log_level_trace, "%s: %s: Triggering-raw %s: %s ",
		    ti_trigger->task->name,
		    ti_trigger->name,
		    ti->task->name,
		    ti->name);
	}

	/* Schedule the task. The schedule routine will check internally
	   to make sure this triggered task is not double scheduled. */
	mmr_schedule_task_inst(ti);
    }

    bitd_mutex_unlock(g_mmr_cb->lock);

    return ret;
}
