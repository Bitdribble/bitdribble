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

#ifndef _BITD_PLATFORM_TIME_H_
#define _BITD_PLATFORM_TIME_H_

/*****************************************************************************
 *                                INCLUDE FILES 
 *****************************************************************************/

#include "bitd/platform-types.h"

#ifdef BITD_HAVE_SYS_TIME_H
# include "sys/time.h"
#endif

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

/*****************************************************************************
 *                             MANIFEST CONSTANTS
 *****************************************************************************/



/*****************************************************************************
 *                                  MACROS 
 *****************************************************************************/

#define BITD_TIMEVAL_CPY(result, a)                      \
  do {                                                 \
    (result)->tv_sec = (a)->tv_sec;                    \
    (result)->tv_usec = (a)->tv_usec;                  \
  } while (0)

/* Does not work for >= or <= */
#define BITD_TIMEVAL_CMP(a, b, CMP)                      \
  (((a)->tv_sec == (b)->tv_sec) ?                      \
   ((a)->tv_usec CMP (b)->tv_usec) :                   \
   ((a)->tv_sec CMP (b)->tv_sec))

#define BITD_TIMEVAL_ADD(result, a, b)                   \
  do {                                                 \
    (result)->tv_sec = (a)->tv_sec + (b)->tv_sec;      \
    (result)->tv_usec = (a)->tv_usec + (b)->tv_usec;   \
    if ((result)->tv_usec > 1000000) {                 \
      ++(result)->tv_sec;                              \
      (result)->tv_usec -= 1000000;                    \
    }                                                  \
  } while (0)

#define BITD_TIMEVAL_SUB(result, a, b)                   \
  do {                                                 \
    (result)->tv_sec = (a)->tv_sec - (b)->tv_sec;      \
    (result)->tv_usec = (a)->tv_usec - (b)->tv_usec;   \
    if ((result)->tv_usec < 0) {                       \
      --(result)->tv_sec;                              \
      (result)->tv_usec += 1000000;                    \
    }                                                  \
  } while (0)

/* Next three macros assume unsigned input */
#define BITD_NANO_TO_TIMEVAL(result, a)                  \
  do {                                                 \
    (result)->tv_sec = (a)/1000000000;                 \
    (result)->tv_usec = ((a)%1000000000)/1000;         \
  } while (0)

#define BITD_MICRO_TO_TIMEVAL(result, a)                 \
  do {                                                 \
    (result)->tv_sec = (a)/1000000;                    \
    (result)->tv_usec = (a)%1000000;                   \
  } while (0)

#define BITD_MILLI_TO_TIMEVAL(result, a)                 \
  do {                                                 \
    (result)->tv_sec = (a)/1000;                       \
    (result)->tv_usec = ((a)%1000)*1000;               \
  } while (0)


/* Next three macros do not handle time overflow */
#define BITD_TIMEVAL_TO_NANO(a)                          \
    ((a)->tv_sec * 1000000000 + (a)->tv_usec * 1000)

#define BITD_TIMEVAL_TO_MICRO(a)                         \
    ((a)->tv_sec * 1000000 + (a)->tv_usec)

#define BITD_TIMEVAL_TO_MILLI(a)                         \
    ((a)->tv_sec * 1000 + (a)->tv_usec / 1000)


#define BITD_FOREVER 0xffffffffUL


/*****************************************************************************
 *                                  TYPES
 *****************************************************************************/



/*****************************************************************************
 *                            FUNCTION DEFINITIONS
 *****************************************************************************/

/* returns time since 1/1/1970 */
void bitd_gettimeofday(struct timeval *tm);

/* returns msecs since 1/1/1970 - may wrap around */
bitd_uint32 bitd_get_time_msec(void);

/* returns nsecs since 1/1/1970 */
bitd_uint64 bitd_get_time_nsec(void);




#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _BITD_PLATFORM_TIME_H_ */
