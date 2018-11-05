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

#ifndef _BITD_PLATFORM_FILE_H_
#define _BITD_PLATFORM_FILE_H_

/*****************************************************************************
 *                                INCLUDE FILES 
 *****************************************************************************/
#include "bitd/platform-types.h"


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



/*****************************************************************************
 *                                  TYPES
 *****************************************************************************/



/*****************************************************************************
 *                            FUNCTION DEFINITIONS
 *****************************************************************************/

/* Return array of file path separator characters for the platform. 
   First character is the default separator. */
char *bitd_get_filepath_sep(void);

/* The environment path separator character for the platform */
char bitd_get_envpath_sep(void);

/* The name of the LD_LIBRARY_PATH environment variable for the platform */
char *bitd_get_dll_path_name(void);

/* Returns heap-allocated full dll name */
char *bitd_get_dll_name(char *name_root);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _BITD_PLATFORM_FILE_H_ */
