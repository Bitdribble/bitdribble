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
static char* g_input_file;
static bitd_boolean g_input_xml = TRUE, g_input_yaml;
static char* g_output_file = NULL;
static bitd_boolean g_output_json, g_output_string, g_output_xml, g_output_yaml;
static char* g_output_json_expected = NULL;
static char* g_output_string_expected = NULL;
static char* g_output_xml_expected = NULL;
static char* g_output_yaml_expected = NULL;
static bitd_boolean g_compressed_output = FALSE;
static bitd_boolean g_full_output = FALSE;
static int g_chunk_size = CHUNK_SIZE_DEF;
static bitd_boolean g_pack = FALSE;
static bitd_boolean g_chunk = FALSE;
static bitd_boolean g_unchunk = FALSE;
static bitd_boolean g_sort = FALSE;

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
    printf("This program tests conversion between objects in xml and yaml format.\n\n");

    printf("Options:\n"
           "    -i|--input input_file.{xml|yml|yaml}\n"
           "       Input file. Format is determined from file suffix.\n"
	   "       Default format is yaml.\n"
           "    -ix|--input-xml input_file\n"
           "       Input file, in xml format. Can use stdin.\n"
           "    -iy|--input-yaml input_file\n"
           "       Input file, in yaml format. Can use stdin.\n"
           "    -os|--output-string output_file\n"
           "       Output file, in string format. Can use stdout.\n"
           "    -ox|--output-xml output_file\n"
           "       Output file, in xml format. Can use stdout.\n"
           "    -oy|--output-yaml output_file\n"
           "       Output file, in yaml format. Can use stdout.\n"
           "    -oje|--output-json-expected output_file\n"
           "       Expected output in json format.\n"
           "    -ose|--output-string-expected output_file\n"
           "       Expected output in string format.\n"
           "    -oxe|--output-xml-expected output_file\n"
           "       Expected output in xml format.\n"
           "    -oye|--output-yaml-expected output_file\n"
           "       Expected output in yaml format.\n"
	   "    -fo|--full-output\n"
	   "       Full output file (not skipping default attributes).\n"
	   "    -co|--compressed-output\n"
	   "       Compressed output file (no newlines). Supported for json output only.\n"
	   "    -s|--chunk-size size\n"
	   "       Parse in this chunk size. Default: %d.\n"
	   "    -p|--pack\n"
	   "       Do a pack-unpack to test the packing mechanism.\n"
	   "    -chunk\n"
	   "       Chunk the object nvp, to test the chunking mechanism.\n"
	   "    -unchunk\n"
	   "       Unchunk the object nvp, to test the chunking mechanism.\n"
	   "    -sort\n"
	   "       Sort the object nvp.\n"
           "    -h, --help, -?\n"
           "       Show this help.\n",
	   CHUNK_SIZE_DEF);

    exit(0);
} 


/*
 *============================================================================
 *                        parse_xml
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  0 on success
 */
int parse_xml(FILE *f, bitd_object_t *a, char **object_name) {
    int ret = 0;
    bitd_xml_stream s_xml = NULL;
    bitd_boolean done = FALSE;
    char *buf;
    int idx;
    
    s_xml = bitd_xml_stream_init();
    
    /* Read the input */
    buf = malloc(g_chunk_size+1);
    idx = 0;
    
    done = FALSE;
    while (!done) {
	idx = fread(buf, 1, g_chunk_size, f);
	if (ferror(f)) {
	    fprintf(stderr, "%s: %s: Read error, errno %d (%s).\n",
		    g_prog_name, g_input_file, errno, strerror(errno));
	    ret = -1;
	    goto end;
	}
	
	buf[idx] = 0;
	done = feof(f);

	/* Parse the buffer */
	if (!bitd_xml_stream_read(s_xml, buf, idx, done)) {
	    fprintf(stderr, "%s: %s: %s.\n",
		    g_prog_name, g_input_file, 
		    bitd_xml_stream_get_error(s_xml));
	    ret = -1;
	    goto end;
	}
    }

    /* Get the object */
    bitd_xml_stream_get_object(s_xml, a, object_name);
    
 end:
    /* Close the nx stream */
    bitd_xml_stream_free(s_xml);

    /* Release memory */
    if (buf) {
	free(buf);
    }

    return ret;
} 


