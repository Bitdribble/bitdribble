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

#ifndef _BITD_FILE_H_
#define _BITD_FILE_H_

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



/*****************************************************************************
 *                                  MACROS 
 *****************************************************************************/



/*****************************************************************************
 *                                  TYPES
 *****************************************************************************/



/*****************************************************************************
 *                            FUNCTION DEFINITIONS
 *****************************************************************************/

/* Returns statically allocated string */
char *bitd_get_leaf_filename(char *path);

/* Returns substring of the path */
char *bitd_get_filename_suffix(char *path);

/* Returns heap allocated string */
char *bitd_get_path_dir(char *path);

/* Concatenate path2 at end of path1 */
char *bitd_path_add(char *path1, char *path2, char sep);
char *bitd_filepath_add(char *path1, char *path2);
char *bitd_envpath_add(char *path1, char *path2);

/* Look up fname in path with given permissions mode */
char *bitd_envpath_find(char *path, char *fname, bitd_uint32 mode);

/* Read text file into *s. Return 0 on success. */
int bitd_read_text_file(char **s, char *file_name);

/* Perform a 'diff -w' operation between strings. Return 0 if they are
   the same module white spaces. */
int bitd_diff_w(char *s1, char *s2);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _BITD_FILE_H_ */
