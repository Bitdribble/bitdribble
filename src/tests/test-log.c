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

/*****************************************************************************
 *                                INCLUDE FILES 
 *****************************************************************************/
#include "bitd/common.h"
#include "bitd/file.h"
#include "bitd/log.h"


/*****************************************************************************
 *                             MANIFEST CONSTANTS
 *****************************************************************************/



/*****************************************************************************
 *                                  MACROS 
 *****************************************************************************/
#define LOG_FILE_SIZE_DEF 10240
#define LOG_FILE_COUNT_DEF 3

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
bitd_int32 g_verbose;
char *g_log_file_name = "stdout";
int g_log_file_size = LOG_FILE_SIZE_DEF;
int g_log_file_count = LOG_FILE_COUNT_DEF;

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
    printf("This program tests the log API.\n\n");

    printf("Options:\n"
	   "    -lf|--log-file file_name\n"
	   "       Set the log file name. Default: stdout\n"
	   "    -ls|--log-file-size size\n"
	   "       Set the log file size. Default: %d.\n"
	   "    -lc|--log-file-count size\n"
	   "       Set the log file count. Default: %d.\n"
	   "    -l|--log-level none|crit|error|warn|info|debug|trace\n"
	   "       Set global log level.\n"
	   "    -reg|-unreg key_name\n"
	   "       Register or unregister a key name.\n"
	   "    -lk|--log-key-level key_name none|crit|error|warn|info|debug|trace\n"
	   "       Set log key level.\n"
	   "    -reg-list\n"
	   "       List keys.\n"
	   "    -msg level key_name message\n"
	   "       Log a message under the given key.\n"
	   "    -msg-raw message\n"
	   "       Log a raw message.\n"
	   "    -s sleep_msec\n"
	   "       Insert a sleep.\n"
           "    -v verbose_level, --verbose verbose_level\n"
           "       Set the verbosity level (default: 2).\n"
           "    -h, --help, -?\n"
           "       Show this help.\n",
	   LOG_FILE_SIZE_DEF, LOG_FILE_COUNT_DEF);
} 