/*
 *============================================================================
 *                        parse_yaml
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  0 on success
 */
int parse_yaml(FILE *f, bitd_object_t *a, bitd_boolean *is_stream) {
    int ret = 0;
    char *buf;
    int idx, size = 10240;
    char *err_buf = NULL;
    int err_size = 1024;
    
    err_buf = malloc(err_size);

    /* Read the input */
    buf = malloc(size);
    idx = 0;
    
    /* Read the whole yaml file */
    do {
	if (idx + 1 >= size) {
	    size *= 2;
	    buf = realloc(buf, size);
	}

	idx += fread(buf + idx, 1, size - idx - 1, f);
	if (ferror(f)) {
	    fprintf(stderr, "%s: %s: Read error, errno %d (%s).\n",
		    g_prog_name, g_input_file, errno, strerror(errno));
	    ret = -1;
	    goto end;
	}
	
	/* Null terminate */
	buf[idx] = 0;
    } while (!feof(f));

    /* Parse the buffer */
    if (!bitd_yaml_to_object(a, buf, idx, is_stream, err_buf, err_size)) {
	fprintf(stderr, "%s: %s: %s.\n",
		g_prog_name, g_input_file, err_buf);
	ret = -1;
	goto end;
    }

 end:
    if (buf) {
	free(buf);
    }
    free(err_buf);
    return ret;
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
    bitd_object_t a;
    char *object_name = NULL;
    bitd_boolean is_stream = FALSE;
    bitd_nvp_t nvp1;
    char *buf, *buf1;
    int idx, size;
    FILE *f = NULL;

    bitd_object_init(&a);

    /* Parse program name argument */
    g_prog_name = bitd_get_leaf_filename(argv[0]);

    /* Skip to next parameter */
    argc--;
    argv++;

    /* Parse command line parameters */
    while (argc) {
        if (!strcmp(argv[0], "-i") ||
	    !strcmp(argv[0], "--input") ||
	    !strcmp(argv[0], "-ix") ||
	    !strcmp(argv[0], "--input-xml") ||
	    !strcmp(argv[0], "-iy") ||
	    !strcmp(argv[0], "--input-yaml")) {
	    char *opt = argv[0];

            /* Skip to next parameter */
            argc--;
            argv++;
            
            if (argc < 1) {
                usage();
            }

	    g_input_file = argv[0];
	    
	    if (!strcmp(opt, "-ix") ||
		!strcmp(opt, "--input-xml")) {
		g_input_xml = TRUE;
		g_input_yaml = FALSE;
	    } else if (!strcmp(opt, "-iy") ||
		       !strcmp(opt, "--input-yaml")) {
		g_input_xml = FALSE;
		g_input_yaml = TRUE;
	    } else {
		/* Determine the config based on suffix */
		char *suffix = bitd_get_filename_suffix(g_input_file);
		
		if (suffix && !strcmp(suffix, "xml")) {
		    g_input_xml = TRUE;
		    g_input_yaml = FALSE;
		} else {
		    g_input_xml = FALSE;
		    g_input_yaml = TRUE;
		}
	    }
        } else if (!strcmp(argv[0], "-oj") ||
		   !strcmp(argv[0], "--output-json")) {

            /* Skip to next parameter */
            argc--;
            argv++;
            
            if (argc < 1) {
                usage();
            }

	    g_output_file = argv[0];

	    g_output_json = TRUE;
	    g_output_string = FALSE;
	    g_output_xml = FALSE;
	    g_output_yaml = FALSE;

        } else if (!strcmp(argv[0], "-os") ||
		   !strcmp(argv[0], "--output-string")) {

            /* Skip to next parameter */
            argc--;
            argv++;
            
            if (argc < 1) {
                usage();
            }

	    g_output_file = argv[0];

	    g_output_json = FALSE;
	    g_output_string = TRUE;
	    g_output_xml = FALSE;
	    g_output_yaml = FALSE;

        } else if (!strcmp(argv[0], "-ox") ||
		   !strcmp(argv[0], "--output-xml")) {

            /* Skip to next parameter */
            argc--;
            argv++;
            
            if (argc < 1) {
                usage();
            }

	    g_output_file = argv[0];

	    g_output_json = FALSE;
	    g_output_string = FALSE;
	    g_output_xml = TRUE;
	    g_output_yaml = FALSE;

        } else if (!strcmp(argv[0], "-oy") ||
		   !strcmp(argv[0], "--output-yaml")) {

            /* Skip to next parameter */
            argc--;
            argv++;
            
            if (argc < 1) {
                usage();
            }

	    g_output_file = argv[0];

	    g_output_json = FALSE;
	    g_output_string = FALSE;
	    g_output_xml = FALSE;
	    g_output_yaml = TRUE;

        } else if (!strcmp(argv[0], "-oje") ||
		   !strcmp(argv[0], "--output-json-expected")) {

            /* Skip to next parameter */
            argc--;
            argv++;
            
            if (argc < 1) {
                usage();
            }

	    g_output_json_expected = argv[0];

        } else if (!strcmp(argv[0], "-ose") ||
		   !strcmp(argv[0], "--output-string-expected")) {

            /* Skip to next parameter */
            argc--;
            argv++;
            
            if (argc < 1) {
                usage();
            }

	    g_output_string_expected = argv[0];

        } else if (!strcmp(argv[0], "-oxe") ||
		   !strcmp(argv[0], "--output-xml-expected")) {

            /* Skip to next parameter */
            argc--;
            argv++;
            
            if (argc < 1) {
                usage();
            }

	    g_output_xml_expected = argv[0];

        } else if (!strcmp(argv[0], "-oye") ||
		   !strcmp(argv[0], "--output-yaml-expected")) {

            /* Skip to next parameter */
            argc--;
            argv++;
            
            if (argc < 1) {
                usage();
            }

	    g_output_yaml_expected = argv[0];

        } else if (!strcmp(argv[0], "-fo")||
		   !strcmp(argv[0], "--full-output")) {

	    g_full_output = TRUE;

        } else if (!strcmp(argv[0], "-co")||
		   !strcmp(argv[0], "--compressed-output")) {

	    g_compressed_output = TRUE;

        } else if (!strcmp(argv[0], "-s") ||
		   !strcmp(argv[0], "--chunk-size")) {

            /* Skip to next parameter */
            argc--;
            argv++;
            
            if (argc < 1) {
                usage();
            }

	    g_chunk_size = atoi(argv[0]);

        } else if (!strcmp(argv[0], "-p") ||
		   !strcmp(argv[0], "--pack")) {
	    g_pack = TRUE;
        } else if (!strcmp(argv[0], "-chunk")) {
	    g_chunk = TRUE;
        } else if (!strcmp(argv[0], "-unchunk")) {
	    g_unchunk = TRUE;
        } else if (!strcmp(argv[0], "-sort")) {
	    g_sort = TRUE;
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
    
    if (!g_input_file) {
	fprintf(stderr, "%s: No input file specified.\n",
		g_prog_name);
	ret = -1;
	goto end;
    }

    /* Open the input file */
    if (!strcmp(g_input_file, "stdin")) {
	f = stdin;
    } else {
	f = fopen(g_input_file, "r");
    }
    if (!f) {
	fprintf(stderr, "%s: Could not open %s: errno %d (%s).\n",
		g_prog_name, g_input_file, errno, strerror(errno));
	ret = -1;
	goto end;
    }

    if (g_input_xml) {
	ret = parse_xml(f, &a, &object_name);
    } else if (g_input_yaml) {
	ret = parse_yaml(f, &a, &is_stream);
    }
    
    if (ret) {
	goto end;
    }

    _bitd_assert_object(&a);

    /* Close the file descriptor */
    if (f != stdin) {
	fclose(f);
	f = NULL;
    }

    if (g_pack) {
	size = bitd_get_packed_size_object(&a);
	
	buf = malloc(size);
	idx = 0;

	if (!bitd_pack_object(buf, size, &idx, &a)) {
	    fprintf(stderr, 
		    "%s: Failed to pack the object, size = %d, idx = %d.\n",
		    g_prog_name, size, idx);
	    free(buf);
	    ret = -1;
	    goto end;
	}

	bitd_object_free(&a);
	bitd_object_init(&a);
	
	if (size != idx) {
	    fprintf(stderr, "%s: nvp packed into different size %d than allocated (%d).\n",
		    g_prog_name, idx, size);
	    free(buf);
	    ret = -1;
	    goto end;
	}

	idx = 0;
	if (!bitd_unpack_object(buf, size, &idx, &a)) {
	    fprintf(stderr, "%s: Failed to unpack the object, size = %d, idx = %d.\n",
		    g_prog_name, size, idx);
	    free(buf);
	    ret = -1;
	    goto end;
	}

	free(buf);
    }

    if (g_chunk) {
	if (a.type == bitd_type_nvp) {
	    nvp1 = bitd_nvp_chunk(a.v.value_nvp);
	    bitd_nvp_free(a.v.value_nvp);
	    a.v.value_nvp = nvp1;
	}
    }

    if (g_unchunk) {
	if (a.type == bitd_type_nvp) {
	    nvp1 = bitd_nvp_unchunk(a.v.value_nvp);
	    bitd_nvp_free(a.v.value_nvp);
	    a.v.value_nvp = nvp1;
	}
    }

    if (g_sort) {
	if (a.type == bitd_type_nvp) {
	    bitd_nvp_sort(a.v.value_nvp);
	}
    }

    if (g_output_file) {
	if (!strcmp(g_output_file, "stdout")) {
	    f = stdout;
	} else {
	    f = fopen(g_output_file, "w");
	}
	if (!f) {
	    fprintf(stderr, "%s: Could not open %s: errno %d (%s).\n",
		    g_prog_name, g_output_file, errno, strerror(errno));
	    ret = -1;
	    goto end;
	}

	if (g_output_json) {
	    buf = bitd_object_to_json(&a, g_full_output, g_compressed_output);
	    fprintf(f, "%s", buf);
	    free(buf);
	}
	
	if (g_output_string) {
	    buf = bitd_object_to_string(&a);
	    fprintf(f, "%s\n", buf);
	    free(buf);
	}
	
	if (g_output_xml) {
	    buf = bitd_object_to_xml(&a, object_name, g_full_output);
	    fprintf(f, "%s", buf);
	    free(buf);
	}

	if (g_output_yaml) {
	    buf = bitd_object_to_yaml(&a, g_full_output, is_stream);
	    if (buf) {
		fprintf(f, "%s", buf);
		free(buf);
	    }
	}

	if (f != stdout) {
	    fclose(f);
	    f = NULL;
	}
    }

    if (g_output_string_expected) {
	buf = bitd_object_to_string(&a);

	ret = bitd_read_text_file(&buf1, g_output_string_expected);
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

    if (ret) {
	goto end;
    }

    if (g_output_xml_expected) {
	buf = bitd_object_to_xml(&a, object_name, g_full_output);

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

    if (ret) {
	goto end;
    }

    if (g_output_yaml_expected) {
	buf = bitd_object_to_yaml(&a, g_full_output, is_stream);
	ret = bitd_read_text_file(&buf1, g_output_yaml_expected);
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

    if (ret) {
	goto end;
    }

 end:
    bitd_object_free(&a);
    if (object_name) {
	free(object_name);
    }
    if (f && (f != stdin) && (f != stdout)) {
	fclose(f);
    }

    return ret;
}
