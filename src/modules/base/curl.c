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
#include "bitd/log.h"
#include "bitd/module-api.h"

#include "curl/curl.h"

/*****************************************************************************
 *                             MANIFEST CONSTANTS
 *****************************************************************************/



/*****************************************************************************
 *                                  MACROS 
 *****************************************************************************/



/*****************************************************************************
 *                                  TYPES
 *****************************************************************************/
static ttlog_keyid s_log_keyid;


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

    s_log_keyid = ttlog_register("bitd-curl");

    ttlog(log_level_trace, s_log_keyid,
	  "%s:%d:%s() called", __FILE__, __LINE__, __FUNCTION__);

    /* Initialize libcurl */
    curl_global_cleanup();

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

    ttlog_unregister(s_log_keyid);

    /* Deinitialize libcurl */
    curl_global_cleanup();
} 


