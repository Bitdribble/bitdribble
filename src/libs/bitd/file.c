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
#include "bitd/file.h"

#include <ctype.h>

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
 *                        is_filepath_sep
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
static bitd_boolean is_filepath_sep(char c) {
    char *sep;

    sep = bitd_get_filepath_sep();
    
    while (*sep) {
	if (*sep++ == c) {
	    return TRUE;
	}
    }

    return FALSE;
} 


/*
 *============================================================================
 *                        bitd_get_leaf_filename
 *============================================================================
 * Description:     Get the leaf file name, according to the platform
 *           path character separator.
 * Parameters:    
 * Returns:  Substring of the passed-in path.
 */
char *bitd_get_leaf_filename(char *path) {
    int len;
    char *c;
    
    if (!path) {
	return NULL;
    }

    len = strlen(path);
    if (!len) {
	return "";
    }

    c = &path[len-1];
    
    for (;;) {
	if (is_filepath_sep(*c)) {
	    return c+1;
	}
	--c;
	if (c == path) {
	    return path;
	}
    }

    /* Unreached */
    return NULL;
} 


/*
 *============================================================================
 *                        bitd_get_filename_suffix
 *============================================================================
 * Description:     Get the file name suffix
 * Parameters:    
 * Returns:  Substring of the passed-in path.
 */
char *bitd_get_filename_suffix(char *path) {
    register char *c;
    
    if (!path) {
	return NULL;
    }
    
    /* Go to the end of the path */
    for (c = path; *c; c++);

    /* Walk back until 1st dot, or 1st path separator */
    while (c != path) {
	if (is_filepath_sep(*c)) {
	    return NULL;
	}
	if (*c == '.') {
	    return c+1;
	}
	--c;
    }

    return NULL;
} 


/*
 *============================================================================
 *                        bitd_get_path_dir
 *============================================================================
 * Description:     Get the directory name of a path
 * Parameters:    
 * Returns:  Heap-allocated array
 */
char *bitd_get_path_dir(char *path) {
    int len;
    char *c;
    
    if (!path) {
	return NULL;
    }

    path = strdup(path);

    len = strlen(path);
    if (!len) {
	return "";
    }

    c = &path[len-1];
    
    while (c != path) {
	if (is_filepath_sep(*c)) {
	    *c = 0;
	    return path;
	}
	--c;
    }

    /* Path is a filename in the current directory. Return the 
       current directory. */

    free(path);
    return strdup(".");
} 


/*
 *============================================================================
 *                        bitd_path_add
 *============================================================================
 * Description:     Append path2 to path1. Assumes path1 is heap allocated.
 * Parameters:    
 * Returns:  
 *    Heap-reallocated concatenated path1
 */
char *bitd_path_add(char *path1, char *path2, char sep) {
    int len1, len2;

    if (!path2 || !path2[0]) {
	return path1;
    }

    if (!path1 || !path1[0]) {
	return strdup(path2);
    }

    len1 = strlen(path1);
    len2 = strlen(path2);

    path1 = realloc(path1, len1 + len2 + 2);

    path1[len1] = sep;
    memcpy(path1 + len1 + 1, path2, len2);
    path1[len1 + len2 + 1] = 0;
    
    return path1;
} 


/*
 *============================================================================
 *                        bitd_filepath_add
 *============================================================================
 * Description:     Append path2 to path1. Assumes path1 is heap allocated.
 * Parameters:    
 * Returns:  
 *    Heap-reallocated concatenated path1
 */
char *bitd_filepath_add(char *path1, char *path2) {
    return bitd_path_add(path1, path2, bitd_get_filepath_sep()[0]);
}


/*
 *============================================================================
 *                        bitd_envpath_add
 *============================================================================
 * Description:     Append path2 to path1. Assumes path1 is heap allocated.
 * Parameters:    
 * Returns:  
 *    Heap-reallocated concatenated path1
 */
char *bitd_envpath_add(char *path1, char *path2) {
    return bitd_path_add(path1, path2, bitd_get_envpath_sep());
}


/*
 *============================================================================
 *                        bitd_envpath_find
 *============================================================================
 * Description:     Get the full name of the first instance of the file
 *     in the path. Result is heap allocated.
 * Parameters:    
 *     path - the search path
 *     fname - the file name we're looking for
 *     mode - File is a match if all the 'mode' bits are enabled.
 * Returns:  
 *    Heap-allocated file name.
 */
char *bitd_envpath_find(char *path, char *fname, bitd_uint32 mode) {
    char sep;
    char *c, *c_next, *fname1;
    struct stat s;

    sep = bitd_get_envpath_sep();

    if (!path || !path[0] || !fname || !fname[0]) {
	return NULL;
    }

    path = strdup(path);
    c = path;
    while (c && *c) {
	c_next = strchr(c, sep);
	if (c_next) {
	    *c_next++ = 0;
	}
	
	/* Try this file name */
	fname1 = bitd_filepath_add(strdup(c), fname);
	
	/* Does it exist with the right permissions? */
	if (!stat(fname1, &s)) {
	    if ((mode & s.st_mode) == mode) {
		free(path);
		return fname1;
	    }
	}

	/* Release the file name */
	free(fname1);

	c = c_next;
    }

    free(path);

    return NULL;
} 


/*
 *============================================================================
 *                        bitd_read_text_file
 *============================================================================
 * Description:   Read text file into *s. Return 0 on success.  
 * Parameters:    
 *     s [OUT] - Heap allocated contents of the file
 *     file_name - Input file name. File is assumed to contain text 
 *           and not binary characters.
 * Returns:  
 *     0 on success, with the contents of the file in *s
 */
int bitd_read_text_file(char **s, char *file_name) {
    int ret = 0;
    int size = 0;
    int idx = 0;
    FILE *f = NULL;

    if (!s) {
	return -1;
    }

    *s = NULL;

    f = fopen(file_name, "r");
    if (!f) {
	fprintf(stderr, "Could not open %s: errno %d (%s).\n",
		file_name, 
		errno, strerror(errno));
	ret = -1;
	goto end;
    }

    for (;;) {
	size += 10240;
	*s = realloc(*s, size);

	idx += fread(*s + idx, 1, size - idx, f);
	if (idx < size) {
	    if (ferror(f)) {
		fprintf(stderr, "%s: Read error, errno %d (%s).\n",
			file_name, 
			errno, strerror(errno));
		ret = -1;
		goto end;
	    }
	    if (feof(f)) {
		/* End of file. Add NULL termination. */
		(*s)[idx] = 0;
		break;
	    }
	}
    }

 end:
    if (f) {
	fclose(f);
    }

    return ret;
} 


/*
 *============================================================================
 *                        bitd_diff_w
 *============================================================================
 * Description:     Perform a 'diff -w' operation between strings. Return 0 
 *     if they are the same module white spaces.
 * Parameters:    
 * Returns:  
 */
int bitd_diff_w(char *s1, char *s2) {
    register char *c1, *c2;

    if (!s1 || !s2) {
	return FALSE;
    }

    c1 = s1;
    c2 = s2;

    for (;;) {
	while (isspace(*c1)) {
	    c1++;
	}
	while (isspace(*c2)) {
	    c2++;
	}

	if (*c1 < *c2) {
	    return -1;
	}
	
	if (*c1 > *c2) {
	    return 1;
	}
	
	if (!*c1 && !*c2) {
	    return 0;
	}

	/* Get next characters */
	c1++;
	c2++;
    }

    /* Unreached */
    return 0;
} 

