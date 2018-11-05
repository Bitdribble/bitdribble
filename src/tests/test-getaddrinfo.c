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
	    char *arg = NULL;
	    char *hostname = NULL;
	    char *service = NULL;
	    char *c, *c1, *c2;
	    int port = 0;
	    struct sockaddr_storage sock_addr;
	    bitd_boolean use_getnameinfo = FALSE;

	    arg = strdup(argv[0]);
	    c = strchr(arg, '[');
	    c1 = strchr(arg, ']');
	    if (c && c1 && c1 > c) {
		*c1++ = 0;
		hostname = strdup(c+1);

		if (*c1 == ':') {
		    service = strdup(c1+1);
		}
	    } else {
		c2 = strchr(arg, ':');
		if (c2) {
		    *c2++ = 0;
		    hostname = arg;
		    arg = NULL;
		    service = strdup(c2);
		} else {
		    hostname = arg;
		    arg = NULL;
		}
	    }

	    memset(&sock_addr, 0, sizeof(sock_addr));
	    bitd_set_sin_family(&sock_addr, AF_INET);

	    if (bitd_inet_pton(AF_INET6, hostname, 
			     bitd_sin_addr(&sock_addr)) > 0) {
		use_getnameinfo = TRUE;
		bitd_set_sin_family(&sock_addr, AF_INET6);
	    } else if (bitd_inet_pton(AF_INET, hostname, 
				    bitd_sin_addr(&sock_addr)) > 0) {
		use_getnameinfo = TRUE;		
	    } 

	    if (use_getnameinfo) {
		char host[1024];
		char serv[1024];

		if (service) {
		    port = atoi(service);
		    if (port > 0 && port <= 65535) {
			*bitd_sin_port(&sock_addr) = htons((bitd_uint16)port);
		    }
		}

		memset(host, 0, sizeof(host));
		memset(serv, 0, sizeof(serv));

		ret = bitd_getnameinfo((struct sockaddr *)&sock_addr,
				     bitd_sockaddrlen(&sock_addr),
				     host, sizeof(host) - 1,
				     serv, sizeof(serv) - 1,
				     0);
		if (!ret) {
		    if (g_verbose) {
			printf("Hostname: %s\n", host);
			if (serv[0]) {
			    printf("Service: %s\n", serv);
			}
		    }
		} else {
		    if (g_verbose) {
			printf("Could not resolve %s: ret %d, %s\n", 
			       argv[0], ret, bitd_gai_strerror(ret));
		    }
		}
	    } else {		
		bitd_addrinfo_t a;

		ret = bitd_getaddrinfo(hostname, service, NULL, &a);
		if (!ret) {
		    bitd_addrinfo_t res;

		    if (g_verbose) {
			for (res = a; res; res=res->ai_next) {
			    char addr_str[24];
			    
			    if (res->ai_addr) {
				memset(addr_str, 0, sizeof(addr_str));
				bitd_inet_ntop(res->ai_family,
					     bitd_sin_addr((struct sockaddr_storage *)res->ai_addr),
					     addr_str,
					     res->ai_addrlen);
				if (res->ai_protocol) {
				    continue;
				}
				if (addr_str[0]) {
				    printf("Address: %s\n", addr_str);
				}
				if (res->ai_canonname) {
				    printf("Name: %s\n", res->ai_canonname);
				}
			    }
			}
		    }
		    bitd_freeaddrinfo(a);
		} else {
		    if (g_verbose) {
			printf("Could not resolve %s: ret %d, %s\n", 
			       argv[0], ret, bitd_gai_strerror(ret));
		    }
		}
	    }

	    if (arg) {
		free(arg);
	    }
	    if (hostname) {
		free(hostname);
	    }
	    if (service) {
		free(service);
	    }

	    if (ret) {
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
