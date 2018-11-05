/*****************************************************************************
 *
 * Original Author: Andrei Radulescu-Banu
 * Creation Date:   
 * Description: 
 * 
 * Copyright 2018 by Andrei Radulescu-Banu.  All Rights Reserved.
 * Unauthorized reproduction, modification, distribution, transmission, 
 * republication, display or performance are strictly prohibited.
 ****************************************************************************/

#ifndef _BITD_PLATFORM_SYS_H_
#define _BITD_PLATFORM_SYS_H_

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

/* Init/destroy the system */
bitd_boolean bitd_sys_init(void);
void bitd_sys_deinit();



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _BITD_PLATFORM_SYS_H_ */
