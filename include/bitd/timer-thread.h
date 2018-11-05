/*****************************************************************************
 *
 * Original Author: 
 * Creation Date:   
 * Description: 
 *
 ****************************************************************************/

#ifndef _BITD_TIMER_THREAD_H_
#define _BITD_TIMER_THREAD_H_

/*****************************************************************************
 *                                INCLUDE FILES 
 *****************************************************************************/
#include "bitd/common.h"

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



/*****************************************************************************
 *                                  TYPES
 *****************************************************************************/
typedef void (tth_timer_expired_callback_t)(void *cookie);
typedef struct tth_timer_s *tth_timer_t;


/*****************************************************************************
 *                            FUNCTION DEFINITIONS
 *****************************************************************************/

/* Init/deinit the timer thread */
bitd_boolean tth_init(void);
void tth_deinit(void);

/* Set/cancel a timer */
tth_timer_t tth_timer_set_msec(bitd_uint32 tmo_msec,
			       tth_timer_expired_callback_t *callback,
			       void *cookie,
			       bitd_boolean periodic_p);
tth_timer_t tth_timer_set_nsec(bitd_uint64 tmo_nsec,
			       tth_timer_expired_callback_t *callback,
			       void *cookie,
			       bitd_boolean periodic_p);
void tth_timer_destroy(tth_timer_t);



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _BITD_TIMER_THREAD_H_ */
