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
#include "bitd/pack.h"
#include "bitd/file.h"
#include "test-util.h"

#include <errno.h>
#include <ctype.h>

/*****************************************************************************
 *                             MANIFEST CONSTANTS
 *****************************************************************************/



/*****************************************************************************
 *                                  MACROS 
 *****************************************************************************/

#define CHUNK_SIZE_DEF 100

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
static char* g_input_file = NULL;
static char* g_input_file_new = NULL;
static char* g_input_file_base = NULL;
static char* g_output_file_xml = NULL;
static char* g_output_file_string = NULL;
static char* g_output_xml_expected = NULL;
static bitd_boolean g_full_output = FALSE;
static int g_chunk_size = CHUNK_SIZE_DEF;

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

    printf("\nUsage: %s [OPTIONS ... ]\n\n", g_prog_name);
    printf("This program tests nvp => xml conversion.\n\n");

    printf("Options:\n"
           "    -ix|--input-xml input_file\n"
           "       File containing the nvp in xml format.\n"
           "    -ixn|--input-new-xml input_file\n"
           "       File containing the new nvp in xml format.\n"
           "    -ixb|--input-base-xml input_file\n"
           "       File containing the base nvp in xml format.\n"
           "    -ox|--output-xml output_file\n"
           "       Output file for the nvp in xml format. Can use stdout. \n"
	   "    -f|--full-output\n"
	   "       Full output file (not skipping default attributes).\n"
           "    -os|--output-tring output_file\n"
           "       Output file for the nvp in string format. Can use stdout. \n"
           "    -oxe|--output-xml-expected output_file\n"
           "       Expected output in xml format.\n"
           "    -h, --help, -?\n"
           "       Show this help.\n");

    exit(0);
} 


