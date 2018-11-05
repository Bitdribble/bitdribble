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


/*****************************************************************************
 *                             MANIFEST CONSTANTS
 *****************************************************************************/



/*****************************************************************************
 *                                  MACROS 
 *****************************************************************************/
#define VERBOSE_DEF 1


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
static int g_verbose = VERBOSE_DEF;


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
static void usage(void) {
    printf("\nUsage: %s [OPTIONS ... ]\n\n", g_prog_name);
    printf("This program tests the gethostbyname API.\n\n");

    printf("Options:\n"
           "    hostport\n"
           "        Look up this host and port in host or host:port or [host]:port format.\n"
           "    -v verbose_level, --verbose verbose_level\n"
           "        Set the verbosity level (default: %d).\n"
           "    -h, --help, -?\n"
           "        Show this help.\n"
           , VERBOSE_DEF);
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
    int ret = 0;

    bitd_sys_init();

    /* Parse program name argument */
    g_prog_name = bitd_get_leaf_filename(argv[0]);

    /* Skip to next parameter */
    argc--;
    argv++;

    /* Parse vector size and help */
    while (argc) {
        if (!strcmp(argv[0], "-h") ||
            !strcmp(argv[0], "--help") ||
            !strcmp(argv[0], "-?")) {
            usage();
            goto end;
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
        } else if (argv[0][0] != '-') {
	    struct sockaddr_storage sockaddr;
	    int ret;

	    ret = bitd_resolve_hostport(&sockaddr, sizeof(sockaddr), argv[0]);
	    if (!ret) {
		char addr_str[1024];
		int port = *bitd_sin_port(&sockaddr);

		if (g_verbose) {
		    inet_ntop(bitd_sin_family(&sockaddr), 
			      bitd_sin_addr(&sockaddr),
			      addr_str, sizeof(addr_str));
		    if ((bitd_sin_family(&sockaddr) == AF_INET6) && port) {
			printf("[%s]", addr_str);
		    } else {
			printf("%s", addr_str);
		    }
		    if (port) {
			printf(":%d\n", port);
		    } else {
			printf("\n");
		    }
		}
	    } else {
		if (g_verbose) {
		    printf("Could not resolve %s: ret %d, %s\n", 
			       argv[0], ret, bitd_gai_strerror(ret));
		}
		goto end;
	    }
        } else {
            printf("%s: Skipping invalid parameter %s\n", g_prog_name, argv[0]);
        }

        /* Skip to next argument */
        argc--;
        argv++;        
    }

end:
    bitd_sys_deinit();
    
    if (ret) {
	ret = -1;
    }

    return ret;
}
