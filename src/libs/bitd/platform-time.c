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

#if defined(_WIN32)
# define FILETIME_1970 0x019db1ded53e8000ULL /* vc++ prefers xui64 */
#endif

#if defined(BITD_HAVE_SYS_TIME_H)
# include <sys/time.h>
#endif

#if defined(BITD_HAVE_INTTYPES_H)
# include <inttypes.h>
#endif

#if defined(BITD_HAVE_TIME_H)
# include <time.h>
#endif


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
 *                        bitd_gettimeofday
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
void bitd_gettimeofday(struct timeval *tm) {
#if defined(_WIN32)
    FILETIME ft;
    ULONGLONG time;

    /* Get the system time */
    GetSystemTimeAsFileTime(&ft);
    time = ((ULONGLONG) ft.dwHighDateTime << 32) +
        (ULONGLONG) ft.dwLowDateTime;
 
    time -= FILETIME_1970;
    tm->tv_sec = (LONG)(time/10000000ULL);
    tm->tv_usec = (LONG)((time%10000000ULL)/10);   
#elif defined(BITD_HAVE_GETTIMEOFDAY)
    gettimeofday(tm, NULL);
#else
# error "gettimeofday() not defined"
#endif
} 

/*
 *============================================================================
 *                        bitd_get_time_nsec
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
bitd_uint64 bitd_get_time_nsec(void) {
#if defined(BITD_HAVE_CLOCK_GETTIME)
    struct timespec spec;
    
    clock_gettime(CLOCK_REALTIME, &spec);

    return ((bitd_uint64)spec.tv_sec)*1000000000ULL + (bitd_uint64)spec.tv_nsec;
#else
    struct timeval tm;
    
    bitd_gettimeofday(&tm);
    
    return tm.tv_sec * 1000000000ULL + tm.tv_usec * 1000ULL;
#endif
} 


/*
 *============================================================================
 *                        bitd_get_time_msec
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
bitd_uint32 bitd_get_time_msec(void) {
  struct timeval tm;

  bitd_gettimeofday(&tm);

  return((tm.tv_sec * 1000) + (tm.tv_usec / 1000));
} 