/*
 *============================================================================
 *                        parse_file
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
bitd_nvp_t parse_file(char *file_name) {
    bitd_nvp_t nvp;
    char *buf;
    int idx;
    FILE *f = NULL;
    bitd_xml_stream s = NULL;
    bitd_boolean done;

    f = fopen(file_name, "r");
    if (!f) {
	fprintf(stderr, "%s: Could not open %s: errno %d (%s).\n",
		g_prog_name, file_name, errno, strerror(errno));
	exit(-1);
    }

    /* Initialize the nx stream */
    s = bitd_xml_stream_init();

    /* Read the input */
    buf = malloc(g_chunk_size+1);
    idx = 0;

    done = FALSE;
    while (!done) {
	idx = fread(buf, 1, g_chunk_size, f);
	if (ferror(f)) {
	    fprintf(stderr, "%s: %s: Read error, errno %d (%s).\n",
		    g_prog_name, file_name, errno, strerror(errno));
	    fclose(f);
	    free(buf);
	    bitd_xml_stream_free(s);
	    exit(-1);
	}

	buf[idx] = 0;
	done = feof(f);

	/* Parse the buffer */
	if (!bitd_xml_stream_read(s, buf, idx, done)) {
	    fprintf(stderr, "%s: %s: %s.\n",
		    g_prog_name, file_name, bitd_xml_stream_get_error(s));
	    fclose(f);
	    free(buf);
	    bitd_xml_stream_free(s);
	    exit(-1);	    
	}
    }

    /* Get the nvp */
    nvp = bitd_xml_stream_get_nvp(s);

    /* Can close the nx stream now */
    bitd_xml_stream_free(s);

    /* ... and the file descriptor */
    fclose(f);

    free(buf);

    return nvp;
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
    int ret = 0;
    bitd_nvp_t nvp = NULL;
    bitd_nvp_t new_nvp = NULL;
    bitd_nvp_t base_nvp = NULL;
    bitd_nvp_t merged_nvp = NULL;
    char *buf, *buf1;
    FILE *f = NULL;

    /* Parse program name argument */
    g_prog_name = bitd_get_leaf_filename(argv[0]);

    /* Skip to next parameter */
    argc--;
    argv++;

    /* Parse command line parameters */
    while (argc) {
        if (!strcmp(argv[0], "-ix") ||
	    !strcmp(argv[0], "--input-xml")) {

            /* Skip to next parameter */
            argc--;
            argv++;
            
            if (argc < 1) {
                usage();
            }

	    g_input_file = argv[0];

        } else if (!strcmp(argv[0], "-ixn") ||
		   !strcmp(argv[0], "--input-new-xml")) {

            /* Skip to next parameter */
            argc--;
            argv++;
            
            if (argc < 1) {
                usage();
            }

	    g_input_file_new = argv[0];

        } else if (!strcmp(argv[0], "-ixb") ||
		   !strcmp(argv[0], "--input-base-xml")) {

            /* Skip to next parameter */
            argc--;
            argv++;
            
            if (argc < 1) {
                usage();
            }

	    g_input_file_base = argv[0];

        } else if (!strcmp(argv[0], "-ox") ||
		   !strcmp(argv[0], "--output-xml")) {

            /* Skip to next parameter */
            argc--;
            argv++;
            
            if (argc < 1) {
                usage();
            }

	    g_output_file_xml = argv[0];

        } else if (!strcmp(argv[0], "-f") ||
		   !strcmp(argv[0], "--full-output")) {

	    g_full_output = TRUE;

        } else if (!strcmp(argv[0], "-os") ||
		   !strcmp(argv[0], "--output-string")) {

            /* Skip to next parameter */
            argc--;
            argv++;
            
            if (argc < 1) {
                usage();
            }

	    g_output_file_string = argv[0];

        } else if (!strcmp(argv[0], "-oxe") ||
		   !strcmp(argv[0], "--output-xml-expected")) {

            /* Skip to next parameter */
            argc--;
            argv++;
            
            if (argc < 1) {
                usage();
            }

	    g_output_xml_expected = argv[0];

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

    /* Open the input file */
    if (!g_input_file) {
	fprintf(stderr, "%s: No input file specified.\n",
		g_prog_name);
	exit(-1);
    }

    /* Parse the nvp */
    nvp = parse_file(g_input_file);

    if (g_input_file_new) {
	/* Parse the new nvp */
	new_nvp = parse_file(g_input_file_new);
    }

    if (g_input_file_base) {
	/* Parse the base nvp */
	base_nvp = parse_file(g_input_file_base);
    }

    /* Perform the merge */
    merged_nvp = bitd_nvp_merge(nvp, new_nvp, base_nvp);

    if (g_output_file_xml) {
	if (!strcmp(g_output_file_xml, "stdout")) {
	    g_output_file_xml = "/dev/stdout";
	}

	f = fopen(g_output_file_xml, "w");
	if (!f) {
	    fprintf(stderr, "%s: Could not open %s: errno %d (%s).\n",
		    g_prog_name, g_output_file_xml, errno, strerror(errno));
	    bitd_nvp_free(nvp);
	    bitd_nvp_free(new_nvp);
	    bitd_nvp_free(base_nvp);
	    bitd_nvp_free(merged_nvp);
	    exit(-1);
	}

	buf = bitd_nvp_to_xml(merged_nvp, "nvp", g_full_output);
	fprintf(f, "%s", buf);
	free(buf);

	fclose(f);
    }

    if (g_output_file_string) {
	if (!strcmp(g_output_file_string, "stdout")) {
	    g_output_file_string = "/dev/stdout";
	}

	f = fopen(g_output_file_string, "w");
	if (!f) {
	    fprintf(stderr, "%s: Could not open %s: errno %d (%s).\n",
		    g_prog_name, g_output_file_xml, errno, strerror(errno));
	    bitd_nvp_free(nvp);
	    bitd_nvp_free(new_nvp);
	    bitd_nvp_free(base_nvp);
	    bitd_nvp_free(merged_nvp);
	    exit(-1);
	}

	buf = bitd_nvp_to_string(merged_nvp, NULL);
	fprintf(f, "%s\n", buf);
	free(buf);

	fclose(f);
    }

    if (g_output_xml_expected) {
	buf = bitd_nvp_to_xml(merged_nvp, "nvp", g_full_output);

	ret = bitd_read_text_file(&buf1, g_output_xml_expected);
	if (!ret) {
	    ret = bitd_diff_w(buf, buf1);
	}

	if (buf) {
	    free(buf);
	}
	if (buf1) {
	    free(buf1);
	}
    }

    bitd_nvp_free(nvp);
    bitd_nvp_free(new_nvp);
    bitd_nvp_free(base_nvp);
    bitd_nvp_free(merged_nvp);

    return ret;
}
