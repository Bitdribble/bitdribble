/*****************************************************************************
 *
 * Original Author: 
 * Creation Date:   
 * Description: 
 *
 ****************************************************************************/

/*****************************************************************************
 *                                INCLUDE FILES 
 *****************************************************************************/
#include "bitd/format.h"


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
 *                        snprintf_w_realloc
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
void snprintf_w_realloc(char **buf, int *buf_size, int *buf_idx,
			char *format_string, ...) {
    va_list args;

    va_start(args, format_string);
    vsnprintf_w_realloc(buf, buf_size, buf_idx, format_string, args);
    va_end(args);
}


/*
 *============================================================================
 *                        vsnprintf_w_realloc
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
void vsnprintf_w_realloc(char **buf, int *buf_size, int *buf_idx,
			 char *format_string, va_list args) {
    vsnprintf_w_r((bitd_realloc_t *)&realloc,
		  (void **)buf, buf_size, buf_idx, format_string, args);
}


/*
 *============================================================================
 *                        snprintf_w_msg_realloc
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
void snprintf_w_msg_realloc(bitd_msg *m, bitd_uint32 *m_size, bitd_uint32 *m_idx,
			    char *format_string, ...) {
    va_list args;

    va_start(args, format_string);
    vsnprintf_w_msg_realloc(m, m_size, m_idx, format_string, args);
    va_end(args);
}


/*
 *============================================================================
 *                        vsnprintf_w_msg_realloc
 *============================================================================
 * Description:     
 * Parameters:    
 * Returns:  
 */
void vsnprintf_w_msg_realloc(bitd_msg *m, bitd_uint32 *m_size, bitd_uint32 *m_idx,
			     char *format_string, va_list args) {
    vsnprintf_w_r((bitd_realloc_t *)&bitd_msg_realloc,
		  (void **)m, (int *)m_size, (int *)m_idx, 
		  format_string, args);
}


/*
 *============================================================================
 *                        snprintf_w_r
 *============================================================================
 * Description:     Generic snprintf function with ability 
 *                  to realloc the buffer
 * Parameters:    
 *     f - the realloc function pointer
 * Returns:  
 */
void snprintf_w_r(bitd_realloc_t *f_realloc, 
		  void **buf, int *buf_size, int *buf_idx,
		  char *format_string, ...) {
    va_list args;

    va_start(args, format_string);
    vsnprintf_w_r(f_realloc, buf, buf_size, buf_idx, format_string, args);
    va_end(args);
}


/*
 *============================================================================
 *                        vsnprintf_w_r
 *============================================================================
 * Description:     Generic snprintf function with ability 
 *                  to realloc the buffer
 * Parameters:    
 *     f - the realloc function pointer
 * Returns:  
 */
void vsnprintf_w_r(bitd_realloc_t *f_realloc, 
		   void **buf, int *buf_size, int *buf_idx,
		   char *format_string, va_list args) {
    int size, idx, idx1;    
    va_list args1;
    bitd_boolean need_realloc = FALSE;

    size = *buf_size;
    idx = *buf_idx;

    bitd_assert(idx <= size);
    
    if (idx + 1024 > size) {
	need_realloc = TRUE;
    }

    do {
	if (need_realloc) {
	    if (!size) {
		size = 1024;
	    } else if (size < 1024*1024) {
		size *= 2;
	    } else {
		size += 1024*1024;
	    }	
	    
	    *buf = f_realloc(*buf, size);
	    need_realloc = FALSE;
	}
	
	/* Operate on a copy of the args, because we may need 
	   to call vsnprintf() again */
	va_copy(args1, args);
	idx1 = vsnprintf((char *)*buf + idx, size - idx, format_string, args1);
	va_end(args1);
	if (idx1 < 0 || idx + idx1 > size) {
	    need_realloc = TRUE;
	}
    } while (need_realloc);

    *buf_size = size;
    *buf_idx = idx + idx1;
}
