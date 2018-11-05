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

#ifdef BITD_HAVE_DLFCN_H
# include <dlfcn.h>
#endif

/*****************************************************************************
 *                             MANIFEST CONSTANTS
 *****************************************************************************/



/*****************************************************************************
 *                                  MACROS 
 *****************************************************************************/



/*****************************************************************************
 *                                  TYPES
 *****************************************************************************/
struct bitd_dll_s {
#ifdef BITD_HAVE_DLFCN_H
    void *h; /* Linux dll handle */
#endif
#ifdef _WIN32
    HMODULE h; /* Windows dll handle */
#endif
};


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
 *                        bitd_dll_load
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
bitd_dll bitd_dll_load(char *name) {
    bitd_dll d;

    d = calloc(1, sizeof(*d));

#ifdef BITD_HAVE_DLFCN_H
    d->h = dlopen(name, RTLD_LOCAL|RTLD_NOW);
    if (!d->h) {
	free(d);
	return FALSE;
    }
#endif

#ifdef _WIN32
    d->h = LoadLibrary(name);
    if (!d->h) {
	free(d);
	return FALSE;
    }
#endif

    return d;
} 


/*
 *============================================================================
 *                        bitd_dll_unload
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
void bitd_dll_unload(bitd_dll d) {
    
    if (!d) {
	return;
    }

#ifdef BITD_HAVE_DLFCN_H
    /* Don't close the dll if valgrind is running, so we get stack traces
       inside dlls */
    if (!getenv("BITD_VALGRIND")) {
	if (d->h) {
	    dlclose(d->h);
	}
    }
#endif
    
#ifdef _WIN32
    if (d->h) {
	FreeLibrary(d->h);
    }
#endif
    
    free(d);
} 


/*
 *============================================================================
 *                        bitd_dll_sym
 *============================================================================
 * Description:     Look up a symbol
 * Parameters:    
 * Returns:  
 */
void *bitd_dll_sym(bitd_dll d, char *symbol) {
    if (!d) {
	return NULL;
    }
    
#ifdef BITD_HAVE_DLFCN_H
    return dlsym(d->h, symbol);
#endif
    
#ifdef _WIN32
    return GetProcAddress(d->h, symbol);    
#endif
} 
    

/*
 *============================================================================
 *                        bitd_dll_error
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
char *bitd_dll_error(char *buf, int buf_size) {
    if (!buf || !buf_size) {
	return NULL;
    }
    /* Null termination */
    buf[0] = 0;

#ifdef BITD_HAVE_DLFCN_H
    {
	char * err = dlerror();
	if (err) {
	    snprintf(buf, buf_size - 1, "%s", err);
	    /* Null termination */
	    buf[buf_size - 1] = 0;
	    
	    return buf;
	}
    }
#endif

#ifdef _WIN32
    {
	DWORD err = GetLastError();
	if (err) { 
	    snprintf(buf, buf_size - 1, "DLL load error %ld", 
		     (long)GetLastError());
	    /* Null termination */
	    buf[buf_size - 1] = 0;
	    
	    return buf;
	}
    }
#endif

    return NULL;

} 
