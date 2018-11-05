/*****************************************************************************
 *
 * Original Author: 
 * Creation Date:   
 * Description: 
 * 
 * Copyright (C) by Andrei Radulescu-Banu, 2018. All Rights Reserved.
 * Unauthorized reproduction, modification, distribution, transmission, 
 * republication, display or performance are strictly prohibited.
 ****************************************************************************/

#ifndef _BITD_TEST_UTIL_H_
#define _BITD_TEST_UTIL_H_

/*****************************************************************************
 *                                INCLUDE FILES 
 *****************************************************************************/
#include "bitd/types.h"



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

/* Read text file into *s. Return 0 on success. */
int bitd_read_text_file(char **s, char *file_name);

/* Perform a 'diff -w' operation between strings. Return 0 if they are
   the same module white spaces. */
int bitd_diff_w(char *s1, char *s2);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _BITD_TEST_UTIL_H_ */
