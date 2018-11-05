/*****************************************************************************
 *
 * Original Author: 
 * Creation Date:   
 * Description: 
 *
 ****************************************************************************/

#ifndef _BITD_FORMAT_H_
#define _BITD_FORMAT_H_

/*****************************************************************************
 *                                INCLUDE FILES 
 *****************************************************************************/
#include "bitd/common.h"
#include "bitd/msg.h"

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
typedef void *(bitd_realloc_t)(void *, int size);


/*****************************************************************************
 *                            FUNCTION DEFINITIONS
 *****************************************************************************/

/* snprintf() into buffer, with realloc */
void snprintf_w_realloc(char **buf, int *buf_size, int *buf_idx,
			char *format_string, ...);
void vsnprintf_w_realloc(char **buf, int *buf_size, int *buf_idx,
			 char *format_string, va_list args);

/* snprintf() into message, with realloc */
void snprintf_w_msg_realloc(bitd_msg *m, bitd_uint32 *m_size, bitd_uint32 *m_idx,
			    char *format_string, ...);
void vsnprintf_w_msg_realloc(bitd_msg *m, bitd_uint32 *m_size, bitd_uint32 *m_idx,
			     char *format_string, va_list args);


/* snprintf() into buffer, with arbitrary realloc */
void snprintf_w_r(bitd_realloc_t *f_realloc, 
		  void **buf, int *buf_size, int *buf_idx,
		  char *format_string, ...);
void vsnprintf_w_r(bitd_realloc_t *f_realloc, 
		   void **buf, int *buf_size, int *buf_idx,
		   char *format_string, va_list args);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _BITD_FORMAT_H_ */
