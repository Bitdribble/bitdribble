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
#include "bitd/types.h"
#include "bitd/base64.h"
#include "bitd/file.h"

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
static char* g_prog_name = "";

/*****************************************************************************
 *                          FUNCTION IMPLEMENTATION
 *****************************************************************************/


/*
 *============================================================================
 *                        usage
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
void usage() {

    printf("\nUsage: %s [-e|-u]\n\n", g_prog_name);
    printf("This program encodes and decodes the stdin into base64\n"
	   "and prints the output on stdout.\n\n");

    printf("Options:\n"
           "    -e\n"
           "       Encode to stdout\n"
           "    -u\n"
           "       Decode to stdout\n"
           "    -h, --help, -?\n"
           "       Show this help.\n");

    exit(0);
} 





/*
 *============================================================================
 *                        main
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
int main(int argc, char **argv) {
    bitd_boolean encode_p = TRUE;
    int buflen = 900, buflen1,len;
    char *buf = NULL, *buf1 = NULL;
    int idx, done;

    /* Parse program name argument */
    g_prog_name = bitd_get_leaf_filename(argv[0]);

    /* Skip to next parameter */
    argc--;
    argv++;

    /* Parse command line parameters */
    while (argc) {
        if (!strcmp(argv[0], "-e")) {
	    encode_p = TRUE;
        } else if (!strcmp(argv[0], "-d")) {
	    encode_p = FALSE;
        } else if (!strcmp(argv[0], "-h") ||
                   !strcmp(argv[0], "--help") ||
                   !strcmp(argv[0], "-?")) {
            usage();
        } else {
            printf("%s: Skipping invalid parameter %s\n", 
		   g_prog_name, argv[0]);
        }

        /* Skip to next argument */
        argc--;
        argv++;        
    }

    /* Read the input */
    buf = malloc(buflen+1);

    done = FALSE;
    do {
	idx = fread(buf, 1, buflen, stdin);
	buf[idx] = 0;
	done = feof(stdin);

	if (encode_p) {
	    buflen1 = bitd_base64_encode_len(idx);
	    buf1 = malloc(buflen1);
	    len = bitd_base64_encode(buf1, buf, idx);
	    fprintf(stdout, "%s\n", buf1);
	} else {
	    buflen1 = bitd_base64_decode_len(buf);
	    buf1 = malloc(buflen1);
	    len = bitd_base64_decode(buf1, buf);
	    fwrite(buf1, 1, len, stdout);
	}

	free(buf1);

    } while (!done);

    free(buf);

    return 0;
}
