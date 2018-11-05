/*****************************************************************************
 *
 * Original Author: Andrei Radulescu-Banu
 * Creation Date:   
 * Description: 
 * 
 * Copyright (C) by Andrei Radulescu-Banu, 2018.  All Rights Reserved.
 * Unauthorized reproduction, modification, distribution, transmission, 
 * republication, display or performance are strictly prohibited.
 ****************************************************************************/

#ifndef _BITD_PLATFORM_DLL_H_
#define _BITD_PLATFORM_DLL_H_

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
typedef struct bitd_dll_s *bitd_dll;


/*****************************************************************************
 *                            FUNCTION DEFINITIONS
 *****************************************************************************/

/* Load and unload a dll */
bitd_dll bitd_dll_load(char *name);
void bitd_dll_unload(bitd_dll d);

/* Look up a dll symbol */
void *bitd_dll_sym(bitd_dll d, char *symbol);

/* Get the last dll error */
char *bitd_dll_error(char *buf, int buf_size);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _BITD_PLATFORM_DLL_H_ */
