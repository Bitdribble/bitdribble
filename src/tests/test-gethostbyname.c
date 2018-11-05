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
#include "bitd/platform-sys.h"
#include "bitd/platform-netdb.h"
#include "bitd/platform-socket.h"
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
           "    hostname\n"
           "        Do a DNS lookup for this hostname.\n"
           "    ipaddr\n"
           "        Do an inverse lookup for a dotted-decimal format\n"
           "        IP address.\n"
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
            bitd_uint32 addr, i;
            bitd_hostent h;

            /* Is this a hostname or an IP address? */
            addr = inet_addr(argv[0]);
            if (addr == 0xffffffff) {
                /* It's a hostname */
                h = bitd_gethostbyname(argv[0]);
            } else {
                h = bitd_gethostbyaddr(addr);
            }
            
            if (h) {
		if (g_verbose) {
		    if (h->h_name) {
			printf("Hostname: %s\n", h->h_name);
		    }
		    
		    if (h->h_addr_list[0]) {
			printf("Addresses:\n");
			
			for (i = 0; h->h_addr_list[i]; i++) {
			    char *c;
			    
			    c = inet_ntoa(*(struct in_addr *)(h->h_addr_list[i]));
			    printf("%s\n", c);
			}
		    }
		    
		    if (h->h_aliases[0]) {
			printf("Aliases:\n");
			
			for(i = 0; h->h_aliases[i]; i++) {
			    printf("%s\n", h->h_aliases[i]);
			}
		    }
		}
	    } else {
		if (g_verbose) {
		    printf("Could not resolve %s\n", argv[0]);
		}
		return -1;
            }

            /* Release resources */
            bitd_hostent_free(h);

        } else {
            printf("%s: Skipping invalid parameter %s\n", g_prog_name, argv[0]);
        }

        /* Skip to next argument */
        argc--;
        argv++;        
    }

end:
    bitd_sys_deinit();
    
    return 0;
}
