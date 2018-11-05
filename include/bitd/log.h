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

#ifndef _BITD_LOGGER_H_
#define _BITD_LOGGER_H_

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
typedef enum {
    log_level_none = 0,
    log_level_crit = 1,
    log_level_err = 2,
    log_level_warn = 3,
    log_level_info = 4,
    log_level_debug = 5,
    log_level_trace = 6,
    log_level_max  /* sentinel */
} ttlog_level;


/*****************************************************************************
 *                                  MACROS 
 *****************************************************************************/



/*****************************************************************************
 *                                  TYPES
 *****************************************************************************/
typedef struct ttlog_keyid_s *ttlog_keyid;


/*****************************************************************************
 *                            FUNCTION DEFINITIONS
 *****************************************************************************/

/* Initialize and deinitialize the logger */
bitd_boolean ttlog_init(void);
void ttlog_deinit(bitd_boolean force_stop);

/* Configure the logger */
bitd_boolean ttlog_set_log_file_name(char *log_file_name);
bitd_boolean ttlog_set_log_file_size(int log_file_size);
bitd_boolean ttlog_set_log_file_count(int log_file_count);

/* Get the configured filter keys */
void ttlog_get_keys(char ***key_names, int *n_keys);

/* Free an array of filter keys */
void ttlog_free_keys(char **key_names, int n_keys);

/* Get and set the level. Filter key may be NULL. */
ttlog_level ttlog_get_level(char *key_name);
bitd_boolean ttlog_set_level(char *key_name, ttlog_level level);

/* Register and unregister a filter key */
ttlog_keyid ttlog_register(char *key_name);
void ttlog_unregister(ttlog_keyid keyid);
ttlog_keyid ttlog_get_keyid(char *key_name);

/* Log a message */
int ttlog(ttlog_level level, ttlog_keyid keyid, char *format_string, ...);
int ttvlog(ttlog_level level, ttlog_keyid keyid, char *format_string, va_list args);

/* Log a raw message */
int ttlog_raw(char *format_string, ...);
int ttvlog_raw(char *format_string, va_list args);

/* Log level names */
char *ttlog_get_level_name(ttlog_level level);
ttlog_level ttlog_get_level_from_name(char *level_name);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _BITD_LOGGER_H_ */
