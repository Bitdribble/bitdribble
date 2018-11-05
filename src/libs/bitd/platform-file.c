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
#include "bitd/platform-file.h"


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
 *                        bitd_get_filepath_sep
 *============================================================================
 * Description:     Returns array of file path separator characters for the 
 *    platform. First character is the default separator. 
 * Parameters:    
 * Returns:  
 */
char *bitd_get_filepath_sep(void) {

#if defined(_WIN32)
    return "\\/";
#endif

    /* All other platforms including Cygwin use a Posix filepath separator */
    return "/";
} 


/*
 *============================================================================
 *                        bitd_get_envpath_sep
 *============================================================================
 * Description:     Returns the environment path separator for the platform.
 * Parameters:    
 * Returns:  
 */
char bitd_get_envpath_sep(void) {

#if defined(_WIN32)
    return ';';
#endif
    
    /* All other platforms including Cygwin use a Posix envpath separator */
    return ':';
} 


/*
 *============================================================================
 *                        bitd_get_ld_library_path_name
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
char *bitd_get_dll_path_name() {
#if defined(_WIN32) || defined(__CYGWIN__)
    return "PATH";
#elif defined(__APPLE__) && defined(__MACH__)
    return "DYLD_FALLBACK_LIBRARY_PATH";
#elif defined(__linux__)
    return "LD_LIBRARY_PATH";
#endif
} 

/*
 *============================================================================
 *                        bitd_get_dll_name
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
char *bitd_get_dll_name(char *name_root) {
    int len;
    bitd_boolean already_dll = FALSE;
    char *c;

    if (!name_root) {
	return NULL;
    }

    /* If there's dots in the name, it's already a dll name */
    if (!already_dll) {
	if (strchr(name_root, '.')) {
	    already_dll = TRUE;
	}
    }

    if (!already_dll) {
	for (c = bitd_get_filepath_sep(); *c; c++) {
	    if (strchr(name_root, *c)) {
		already_dll = TRUE;
		break;
	    }
	}
    }

    if (already_dll) {
	/* The name root is already a dll name */
	return strdup(name_root);
    }

    len = strlen(name_root) + 24;
    c = malloc(len);

#if defined(__linux__)
    sprintf(c, "lib%s.so", name_root);
#endif

#if defined(__APPLE__) && defined(__MACH__)
    sprintf(c, "lib%s.dylib", name_root);
#endif

#if defined(_WIN32)
    sprintf(c, "%s.dll", name_root);
#endif

#if defined(__CYGWIN__)
    sprintf(c, "cyg%s.dll", name_root);
#endif

    return c;
} 
