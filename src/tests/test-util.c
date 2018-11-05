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

/*****************************************************************************
 *                                INCLUDE FILES 
 *****************************************************************************/
#include "test-util.h"
#include "bitd/file.h"

#include <errno.h>
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