/*
 *============================================================================
 *                        main
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
int main(int argc, char ** argv) {
    int ret;
    ttlog_keyid keyid;
    ttlog_level level;

    bitd_sys_init();

    /* Parse program name argument */
    g_prog_name = bitd_get_leaf_filename(argv[0]);

    if (!ttlog_init()) {
	return -1;
    }

    /* Log to stdout by default */
    ttlog_set_log_file_name("stdout");

    /* Skip to next parameter */
    argc--;
    argv++;

    /* Parse parameters */
    while (argc) {
	if (!strcmp(argv[0], "-lf") ||
	    !strcmp(argv[0], "--log-file")) {

            /* Skip to next parameter */
            argc--;
            argv++;
            
            if (!argc) {
                usage();
		exit(-1);
            }

            if (!ttlog_set_log_file_name(argv[0])) {
		fprintf(stderr, 
			"%s: Failed to set log file name %s.\n", 
			g_prog_name, argv[0]);
		return -1;
	    }

	} else if (!strcmp(argv[0], "-ls") ||
		   !strcmp(argv[0], "--log-file-size")) {

            /* Skip to next parameter */
            argc--;
            argv++;
            
            if (!argc) {
                usage();
		exit(-1);
            }

            if (!ttlog_set_log_file_size(atoi(argv[0]))) {
		fprintf(stderr, 
			"%s: Failed to set log file size %s.\n", 
			g_prog_name, argv[0]);
		return -1;
	    }

	} else if (!strcmp(argv[0], "-lc") ||
		   !strcmp(argv[0], "--log-file-count")) {

            /* Skip to next parameter */
            argc--;
            argv++;
            
            if (!argc) {
                usage();
		exit(-1);
            }

            if (!ttlog_set_log_file_count(atoi(argv[0]))) {
		fprintf(stderr, 
			"%s: Failed to set log file count %s.\n", 
			g_prog_name, argv[0]);
		return -1;
	    }

	} else if (!strcmp(argv[0], "-l") ||
		   !strcmp(argv[0], "--log-level")) {

            /* Skip to next parameter */
            argc--;
            argv++;
            
            if (!argc) {
                usage();
		exit(-1);
            }

	    level = ttlog_get_level_from_name(argv[0]);
	    if (!level) {
		fprintf(stderr, 
			"%s: Invalid log level %s.\n", 
			g_prog_name, argv[0]);
	    }

            if (!ttlog_set_level(NULL, level)) {
		fprintf(stderr, 
			"%s: Failed to set log file level %s.\n", 
			g_prog_name, argv[0]);
		return -1;
	    }

	} else if (!strcmp(argv[0], "-reg")) {

            /* Skip to next parameter */
            argc--;
            argv++;
            
            if (!argc) {
                usage();
		exit(-1);
            }

	    keyid = ttlog_register(argv[0]);
	    if (!keyid) {
		fprintf(stderr, 
			"%s: Failed to register key '%s'.\n", 
			g_prog_name, argv[0]);
		return -1;
	    }

	} else if (!strcmp(argv[0], "-unreg")) {

            /* Skip to next parameter */
            argc--;
            argv++;
            
            if (!argc) {
                usage();
		exit(-1);
            }

	    keyid = ttlog_get_keyid(argv[0]);
	    if (!keyid) {
		fprintf(stderr, "%s: Key '%s' not registered.\n", 
			g_prog_name, argv[0]);
		return -1;
	    }

	    ttlog_unregister(keyid);

	} else if (!strcmp(argv[0], "-lk") ||
		   !strcmp(argv[0], "--log-key-level")) {

            /* Skip to next parameter */
            argc--;
            argv++;
            
            if (argc < 2) {
                usage();
		exit(-1);
            }

	    level = ttlog_get_level_from_name(argv[1]);
	    if (!level) {
		fprintf(stderr, 
			"%s: Invalid log level %s.\n", g_prog_name, argv[1]);
	    }

	    if (!ttlog_set_level(argv[0], level)) {
		fprintf(stderr, 
			"%s: Failed to set log level %s for key '%s'.\n", 
			g_prog_name, argv[1], argv[0]);
		return -1;
	    }

            /* Skip to next parameter */
            argc--;
            argv++;

	} else if (!strcmp(argv[0], "-reg-list")) {
	    char **key_names = NULL;
	    int n_keys = 0, i;

	    level = ttlog_get_level(NULL);
	    printf("Log level: %s\n", ttlog_get_level_name(level));

	    ttlog_get_keys(&key_names, &n_keys);

	    for (i = 0; i < n_keys; i++) {
		level = ttlog_get_level(key_names[i]);
		
		printf("  %s: %s\n", key_names[i], ttlog_get_level_name(level));
	    }

	    ttlog_free_keys(key_names, n_keys);

	} else if (!strcmp(argv[0], "-msg")) {

            /* Skip to next parameter */
            argc--;
            argv++;
            
            if (argc < 3) {
                usage();
		exit(-1);
            }

	    level = ttlog_get_level_from_name(argv[0]);
	    if (!level) {
		fprintf(stderr, 
			"%s: Invalid log level %s.\n", g_prog_name, argv[0]);
	    }

            ret = ttlog(level, ttlog_get_keyid(argv[1]), argv[2]);
	    if (ret < 0) {
		fprintf(stderr, 
			"%s: Level %s, key '%s': Failed to send message '%s' to log, error %d.\n", 
			g_prog_name, argv[0], argv[1], argv[2], ret);
		return -1;
	    }

            /* Skip to 2nd next parameter */
            argc -= 2;
            argv += 2;	    

	} else if (!strcmp(argv[0], "-msg-raw")) {

            /* Skip to next parameter */
            argc--;
            argv++;
            
            ret = ttlog_raw(argv[0]);
	    if (ret < 0) {
		fprintf(stderr, 
			"%s: Failed to send message '%s' to log, error %d.\n", 
			g_prog_name, argv[0], ret);
		return -1;
	    }

	} else if (!strcmp(argv[0], "-s")) {

            /* Skip to next parameter */
            argc--;
            argv++;
            
            if (!argc) {
                usage();
		exit(-1);
            }

	    if (g_verbose) {
		printf("%s: Sleeping %d msecs\n", 
		       g_prog_name, atoi(argv[0]));
	    }

            bitd_sleep(atoi(argv[0]));

	} else if (!strcmp(argv[0], "-v") ||
		   !strcmp(argv[0], "--verbose")) {

            /* Skip to next parameter */
            argc--;
            argv++;
            
            if (!argc) {
                usage();
		exit(-1);
            }

            /* Get the verbose level */
            g_verbose = atoi(argv[0]);
        } else if (!strcmp(argv[0], "-h") ||
                   !strcmp(argv[0], "--help") ||
                   !strcmp(argv[0], "-?")) {
            usage();
	    exit(0);
        } else {
            printf("%s: Skipping invalid parameter %s\n", g_prog_name, argv[0]);
        }

        /* Skip to next argument */
        argc--;
        argv++;        
    }    

    ttlog_deinit(TRUE);

    bitd_sys_deinit();

    return 0;
}
